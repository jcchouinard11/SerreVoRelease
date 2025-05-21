#include "capteurs.h"
#include "BLE.h"

// Valeur brute lue sur les capteurs Seesaw (humidité)
float raw_value;
TaskHandle_t detectSensorshandle;
// Indique si un multiplexeur est détecté sur le bus
bool muxDetected = false;

// Initialise le capteur de température interne de l'ESP32
void initTempSensor() {
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor.dac_offset = TSENS_DAC_L2;
    temp_sensor_set_config(temp_sensor);
    temp_sensor_start();
}

// Sélectionne un canal spécifique sur un multiplexeur I2C donné
void selectMuxChannel(uint8_t bus, uint8_t channel) {
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    i2c_buses[bus].write(1 << channel); // Active le canal en décalant le bit
    i2c_buses[bus].endTransmission();
}

// Vérifie si un multiplexeur est présent sur un bus I2C
bool isMuxPresent(uint8_t bus) {
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    return (i2c_buses[bus].endTransmission() == 0);
}

// Initialise la table des capteurs avec les entrées prévues (directs et via mux)
void initializeSensorTable() {
    u_int8_t i = 0;
    for (uint8_t bus = 0; bus < NUM_BUSES; bus++) {
        // Capteur connecté directement (sans multiplexeur)
        sensorTable[i] = {i, bus, 255, false, 0, 0, false};
        sensors[i] = new Adafruit_seesaw(&i2c_buses[bus]);
        i++;

        // Capteurs via multiplexeurs
        for (uint8_t mux = 0; mux < NUM_MUX; mux++) {
            sensorTable[i] = {i, bus, mux, true, 0, 0, false};
            sensors[i] = new Adafruit_seesaw(&i2c_buses[bus]);
            i++;
        }
    }
}

// Affiche les données actuelles de tous les capteurs dans la console
void displayData() {
    printf("=== Sensor Data ===\n");
    for (uint8_t i = 0; i < MAX_TOTAL_SENSORS; i++) {
        printf("S%d [Bus %d, %s %d]: H=%d%%, T=%.2f°C\n",
        sensorTable[i].index,
        sensorTable[i].bus,
        sensorTable[i].is_mux ? "Mux" : "Dir",
        sensorTable[i].is_mux ? sensorTable[i].mux_channel : 255,
        sensorTable[i].humidity,
        sensorTable[i].temperature);
    }
    printf("SHT31: H=%.2f%%, T=%.2f°C\n", sharedHumiditeAmbiante, sharedTempAmbiante);
    printf("===================\n");
}

// Lit les valeurs de tous les capteurs présents et les stocke dans sensorTable
void readSensors() {
    for (uint8_t i = 0; i < MAX_TOTAL_SENSORS; i++) {
        if (sensorTable[i].present)
        {
            // Si le capteur est sur un mux, sélectionne le canal approprié
            if (sensorTable[i].is_mux) {
                selectMuxChannel(sensorTable[i].bus, sensorTable[i].mux_channel);
            }

            // Lecture de l’humidité brute, convertie en pourcentage (calibrée)
            raw_value = sensors[i]->touchRead(0);
            sensorTable[i].humidity = ((raw_value - 264.0f) / (1016.0f - 264.0f)) * 100.0f;
            sensorTable[i].temperature = sensors[i]->getTemp();

            // Si des valeurs erronées sont détectées, le capteur est marqué absent
            if (sensorTable[i].humidity  > 100 || sensorTable[i].temperature > 125) {
                sensorTable[i].present = false;
                sensorCount--;
            }
        }
    }

    // Mise à jour des données du capteur ambiant (SHT31)
    sharedHumiditeAmbiante = sht31.readHumidity();
    sharedTempAmbiante = sht31.readTemperature();
}

// Tâche FreeRTOS : détection automatique des capteurs présents (exécutée en boucle)
void detectSensors(void *pvParameters) {
    //initializeSensorTable(); // Remplit sensorTable avec les capteurs possibles
    detectSensorshandle = xTaskGetCurrentTaskHandle(); // Récupération de l'identifiant de la tâche BLE
    printf("\n[DETECTSENSOR] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));

    while (1) {
        if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) {
            for (uint8_t i = 0; i < MAX_TOTAL_SENSORS; i++) {
                muxDetected = isMuxPresent(sensorTable[i].bus);
                if (sensorTable[i].is_mux) {
                    
                    if (muxDetected) {
                        selectMuxChannel(sensorTable[i].bus, sensorTable[i].mux_channel);

                        // Si non déjà présent, tente une détection (via begin)
                        if (!sensorTable[i].present) {
                            if (sensors[i]->begin(0x36) && sensorTable[i].mux_channel != 255) {
                                sensorTable[i].present = true;
                                sensorCount++;
                            } else {
                                if(sensorTable[i].present) {
                                    // Si le capteur était déjà présent, on le marque comme absent
                                    sensorCount--;
                                }
                                sensorTable[i].present = false;
                            }
                        }
                    } else {
                        if(sensorTable[i].present) {
                            // Si le capteur était déjà présent, on le marque comme absent
                            sensorCount--;
                        }
                        sensorTable[i].present = false; // mux absent
                    }
                }
                // Capteur direct (non muxé)
                else if (!sensorTable[i].present && !muxDetected && sensorTable[i].mux_channel == 255 && sensorTable[i].is_mux == false) {
                    if (sensors[i]->begin(0x36)) {
                        sensorTable[i].present = true;
                        sensorCount++;
                    } else {
                        if(sensorTable[i].present) {
                            // Si le capteur était déjà présent, on le marque comme absent
                            sensorCount--;
                        }
                        sensorTable[i].present = false;
                    }
                }
            }

            // Vérifie si le capteur d'air est encore présent
            shtPresent = sht31.begin();
            xSemaphoreGive(data_mutex);
        }

        // Délai de 1.5s entre les tentatives de détection
        delay(1500);
    }
}
void singleDetectSensorWithRead(){
    initializeSensorTable(); // Remplit sensorTable avec les capteurs possibles

    if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) {
        for (uint8_t i = 0; i < MAX_TOTAL_SENSORS; i++) {
            muxDetected = isMuxPresent(sensorTable[i].bus);
            if (sensorTable[i].is_mux) {
                
                if (muxDetected) {
                    selectMuxChannel(sensorTable[i].bus, sensorTable[i].mux_channel);

                    // Si non déjà présent, tente une détection (via begin)
                    if (!sensorTable[i].present) {
                        if (sensors[i]->begin(0x36) && sensorTable[i].mux_channel != 255) {
                            sensorTable[i].present = true;
                            sensorCount++;
                        } else {
                            if(sensorTable[i].present) {
                                // Si le capteur était déjà présent, on le marque comme absent
                                sensorCount--;
                            }
                            sensorTable[i].present = false;
                        }
                    }
                } else {
                    if(sensorTable[i].present) {
                        // Si le capteur était déjà présent, on le marque comme absent
                        sensorCount--;
                    }
                    sensorTable[i].present = false; // mux absent
                }
            }
            // Capteur direct (non muxé)
            else if (!sensorTable[i].present && !muxDetected && sensorTable[i].mux_channel == 255 && sensorTable[i].is_mux == false) {
                if (sensors[i]->begin(0x36)) {
                    sensorTable[i].present = true;
                    sensorCount++;
                } else {
                    if(sensorTable[i].present) {
                        // Si le capteur était déjà présent, on le marque comme absent
                        sensorCount--;
                    }
                    sensorTable[i].present = false;
                }
            }
        }

        // Vérifie si le capteur d'air est encore présent
        shtPresent = sht31.begin();
        xSemaphoreGive(data_mutex);
        readSensors();
    }
}