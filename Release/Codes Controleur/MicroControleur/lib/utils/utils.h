#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <FlexWire.h>
#include <lvgl.h>
#include "Adafruit_seesaw.h"
#include "Adafruit_SHT31.h"  // 
#include <driver/temp_sensor.h>
#include <XPT2046_Touchscreen.h>
#include <TFT_eSPI.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include "ui.h"  // Pour la définition de objects_t
#include <Preferences.h>
#include <EEPROM.h>
// -------------------------
// Constantes et définitions
// -------------------------
#define NUM_BUSES 4
#define NUM_MUX 4
#define SHTBUS (NUM_BUSES)
#define MUX_ADDR 0x70
#define SHT31ADRESS 0x44
#define MAX_SENSORS_PER_MUX 4
#define MAX_TOTAL_SENSORS (NUM_MUX * MAX_SENSORS_PER_MUX)+NUM_MUX

// Pins du touchscreen
#define XPT2046_IRQ  255  // T_IRQ
#define XPT2046_MOSI 4    // T_DIN
#define XPT2046_MISO 15   // T_OUT
#define XPT2046_CLK 47    // T_CLK
#define XPT2046_CS 16     // T_CS

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
#define ARC_UPDATE_INTERVAL 2000





// Taille d'un élément de la Queue
const int QueueElementSize = 15;

// -------------------------
// Structure des données capteur
// -------------------------
struct SensorData {
    uint8_t index;
    uint8_t bus;
    uint8_t mux_channel;
    bool is_mux;
    int humidity;
    float temperature;
    bool present;
};

// -------------------------
// Déclarations externes des variables globales
// -------------------------
extern objects_t objects;
extern Preferences prefs;  // Create an instance
extern uint8_t sdapin[NUM_BUSES];
extern uint8_t sclpin[NUM_BUSES];
extern FlexWire i2c_buses[NUM_BUSES];

extern Adafruit_seesaw* sensors[MAX_TOTAL_SENSORS];
extern Adafruit_SHT31 sht31; // Capteur SHT31

extern float sharedTempAmbiante;
extern float sharedHumiditeAmbiante;
extern bool shtPresent;
extern SensorData sensorTable[MAX_TOTAL_SENSORS];
extern uint8_t sensorCount;



extern SemaphoreHandle_t gui_mutex;
extern SemaphoreHandle_t data_mutex;
extern QueueHandle_t QueueHandle;

extern SPIClass touchscreenSPI;
extern XPT2046_Touchscreen touchscreen;

extern EXT_RAM_ATTR uint32_t draw_buf1[DRAW_BUF_SIZE/2];
extern EXT_RAM_ATTR uint32_t draw_buf2[DRAW_BUF_SIZE/2];

extern int _arrow_angle;
extern unsigned long lastTickMillis;
extern int x, y, z;

extern float result;
extern uint32_t current_delay;
extern uint32_t veille_delay;
extern uint32_t update_local_delay;
extern int update_BLE_delay;

extern char current_delay_setting[10];
extern char current_veille_setting[10];
extern char current_update_BLE_delay_setting[10];
extern char current_update_local_delay_setting[10]; 
void putConfig();
void getConfig();
#endif
