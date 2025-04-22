/*  Rui Santos & Sara Santos - Random Nerd Tutorials
    THIS EXAMPLE WAS TESTED WITH THE FOLLOWING HARDWARE:
    1) ESP32-2432S028R 2.8 inch 240×320 also known as the Cheap Yellow Display (CYD): https://makeradvisor.com/tools/cyd-cheap-yellow-display-esp32-2432s028r/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/cyd-lvgl/
    2) REGULAR ESP32 Dev Board + 2.8 inch 240x320 TFT Display: https://makeradvisor.com/tools/2-8-inch-ili9341-tft-240x320/ and https://makeradvisor.com/tools/esp32-dev-board-wi-fi-bluetooth/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/esp32-tft-lvgl/
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

/*  Install the "lvgl" library version 9.2 by kisvegabor to interface with the TFT Display - https://lvgl.io/
    *** IMPORTANT: lv_conf.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE lv_conf.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <lvgl.h>

/*  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI
    *** IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE User_Setup.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <TFT_eSPI.h>

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen - Note: this library doesn't require further configuration
#include <XPT2046_Touchscreen.h>
#include <BLEDevice.h>       // Fonctionnalités BLE (Bluetooth Low Energy)
#include <BLEUtils.h>
#include <BLEServer.h>
#include "ui.h"
#include "Adafruit_SHT31.h"  // Capteur SHT31 (température et humidité)
#include "Adafruit_seesaw.h" // Contrôleur de capteur seesaw (température, humidité, etc.)

#define debug true // Debug

#define SensorSDA 45
#define SensorSCL 48
#define SHT31ADRESS 0x44
#define SEESAWSOILADRESS 0x36
#define SD_CHIPSELECT 10

// Touchscreen pins
#define XPT2046_IRQ  255  // T_IRQ
#define XPT2046_MOSI 4  // T_DIN
#define XPT2046_MISO 15  // T_OUT
#define XPT2046_CLK 47   // T_CLK
#define XPT2046_CS 2    // T_CS

SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

Adafruit_SHT31 sht31 = Adafruit_SHT31(); // Capteur d'humidité/température SHT31
Adafruit_seesaw ss;                     // Capteur seesaw pour la température du sol
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// ------------------------------------------------
// Configuration du BLE (Bluetooth Low Energy)
// ------------------------------------------------
BLEAdvertising *advertising;   // Gestion des données publicitaires BLE
BLEServer *bleServer;          // Serveur BLE
BLEUUID SERVICE_UUID("91bad492-b950-4226-aa2b-4ede9fa42f59"); // UUID pour identifier le service BLE
// ------------------------------------------------
// Configuration des tâches asynchrones
// ------------------------------------------------
SemaphoreHandle_t dataMutex;       // Mutex pour synchroniser l'accès aux données partagées
TaskHandle_t bleTaskHandle;        // Gestionnaire pour la tâche BLE
TaskHandle_t guiTaskHandle;        // Gestionnaire pour la tâche GUI (interface graphique)
TaskHandle_t SDTaskHandle;        // Gestionnaire pour la tâche GUI (interface graphique)
// ------------------------------------------------
// Données partagées des capteurs
// ------------------------------------------------
float sharedTempAmbiante, sharedHumiditeAmbiante, sharedTempSol; // Température et humidité partagées
uint16_t sharedEauSol;                     // Lecture de la capacité du sol
// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// If logging is enabled, it will inform the user about what is happening in the library
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

// Get the Touchscreen data
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
  if(touchscreen.touched()) {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    // Set the coordinates
    data->point.x = x;
    data->point.y = y;

    // Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
    Serial.print("X = ");
    Serial.print(x);
    Serial.print(" | Y = ");
    Serial.print(y);
    Serial.print(" | Pressure = ");
    Serial.print(z);
    Serial.println();
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *main3;
    lv_obj_t *obj0;
    lv_obj_t *table1;
} objects_t;

extern objects_t objects;

// ------------------------------------------------
// Tâches asynchrones pour BLE et GUI
// ------------------------------------------------

/**
 * @brief Fonction pour préparer les données en BLE (Fonction asyncrone)
 * @param temp1, hum1,temp2,hum2
 * @return Rien
 */
void updateAdvertisementData(float temp1, float hum1, float temp2, uint16_t hum2) 
{
    // Création d'un objet de donnée BLE
    BLEAdvertisementData advertisementData;
    const char* dataCStr;  //Variable temporaire pour stocker les donnée BLE en une ligne
    //Convertit les données passées en parametres en string de une ligne
    String data = String(temp1) + "," + String(hum1) + "," + String(temp2) + "," + String(hum2);  // Adding a comma for readability
    dataCStr = data.c_str();

    //Definition des differents parametres pour le message BLE 
    advertisementData.setFlags(0x06);      // Mode générale détectable, BR/EDR Non supporté
    if(debug)
    {
      Serial.println("Updated :");
      Serial.println(dataCStr);
    }
    //advertisementData.setName("ESP32_SerVo"); 
    advertisementData.addData(dataCStr);  // Ajout des données a la prochaine publication 

    // Mise en paramete des données a publier 
    advertising->setAdvertisementData(advertisementData);
    if(debug)
    {
      Serial.println("Updated advertising data with new sensor values:");
      Serial.println(data);
    }
    
}
/**
 * @brief Fonction pour transmettre les données en BLE ansi que SD (Fonction asyncrone)
 * @param void *parameter
 * @return Rien
 */
void updateAdvertisementTask(void *parameter) {
    while (true) {
        //Acces aux données partagés
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
            xSemaphoreGive(dataMutex);
        }

        updateAdvertisementData(sharedTempAmbiante, sharedHumiditeAmbiante, sharedTempSol, sharedEauSol); //Mise a jour des données et mise en attente du BLE
        
        delay(1000); //Délais de communication 
        advertising->start();
        delay(50);
        advertising->stop();
    }
}
/**
 * @brief Fonction pour mettre a jour l'interface ansi que les données des capteurs 
 * @param void *parameter
 * @return Rien
 */
void updateGuiAndDataTask(void *parameter) 
{
    char strVarTAmbiante[20], strVarHAmbiante[20], srtVarTsoil[20], strVarHsoil[20]; //Variables pour avoir une string avec les données des variables
    while (true) {
        //Lecture des capteurs 
        float tempAmbiante = sht31.readTemperature();
        float humiditeAmbiante = sht31.readHumidity();
        float tempSol = ss.getTemp();
        uint16_t eauSol = ss.touchRead(0);

        //Obtention du tableau pour stocker les données 
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) 
        {
            sharedTempAmbiante = tempAmbiante;
            sharedHumiditeAmbiante = humiditeAmbiante;
            sharedTempSol= tempSol;
            sharedEauSol = eauSol;
            xSemaphoreGive(dataMutex);
        }

        // Mise a jour des differents elements de l'interface 
        
        dtostrf(tempAmbiante, 4, 2, strVarTAmbiante);
        dtostrf(humiditeAmbiante, 4, 2, strVarHAmbiante);
        dtostrf(tempSol, 4, 2, srtVarTsoil);
        dtostrf(eauSol, 4, 0, strVarHsoil);

        lv_table_set_cell_value(objects.table1,2,2,strVarTAmbiante);
        lv_table_set_cell_value(objects.table1,2,3,strVarHAmbiante);
        lv_table_set_cell_value(objects.table1,3,2,srtVarTsoil);
        lv_table_set_cell_value(objects.table1,3,3,strVarHsoil);

        
    }
    delay(10); 
}
void setup() {
  String LVGL_Arduino = String("LVGL Library Version: ") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
  Serial.begin(115200);
  Serial.println(LVGL_Arduino);
  // Wait for USB Serial 
  //delay(1000);  // NOTE: Some devices require a delay after Serial.begin() before serial port can be used
  Wire.setPins(SensorSDA,SensorSCL);
  Wire.setClock(10000);
  sht31.begin(SHT31ADRESS); //Adresse du SHT 31 et le begin 
  ss.begin(SEESAWSOILADRESS); //Adresse du SeeSaw Soil Sensor et le begin
  //gslc_InitDebug(&DebugOut);
  
  
  // Start LVGL
  lv_init();
  // Register print function for debugging
  lv_log_register_print_cb(log_print);

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
  touchscreen.setRotation(0);
  
  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
    
  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  // Set the callback function to read Touchscreen input
  lv_indev_set_read_cb(indev, touchscreen_read);

  // Function to draw the GUI (text, buttons and sliders)
  ui_init();
  lv_table_set_cell_value(objects.table1,1,2,"Temperature");
  lv_table_set_cell_value(objects.table1,1,3,"Humidité");
  lv_table_set_cell_value(objects.table1,2,1,"Ambiante");
  lv_table_set_cell_value(objects.table1,2,3,"Sol");
    // Create mutex
  dataMutex = xSemaphoreCreateMutex();

  // Create FreeRTOS tasks
  xTaskCreate(updateAdvertisementTask, "BLE Advertisement Task", 4096, NULL, 1, &bleTaskHandle);
  xTaskCreate(updateGuiAndDataTask, "GUI Update Task", 4096, NULL, 1, &guiTaskHandle);
  
}



void loop() {
      lv_task_handler();  // let the GUI do its work
      lv_tick_inc(5);     // tell LVGL how much time has passed
      delay(5);           // let this time pass
}
