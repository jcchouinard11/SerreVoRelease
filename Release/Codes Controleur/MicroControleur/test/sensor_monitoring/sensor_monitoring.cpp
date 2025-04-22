#include "sensor_monitoring.h"

// ------------------------------------------------
// Traitement et detection des capteurs
// ------------------------------------------------
void selectMuxChannel(uint8_t bus, uint8_t channel) 
{
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    i2c_buses[bus].write(1 << channel);
    i2c_buses[bus].endTransmission();
}

bool isMuxPresent(uint8_t bus) 
{
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    return (i2c_buses[bus].endTransmission() == 0);
}
int findExistingSensor(uint8_t bus, uint8_t mux_channel, bool is_mux) 
{
    for (int i = 0; i < sensorCount; i++) 
    {
        if (sensorTable[i].bus == bus && sensorTable[i].mux_channel == mux_channel && sensorTable[i].is_mux == is_mux) {
            sensorTable[i].present = true;
            return i;
        }
    }
    return -1;
}

void displayData() {
    if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) 
    {
      for (uint8_t i = 0; i < sensorCount; i++) 
      {
          Serial.printf("Sensor %d (Bus %d, %s %d): Humidity = %.2f, Temp = %.2f\n", 
                        sensorTable[i].index, sensorTable[i].bus, 
                        sensorTable[i].is_mux ? "Mux" : "Direct", 
                        sensorTable[i].is_mux ? sensorTable[i].mux_channel : 255,
                        sensorTable[i].humidity, sensorTable[i].temperature);
      }
      xSemaphoreGive(data_mutex);
    }
} 
void readSensors() 
{
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
            
            sensorTable[i].humidity = (float)sensors[i]->touchRead(0)/1016.0*100.0;
            sensorTable[i].temperature = sensors[i]->getTemp();
            
            if(sensorTable[i].humidity>3000||sensorTable[i].temperature >500)
            {
              sensorTable[i].present= false;
            }
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
    sensorTable[sensorCount] = {sensorCount, SHTBUS, 255, false, (int)sht31.readHumidity(), sht31.readTemperature(), true};
    float sharedTempAmbiante = sht31.readTemperature();
    float sharedHumiditeAmbiante = sht31.readHumidity();
    Serial.printf("%.2f",sharedTempAmbiante);
    Serial.printf("%.2f",sharedHumiditeAmbiante);
    xSemaphoreGive(data_mutex);
    }
}
void detectSensors(void *pvParameters) {
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

            
            xSemaphoreGive(data_mutex);
        }
        
        readSensors();
        displayData();
       int ret = xQueueSend(QueueHandle, (void *)&sensorTable, 0);
      
        if (ret == pdTRUE) 
        {
            // The message was successfully sent.
            Serial.println("[Task_Sensor] The message was successfully sent.");
        } else if (ret == errQUEUE_FULL) 
        {
            // Since we are checking uxQueueSpacesAvailable this should not occur, however if more than one task should
            //   write into the same queue it can fill-up between the test and actual send attempt
            Serial.println("[Task_Sensor] Task Sensor was unable to send data into the Queue");
        }  // Queue send check 
        delay(1000);
        }
        
        
      
    }
    


//Utilisé pour des fins de debug