/**
 * @file        mainControleur.cpp
 * @brief       Programme principal pour la gestion des capteurs et de l'affichage.
 * 
 * @details     Ce programme utilise plusieurs capteurs connectés via I2C et BLE pour surveiller
 *              l'humidité et la température. Il affiche les données sur un écran via LVGL et gère 
 *              la communication Bluetooth.
 * 
 * @author      Jean-Christophe Chouinard
 * @version     1.1
 * @date        2025-02-24
 * 
 * @dependencies
 *              - Adafruit Seesaw, SHT31 pour la gestion des capteurs.
 *              - LVGL pour l'affichage.
 *              - ArduinoBLE pour la communication Bluetooth.
 * 
 */
#include <Arduino.h>
#include <FlexWire.h>
#include <lvgl.h>
#include "Adafruit_seesaw.h"
#include "Adafruit_SHT31.h"
#include <XPT2046_Touchscreen.h>
#include "ui.h"
#include <TFT_eSPI.h>
#include "driver/temp_sensor.h"
#include "BLE.h"
#include "sdlib.h"
#include "utils.h"
#include "capteurs.h"
#include "affichage.h"
#include "touchscreen.h"
#include <MycilaTaskMonitor.h>
//SPIClass sdSPI = SPIClass(HSPI);
void setup() 
{
    Serial.begin(115200);
    
    i2c_buses[0].begin();
    i2c_buses[1].begin();
    i2c_buses[2].begin();
    i2c_buses[3].begin();
    sht31.begin(SHT31ADRESS); 
/*     Mycila::TaskMonitor.addTask("Task_Tick");
    Mycila::TaskMonitor.addTask("Task_Screen_Update");
    Mycila::TaskMonitor.addTask("Task_BLE");
    Mycila::TaskMonitor.addTask("detectSensors");  */
    gui_mutex = xSemaphoreCreateMutex();
    data_mutex = xSemaphoreCreateMutex();
    lv_init();
    setupBLE();
    pinMode(42, OUTPUT);
    pinMode(40,OUTPUT);
    digitalWrite(42, HIGH);
    pinMode(17, OUTPUT);
    digitalWrite(17, LOW);
    //initTempSensor();
    // Enregistrement de la fonction de log pour LVGL
    lv_log_register_print_cb(log_print);
    


    

    
     // Initialisation du touchscreen
      touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(0);  
    
    //printf("Here");
    // Initialisation de l'écran TFT via TFT_eSPI et LVGL
         lv_display_t * disp;
    disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf1, sizeof(draw_buf1));
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
    lv_display_set_buffers(disp, draw_buf1, draw_buf2, sizeof(draw_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);   
 
     //printf("passeed");
    //sdSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI,40);
    //SD.begin(40,sdSPI,4000000,"/sd",1); 
    
     // Initialisation de l’appareil de saisie (touchscreen) pour LVGL
      static lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchscreen_read);  
 
      // Création de l'interface graphique (ui)
    ui_init();
    lv_table_set_cell_value(objects.table1, 0, 0, "ID");
    lv_table_set_cell_value(objects.table1, 0, 1, "Humidite %");
    lv_table_set_cell_value(objects.table1, 0, 2, "Temperature");
    lv_table_set_column_width(objects.table1, 0, 60);
    lv_table_set_column_width(objects.table1, 1, 100);
    lv_table_set_column_width(objects.table1, 2, 130); 
    lv_obj_add_event_cb(objects.table1, draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(objects.table1, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);  
    getConfig();
      

    //SD.begin(); 



        
     

    
  

    // Création des tâches FreeRTOS
    xTaskCreatePinnedToCore(Task_LVGL, "Task_LVGL", 4096*10, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(Task_Tick, "Task_Tick", 4096, NULL, 1, NULL, 1);
    //xTaskCreatePinnedToCore(Task_Screen_Update, "Task_Screen_Update", 8192, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(Task_BLE, "Task_BLE", 4096*12, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(detectSensors, "Task_Sensor", 4096, NULL, 4, NULL, 0);    
}

void loop() 
{
    /*   if (!SD.begin(40,touchscreenSPI)) {
    printf("Card Mount Failed");
    return;
    }
    File file = SD.open(filePath, FILE_WRITE);
        if (file) {
            file.println("Horodatage,Voltage_Batterie,Temp_Sol1,Humidité_Sol1,Temp_Sol2,Humidité_Sol2,Temp_Sol3,Humidité_Sol3,Temp_Air,Humidité_Air,Vitesse_Pompe,État_Pompe,État_Ventilateur,État_Fenêtre");
            file.close();
            Serial.println("CSV header created");
        }
        listDir(SD, "/", 0);
      createDir(SD, "/mydir");
      listDir(SD, "/", 0);
      removeDir(SD, "/mydir");
      listDir(SD, "/", 2);
    listDir(SD, "/", 0);
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    printf("SD Card Size: %lluMB\n", cardSize);
    delay(5000);  */
           updateArcs(); 
      delay(200);    
      Task_Screen_Update();   
      //uxTaskGetStackHighWaterMark(NULL);
    //delay(2000);  
/*     if (millis() - lastUpdate >=  5000) 
    {
        lastUpdate = millis();
        putConfig();
    } */
      
        //detectSensors(); 
}
