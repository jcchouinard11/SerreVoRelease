#include "capteurs.h"
#include "BLE.h"

float raw_value;
bool muxDetected = false;

void initTempSensor() {
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    temp_sensor.dac_offset = TSENS_DAC_L2;
    temp_sensor_set_config(temp_sensor);
    temp_sensor_start();
}

void selectMuxChannel(uint8_t bus, uint8_t channel) {
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    i2c_buses[bus].write(1 << channel);
    i2c_buses[bus].endTransmission();
}

bool isMuxPresent(uint8_t bus) {
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    return (i2c_buses[bus].endTransmission() == 0);
}

void initializeSensorTable() {
    u_int8_t i = 0;
    for (uint8_t bus = 0; bus < NUM_BUSES; bus++) {
        // Capteur direct (sans mux)
        sensorTable[i] = {i, bus, 255, false, 0, 0, false};
        sensors[i] = new Adafruit_seesaw(&i2c_buses[bus]);
        i++;

        // Capteurs via mux
        for (uint8_t mux = 0; mux < NUM_MUX; mux++) {
            sensorTable[i] = {i, bus, mux, true, 0, 0, false};
            sensors[i] = new Adafruit_seesaw(&i2c_buses[bus]);
            i++;
        }
    }
}

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

void readSensors() {
    for (uint8_t i = 0; i < MAX_TOTAL_SENSORS; i++) {
        if (sensorTable[i].present)
        {
            if (sensorTable[i].is_mux) {
                selectMuxChannel(sensorTable[i].bus, sensorTable[i].mux_channel);
            }
    
            raw_value = sensors[i]->touchRead(0);
            sensorTable[i].humidity = ((raw_value - 264.0f) / (1016.0f - 264.0f)) * 100.0f;
            sensorTable[i].temperature = sensors[i]->getTemp();
    
            if (sensorTable[i].humidity  > 100 || sensorTable[i].temperature > 125) {
                sensorTable[i].present = false;
                sensorCount--;
            }
        }
        }

        

    sharedHumiditeAmbiante = sht31.readHumidity();
    sharedTempAmbiante = sht31.readTemperature();
}

void detectSensors(void *pvParameters) {
    initializeSensorTable();  // Prédéfinir les capteurs possibles une seule fois
    printf("\n[DETECTSENSOR] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));

    while (1) {
        //printf("\nRAM USAGE BEFORE:%i",ESP.getFreeHeap()); 
        
        if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) {
            for (uint8_t i = 0; i < MAX_TOTAL_SENSORS; i++) {
                muxDetected = isMuxPresent(sensorTable[i].bus);
                    if (sensorTable[i].is_mux) {
                        if (muxDetected) {
                            selectMuxChannel(sensorTable[i].bus, sensorTable[i].mux_channel);
                            if(!sensorTable[i].present)
                            {
                                if (sensors[i]->begin(0x36) && sensorTable[i].mux_channel !=255) {
                                    sensorTable[i].present = true;
                                    sensorCount++;
                                } 
                                else {
                                    sensorTable[i].present = false;
                                }
                            }
                            
                        }
                        else {
                            sensorTable[i].present = false;
                            
                        }
                    }
                    else if(!sensorTable[i].present && !muxDetected)
                    {
                        if (sensors[i]->begin(0x36)) {
                        sensorTable[i].present = true;
                        sensorCount++;
                    } 
                    else {
                        sensorTable[i].present = false;
                    }
                }

                    
                    
                }
            shtPresent = sht31.begin();
            //displayData();
            xSemaphoreGive(data_mutex);
        }
        
        
        
        //printf("\nRAM USAGE AFTER:%i",ESP.getFreeHeap()); 

        delay(1500);
    }
}
