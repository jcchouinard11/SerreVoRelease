#ifndef SENSOR_MONITORING_H
#define SENSOR_MONITORING_H

#include <FlexWire.h>
#include <lvgl.h>
#include <Arduino.h>
#include "Adafruit_seesaw.h"
#include "Adafruit_SHT31.h"  // Capteur SHT31 (température et humidité)

#include <TFT_eSPI.h>
#include <BLEDevice.h>       // Fonctionnalités BLE (Bluetooth Low Energy)
#include <BLEUtils.h>
#include <BLEServer.h>

// Définitions de configurations et de broches
#define NUM_BUSES 5
#define NUM_MUX 4
#define SHTBUS (NUM_BUSES)
#define MUX_ADDR 0x70
#define SHT31ADRESS 0x44
#define MAX_SENSORS_PER_MUX 4
#define MAX_TOTAL_SENSORS (NUM_MUX * MAX_SENSORS_PER_MUX)

// Broches des bus I2C
#define XPT2046_IRQ  255  // T_IRQ
#define XPT2046_MOSI 4   // T_DIN
#define XPT2046_MISO 15  // T_OUT
#define XPT2046_CLK 47   // T_CLK
#define XPT2046_CS 16    // T_CS


extern uint8_t sdapin[NUM_BUSES] = { 45, 1,38,42,40 };
extern uint8_t sclpin[NUM_BUSES] = { 48, 2,37,39,41 };
// Array of Flexwire instances
extern FlexWire i2c_buses[NUM_BUSES] = { {sdapin[0], sclpin[0]}, {sdapin[1], sclpin[1]},{sdapin[2], sclpin[2]},{sdapin[3], sclpin[3]},{sdapin[4], sclpin[4]} }; 

extern Adafruit_seesaw* sensors[MAX_TOTAL_SENSORS];
extern Adafruit_SHT31 sht31 = Adafruit_SHT31(&i2c_buses[4]); // Capteur d'humidité/température SHT31

extern int QueueElementSize = 15;
extern SemaphoreHandle_t gui_mutex;
extern SemaphoreHandle_t data_mutex;
extern QueueHandle_t QueueHandle;
extern uint8_t sensorCount;
extern SensorData sensorTable[MAX_TOTAL_SENSORS+1];
float sharedTempAmbiante, sharedHumiditeAmbiante;
struct SensorData {
    uint8_t index;
    uint8_t bus;
    uint8_t mux_channel;
    bool is_mux;
    int humidity;
    float temperature;
    bool present;
};


// Déclaration des fonctions
void selectMuxChannel(uint8_t bus, uint8_t channel);
bool isMuxPresent(uint8_t bus);
int findExistingSensor(uint8_t bus, uint8_t mux_channel, bool is_mux);
void displayData();
void readSensors();
void detectSensors(void *pvParameters);
void configureBLE();
void updateBLE();
void setupUI();

#endif