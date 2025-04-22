#include <Arduino.h>
#include "SensorManager.h"
// ------------------------------------------------
// Traitement et detection des capteurs
// ------------------------------------------------
void selectMuxChannel(uint8_t bus, uint8_t channel) {
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    i2c_buses[bus].write(1 << channel);
    i2c_buses[bus].endTransmission();
}

bool isMuxPresent(uint8_t bus) {
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    return (i2c_buses[bus].endTransmission() == 0);
}

void detectSensors(void *pvParameters) {
    SensorData sensorTable[MAX_TOTAL_SENSORS];
    uint8_t sensorCount = 0;
    while (1) 
    {
            for (uint8_t bus = 0; bus < NUM_BUSES; bus++) 
            {
            // Check for direct sensors on the bus
            sensors[sensorCount] = new Adafruit_seesaw(&i2c_buses[bus]);
            if (sensors[sensorCount]->begin(0x36)) 
            {
                sensorTable[sensorCount] = {sensorCount, bus, 255, false, 0, 0};
                sensorCount++;
            } else 
            {
                delete sensors[sensorCount];
                sensors[sensorCount] = nullptr;
            }
            
            // Check for sensors connected via multiplexers only if a mux is present
            if (isMuxPresent(bus)) 
            {
                for (uint8_t mux = 0; mux < NUM_MUX; mux++) 
                {
                    selectMuxChannel(bus, mux);
                    sensors[sensorCount] = new Adafruit_seesaw(&i2c_buses[bus]);
                    if (sensors[sensorCount]->begin(0x36)) 
                    {
                        sensorTable[sensorCount] = {sensorCount, bus, mux, true, 0, 0};
                        sensorCount++;
                    } else 
                    {
                        delete sensors[sensorCount];
                        sensors[sensorCount] = nullptr;
                    }
                }
            }
        }
        readSensors();
        int ret = xQueueSend(QueueHandle, (void *)&sensorTable, 0);
      
        if (ret == pdTRUE) {
            // The message was successfully sent.
            Serial.println("[Task_Sensor] The message was successfully sent.");
        } else if (ret == errQUEUE_FULL) {
            // Since we are checking uxQueueSpacesAvailable this should not occur, however if more than one task should
            //   write into the same queue it can fill-up between the test and actual send attempt
            Serial.println("[Task_Sensor] Task Sensor was unable to send data into the Queue");
        }  // Queue send check
        }
        delay(5000); // Update data every second
        
      
    }
    

void readSensors() {
    uint8_t sensorCount = 0;
    for (uint8_t i = 0; i < sensorCount; i++) {
        if (sensorTable[i].is_mux) {
            selectMuxChannel(sensorTable[i].bus, sensorTable[i].mux_channel);
        }
        if (sensors[i]) {
            sensorTable[i].humidity = sensors[i]->touchRead(0);
            sensorTable[i].temperature = sensors[i]->getTemp();
        }
    }
}
//Utilisé pour des fins de debug
/* void displayData() {
    uint8_t sensorCount = 0;
    for (uint8_t i = 0; i < sensorCount; i++) {
        Serial.printf("Sensor %d (Bus %d, %s %d): Humidity = %.2f, Temp = %.2f\n", 
                      sensorTable[i].index, sensorTable[i].bus, 
                      sensorTable[i].is_mux ? "Mux" : "Direct", 
                      sensorTable[i].is_mux ? sensorTable[i].mux_channel : 255,
                      sensorTable[i].humidity, sensorTable[i].temperature);
    }
} */