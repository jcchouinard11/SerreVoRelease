/ ------------------------------------------------
// Traitement et détection des capteurs
// ------------------------------------------------
void selectMuxChannel(uint8_t bus, uint8_t channel) 
{
    // Sélection du canal du multiplexeur pour le bus spécifié
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    i2c_buses[bus].write(1 << channel);  // Active le canal en envoyant une valeur correspondante
    i2c_buses[bus].endTransmission();
}
/**
 * @brief fonction qui met a jour les données des capteurs ainsi les capteurs sont aussi verifié pour voir si il sont erronées ou débranchées 
 * @param void *parameter
 * @return Rien
 */
bool isMuxPresent(uint8_t bus) 
{
    // Vérifie la présence du multiplexeur sur le bus spécifié
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    return (i2c_buses[bus].endTransmission() == 0);  // Retourne vrai si le multiplexeur est présent
}
/**
 * @brief fonction qui met a jour les données des capteurs ainsi les capteurs sont aussi verifié pour voir si il sont erronées ou débranchées 
 * @param void *parameter
 * @return Rien
 */
int findExistingSensor(uint8_t bus, uint8_t mux_channel, bool is_mux) 
{
    // Recherche si un capteur existe déjà dans la table sensorTable
    for (int i = 0; i < sensorCount; i++) 
    {
        if (sensorTable[i].bus == bus && sensorTable[i].mux_channel == mux_channel && sensorTable[i].is_mux == is_mux) {
            sensorTable[i].present = true;
            return i;
        }
    }
    return -1; // Aucun capteur trouvé
}

/**
 * @brief Utilisé a des fins de débuggage 
 * @param void *parameter
 * @return Rien
 */

void displayData() {
    if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) 
    {
        printf("=== Sensor Data ===\n");
        
        for (uint8_t i = 0; i < sensorCount; i++) 
        {
            if (sensorTable[i].present) 
            {
                printf("S%d [Bus %d, %s %d]: H=%d%%, T=%.2f°C\n", 
                    sensorTable[i].index, 
                    sensorTable[i].bus, 
                    sensorTable[i].is_mux ? "Mux" : "Dir", 
                    sensorTable[i].is_mux ? sensorTable[i].mux_channel : 255,
                    sensorTable[i].humidity, 
                    sensorTable[i].temperature
                );
            }
            else
            {
                printf("S%d [Bus %d]: Not detected\n", sensorTable[i].index, sensorTable[i].bus);
            }
        }

        printf("SHT31: H=%.2f%%, T=%.2f°C\n", sharedHumiditeAmbiante, sharedTempAmbiante);
        printf("===================\n");
        
        xSemaphoreGive(data_mutex);
    }
}
/**
 * @brief fonction qui met a jour les données des capteurs ainsi les capteurs sont aussi verifié pour voir si il sont erronées ou débranchées 
 * @param void *parameter
 * @return Rien
 */
void readSensors() 
{
    // Lecture des capteurs et mise à jour des données dans sensorTable
    if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) 
    {
        for (uint8_t i = 0; i < sensorCount; i++) 
        {
            if (sensorTable[i].is_mux) 
            {
                selectMuxChannel(sensorTable[i].bus, sensorTable[i].mux_channel);
            }

            if (sensors[i]) 
            {
                // Lecture de l'humidité et de la température depuis le capteur
                sensorTable[i].humidity = (float)sensors[i]->touchRead(0)/1016.0*100.0;
                sensorTable[i].temperature = sensors[i]->getTemp();
                
                // Vérifie les valeurs extrêmes et marque le capteur comme absent si nécessaire
                if(sensorTable[i].humidity>3000 || sensorTable[i].temperature >500)
                {
                  sensorTable[i].present = false;
                }

                // Suppression des capteurs qui ne sont plus valides
                for (uint8_t i = 0; i < sensorCount; i++) 
                {
                    if (!sensorTable[i].present) 
                    {
                        delete sensors[i];
                        sensors[i] = nullptr;
                        Serial.printf("Sensor %d removed.\n", i);
                        for (int j = i; j < sensorCount - 1; j++) 
                        {
                            sensorTable[j] = sensorTable[j + 1];
                            sensors[j] = sensors[j + 1];
                        }
                        sensorCount--;
                        i--;
                    }
                }
            }
        }
        // Ajoute le capteur SHT31 pour la température et l'humidité ambiantes
        sharedHumiditeAmbiante=sht31.readHumidity();
        sharedTempAmbiante=sht31.readTemperature();

        xSemaphoreGive(data_mutex);
    }
}
/**
 * @brief Tache Async qui met verifie les ports i2c du projet et verifie les capteurs 
 * @param void *parameter
 * @return Rien
 */
void detectSensors(void *pvParameters) {
    bool ledState = false;
    Serial.printf("\n[DETECTSENSOR] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
    while (1) {
        if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) 
        {
            for (uint8_t i = 0; i < sensorCount; i++) 
            {
                sensorTable[i].present = false;
            }
            for (uint8_t bus = 0; bus < NUM_BUSES-1; bus++) 
            {
                int existingIndex = findExistingSensor(bus, 255, false);
                if (existingIndex == -1) 
                {
                    sensors[sensorCount] = new Adafruit_seesaw(&i2c_buses[bus]);
                    if (sensors[sensorCount]->begin(0x36)) 
                    {
                        sensorTable[sensorCount] = {sensorCount, bus, 255, false, 0, 0, true};
                        sensorCount++;
                    }
                    else 
                    {
                        delete sensors[sensorCount];
                        sensors[sensorCount] = nullptr;
                    }
                }

                if (isMuxPresent(bus)) 
                {
                    for (uint8_t mux = 0; mux < NUM_MUX; mux++) 
                    {
                        existingIndex = findExistingSensor(bus, mux, true);
                        if (existingIndex == -1) 
                        {
                            selectMuxChannel(bus, mux);
                            sensors[sensorCount] = new Adafruit_seesaw(&i2c_buses[bus]);
                            if (sensors[sensorCount]->begin(0x36)) 
                            {
                                sensorTable[sensorCount] = {sensorCount, bus, mux, true, 0, 0, true};
                                sensorCount++;
                            } else 
                            {
                                delete sensors[sensorCount];
                                sensors[sensorCount] = nullptr;
                            }
                        }
                    }
                }
            }

            if(sht31.begin())
            {
                shtPresent=true;
            }else
            {
                shtPresent=false;
            }
            xSemaphoreGive(data_mutex);
        }
        
        readSensors();
        displayData();
/*         if(ledState){
            ledState = false;
            digitalWrite(42,LOW); //
        }else{
            ledState = true;
            digitalWrite(42, HIGH); //
        } */
        
       int ret = xQueueSend(QueueHandle, (void *)&sensorTable, 0);
      
       /*  if (ret == pdTRUE) 
        {
            // The message was successfully sent.
            Serial.println("[Task_Sensor] The message was successfully sent.");
        } else if (ret == errQUEUE_FULL) 
        {
            // Since we are checking uxQueueSpacesAvailable this should not occur, however if more than one task should
            //   write into the same queue it can fill-up between the test and actual send attempt
            Serial.println("[Task_Sensor] Task Sensor was unable to send data into the Queue");
        }  // Queue send check  */
        delay(1000);
        }
        
        
      
    }
