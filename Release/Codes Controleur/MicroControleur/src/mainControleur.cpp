/**
 * @file        mainControleur.cpp
 * @brief       Programme principal pour la gestion des capteurs et de l'affichage.
 * 
 * @details     Ce programme utilise plusieurs capteurs connectés via I2C et BLE pour surveiller
 *              l'humidité et la température. Il affiche les données sur un écran via LVGL et gère 
 *              la communication Bluetooth.
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
 #include "driver/rtc_io.h"

 #define uS_TO_S_FACTOR 1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  10           // Time ESP32 will go to sleep (in seconds)


 // -- SETUP PRINCIPAL --
 void setup() 
 {
   
    
     Serial.begin(115200);
 
     // Init des 4 bus I2C utilisés pour les capteurs
     i2c_buses[0].begin();
     i2c_buses[1].begin();
     i2c_buses[2].begin();
     i2c_buses[3].begin();
 
     // Init du capteur d'air SHT31
     sht31.begin(SHT31ADRESS);
    
     // Mutex pour synchronisation d'accès mémoire
     gui_mutex = xSemaphoreCreateMutex();
     data_mutex = xSemaphoreCreateMutex();
      setupBLE();
      esp_sleep_wakeup_cause_t wakeup_reason;
     wakeup_reason = esp_sleep_get_wakeup_cause();
  
     switch (wakeup_reason) {
       case ESP_SLEEP_WAKEUP_EXT0:     printf("Wakeup caused by external signal using RTC_IO"); break;
       case ESP_SLEEP_WAKEUP_EXT1:     printf("Wakeup caused by external signal using RTC_CNTL"); break;
       case ESP_SLEEP_WAKEUP_TIMER:
         //getConfig();
            singleDetectSensorWithRead();
            readSensors();
            singleTask_BLE();
           esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, LOW); // Wake up when the button is pressed
           esp_sleep_enable_timer_wakeup(/*update_BLE_delay/1000*/60*uS_TO_S_FACTOR);
           esp_deep_sleep_start();  
           break;
       case ESP_SLEEP_WAKEUP_TOUCHPAD: printf("Wakeup caused by touchpad"); break;
       case ESP_SLEEP_WAKEUP_ULP:      printf("Wakeup caused by ULP program"); break;
       default:                        

       printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
     }  
     // Init de la librairie LVGL pour affichage graphique
     lv_init();
 
     // Init du Bluetooth Low Energy
     pinMode(42, OUTPUT);
     pinMode(40, OUTPUT);
     digitalWrite(42, HIGH);
     pinMode(17, OUTPUT);
     digitalWrite(17, LOW);
 
     // Configuration GPIO (alimentation/écran)

   //pinMode(2, INPUT_PULLUP); // GPIO pour le bouton de réveil
     // Callback de log pour LVGL
      lv_log_register_print_cb(log_print);
     
   
     // Init de l'écran tactile
     touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
     touchscreen.begin(touchscreenSPI);
     touchscreen.setRotation(0);
 
     // Initialisation de l'afficheur via TFT_eSPI et buffers LVGL
     lv_display_t * disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf1, sizeof(draw_buf1));
     lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
     lv_display_set_buffers(disp, draw_buf1, draw_buf2, sizeof(draw_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
 
     // Configuration de l'entrée tactile pour LVGL
     static lv_indev_t * indev = lv_indev_create();
     lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
     lv_indev_set_read_cb(indev, touchscreen_read);
        // First we configure the wake up source We set our ESP32 to wake up every 5 seconds
    
     //rtc_gpio_pullup_en(GPIO_NUM_2);
    //rtc_gpio_pulldown_dis(GPIO_NUM_2);
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_2, LOW); // Wake up when the button is pressed 
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP*uS_TO_S_FACTOR);
  
     // Chargement de l'interface graphique et de la table
     ui_init();
     lv_table_set_cell_value(objects.table1, 0, 0, "ID");
     lv_table_set_cell_value(objects.table1, 0, 1, "Humidite %");
     lv_table_set_cell_value(objects.table1, 0, 2, "Temperature");
     lv_table_set_column_width(objects.table1, 0, 60);
     lv_table_set_column_width(objects.table1, 1, 100);
     lv_table_set_column_width(objects.table1, 2, 130);
     lv_obj_add_event_cb(objects.table1, draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
     lv_obj_add_flag(objects.table1, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS); 
 
     // Chargement des configurations sauvegardées
 
     // Création des tâches principales (FreeRTOS)
     xTaskCreatePinnedToCore(Task_LVGL, "Task_LVGL", 4096*10, NULL, 2, NULL, 1);
     xTaskCreatePinnedToCore(Task_Tick, "Task_Tick", 4096, NULL, 1, NULL, 1);
     singleDetectSensorWithRead();
     xTaskCreatePinnedToCore(detectSensors, "Task_Sensor", 4096, NULL, 4, NULL, 0);
     
     //readSensors();
     xTaskCreatePinnedToCore(Task_BLE, "Task_BLE", 4096*12, NULL, 5, NULL, 0);
     
     
     //delay(15000);
     getConfig();
      
 }
 
 // -- BOUCLE PRINCIPALE (exécutée en parallèle des tâches FreeRTOS) --
 void loop() 
 {
  while(stopFlag){
    // Met à jour l'affichage local (valeurs actuelles)
    updateArcs();

    // Délai pour laisser LVGL et BLE rouler (non-bloquant si multitâche bien utilisé)
    delay(200);

    // Rafraîchit l'interface manuellement (sauf si géré par thread indépendant)
    Task_Screen_Update();
    
  }

  
  esp_sleep_enable_timer_wakeup(update_BLE_delay/1000*uS_TO_S_FACTOR);
  vTaskDelete(BLEtaskHandle);
  lv_scr_load(objects.veille);
  delay(1000);
  //lv_indev_wait_release(lv_indev_get_act());
  singleTask_BLE(); 
  esp_deep_sleep_start();
 }
 