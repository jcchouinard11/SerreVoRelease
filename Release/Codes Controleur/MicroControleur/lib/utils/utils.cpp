#include "utils.h"

// Définition de l'objet d'affichage (défini dans ui.h)
objects_t objects;

Preferences prefs;  // Create an instance

// Définition des tableaux de pins I2C
uint8_t sdapin[NUM_BUSES] = {46, 10, 12, 14};
uint8_t sclpin[NUM_BUSES] = {3, 9, 11, 13};

// Création du tableau de bus I2C (FlexWire)
FlexWire i2c_buses[NUM_BUSES] = { {sdapin[0], sclpin[0]},
                                  {sdapin[1], sclpin[1]},
                                  {sdapin[2], sclpin[2]},
                                  {sdapin[3], sclpin[3]}};

// Capteurs
Adafruit_seesaw* sensors[MAX_TOTAL_SENSORS];
Adafruit_SHT31 sht31 = Adafruit_SHT31(&i2c_buses[0]);

// Variables de données capteur
float sharedTempAmbiante = 0;
float sharedHumiditeAmbiante = 0;
bool shtPresent = false;
SensorData sensorTable[MAX_TOTAL_SENSORS];

uint8_t sensorCount = 0;

// Sémaphores et Queue
SemaphoreHandle_t gui_mutex;
SemaphoreHandle_t data_mutex;
QueueHandle_t QueueHandle;

// Touchscreen
 SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
 
// Buffers LVGL
EXT_RAM_ATTR uint32_t draw_buf1[DRAW_BUF_SIZE/2];
EXT_RAM_ATTR uint32_t draw_buf2[DRAW_BUF_SIZE/2];

// Autres variables globales
int _arrow_angle = 0;
unsigned long lastTickMillis = 0;
int x, y, z;

float result=0;
uint32_t current_delay =0;
uint32_t veille_delay=10000;
uint32_t update_local_delay =2000;
int update_BLE_delay =10000;

void getConfig(){
    prefs.begin("my_settings", false);  // Namespace, read/write mode
    lv_roller_set_selected(objects.refresh_delay_config,prefs.getInt("refresh", 0),LV_ANIM_ON);
    lv_roller_set_selected(objects.veille_delay_config,prefs.getInt("veille", 0),LV_ANIM_ON);
    lv_roller_set_selected(objects.local_update_config_1,prefs.getInt("local", 0),LV_ANIM_ON);
    lv_roller_set_selected(objects.ble_update_config_2,prefs.getInt("ble", 0),LV_ANIM_ON);
    prefs.end();
}
void putConfig(){
    prefs.begin("my_settings", false);  // Namespace, read/write mode
    prefs.putInt("refresh", lv_roller_get_selected(objects.refresh_delay_config));
    prefs.putInt("veille", lv_roller_get_selected(objects.veille_delay_config));
    prefs.putInt("local", lv_roller_get_selected(objects.local_update_config_1));
    prefs.putInt("ble", lv_roller_get_selected(objects.ble_update_config_2));
    prefs.end();
}