#ifndef SENSOR_LIBRARY_H
#define SENSOR_LIBRARY_H

#include <Arduino.h>
#include <Wire.h>
#include "ui.h"
#include "Adafruit_seesaw.h"
#include "Adafruit_SHT31.h"  // Capteur SHT31 (température et humidité)
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define NUM_BUSES 5
#define NUM_MUX 4
#define SHTBUS (NUM_BUSES)
#define MUX_ADDR 0x70
#define SHT31ADRESS 0x44
#define MAX_SENSORS_PER_MUX 4
#define MAX_TOTAL_SENSORS (NUM_MUX * MAX_SENSORS_PER_MUX)
// Touchscreen pins
#define XPT2046_IRQ  255  // T_IRQ
#define XPT2046_MOSI 4  // T_DIN
#define XPT2046_MISO 15  // T_OUT
#define XPT2046_CLK 47   // T_CLK
#define XPT2046_CS 16    // T_CS

objects_t objects;
uint8_t sdapin[NUM_BUSES] = { 45, 1, 12, 42, 40 };
uint8_t sclpin[NUM_BUSES] = { 48, 2, 11, 39, 41 };
// Array of Flexwire instances
FlexWire i2c_buses[NUM_BUSES] = { {sdapin[0], sclpin[0]}, {sdapin[1], sclpin[1]},{sdapin[2], sclpin[2]},{sdapin[3], sclpin[3]},{sdapin[4], sclpin[4]} }; 

extern Adafruit_seesaw* sensors[MAX_TOTAL_SENSORS];
extern Adafruit_SHT31 sht31 = Adafruit_SHT31(&i2c_buses[4]); // Capteur d'humidité/température SHT31

struct SensorData {
    uint8_t index;
    uint8_t bus;
    uint8_t mux_channel;
    bool is_mux;
    int humidity;
    float temperature;
    bool present;
};

extern float sharedTempAmbiante, sharedHumiditeAmbiante;
extern bool shtPresent;
extern SensorData sensorTable[MAX_TOTAL_SENSORS+1];
extern uint8_t sensorCount = 0;
extern SemaphoreHandle_t gui_mutex;
extern SemaphoreHandle_t data_mutex;
extern QueueHandle_t QueueHandle;
extern const int QueueElementSize = 15;

// Fonctions
void selectMuxChannel(uint8_t bus, uint8_t channel);
bool isMuxPresent(uint8_t bus);
int findExistingSensor(uint8_t bus, uint8_t mux_channel, bool is_mux);
void displayData();
void readSensors();
void detectSensors(void *pvParameters);

#endif // SENSOR_LIBRARY_H
