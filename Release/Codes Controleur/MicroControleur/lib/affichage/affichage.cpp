#include "affichage.h"
#include "ui.h"
#include "utils.h"
#include "capteurs.h"
char current_delay_setting[10];
char current_veille_setting[10];
char current_update_BLE_delay_setting[10];
char current_update_local_delay_setting[10]; 
void log_print(lv_log_level_t level, const char * buf)   
{
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

void updateArcs() {
    static uint32_t lastUpdate = 0;
    static uint8_t currentSensor = 0;
    static bool reset = true;
    lv_roller_get_selected_str(objects.refresh_delay_config,current_delay_setting,4);
    //printf("%d\n",current_delay);
    lv_roller_get_selected_str(objects.veille_delay_config,current_veille_setting,4);
    lv_roller_get_selected_str(objects.local_update_config_1,current_update_local_delay_setting,4);
    lv_roller_get_selected_str(objects.ble_update_config_2,current_update_BLE_delay_setting,4);


        update_BLE_delay = ((atoi(current_update_BLE_delay_setting)*60000));
        update_local_delay =((atoi(current_update_local_delay_setting)*1000)); 
        veille_delay=(atoi(current_veille_setting)*1000);
    current_delay =(millis() - lastUpdate);

    if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) 
    {

        

        lv_bar_set_value(objects.update_bar,current_delay,LV_ANIM_OFF);
        lv_bar_set_range(objects.update_bar,0,(atoi(current_delay_setting)*1000));
        if (millis() - lastUpdate >=  (atoi(current_delay_setting)*1000) ) 
        {
            lastUpdate = millis();
        if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) 
        {   
            do
            {
              currentSensor++;
            }
            while(currentSensor < MAX_TOTAL_SENSORS && sensorTable[currentSensor].present == false);

            if(sensorTable[0].present && reset)
            {
                currentSensor = 0;
                reset = false;
            }

            if (currentSensor < MAX_TOTAL_SENSORS) {
                static char humidityStr[10];
                static char temperatureStr[10];
                static char nameStr[10];
                if (sensorTable[currentSensor].is_mux)    
                {
                    sprintf(nameStr, "%d-Mux-%d", sensorTable[currentSensor].bus+1, sensorTable[currentSensor].mux_channel+1);
                } else    
                {
                    sprintf(nameStr, "%d", sensorTable[currentSensor].bus+1);
                } 
                dtostrf(sensorTable[currentSensor].humidity, 4, 0, humidityStr);
                dtostrf(sensorTable[currentSensor].temperature, 4, 0, temperatureStr);
                
                int arc_value_humidity = sensorTable[currentSensor].humidity;
                int arc_value_temperature = (int)sensorTable[currentSensor].temperature;
                
                lv_arc_set_value(objects.data_c_1, arc_value_temperature);
                lv_arc_set_value(objects.data_h_1, arc_value_humidity);

                //lv_label_set_text_fmt(objects.home_id_label, "%s", currentSensor);
                lv_label_set_text_fmt(objects.home_ch_label,nameStr);

                lv_label_set_text_fmt(objects.data_c_text_1,temperatureStr);
                lv_label_set_text_fmt(objects.data_h_text_1,humidityStr);
            }
            else
            {
              currentSensor = 0;
              reset = true;
            }

            
          xSemaphoreGive(data_mutex);
      }
    
    }
    xSemaphoreGive(gui_mutex);
}
}

void Task_Screen_Update() 
{
  static uint32_t lastUpdate = 0;
    static char label_char[100];
    static char strVarTAmbiante[20], strVarHAmbiante[20], srtVarTsoil[20], strVarHsoil[20], strVarTAmbiante2[20], strVarHAmbiante2[20];
    static char nameStr[10];
    static char humidityStr[10];
    static char temperatureStr[10];
    static int currentindex =0;
    if (millis() - lastUpdate >=  update_local_delay ) 
    {
        lastUpdate = millis();
                if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) 
                {
                    
                    
                    if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) 
                    {
                      readSensors();
                      currentindex=0;
                      lv_table_set_row_count(objects.table1, sensorCount+1);
                      //printf("\n[Task_Screen_Update] %d", sensorCount);
                      for (uint8_t i = 0; i < MAX_TOTAL_SENSORS; i++) 
                      {
                        
                        if(sensorTable[i].present)
                        {
                          
                          dtostrf(sensorTable[i].humidity, 4, 0, humidityStr);
                          dtostrf(sensorTable[i].temperature, 4, 1, temperatureStr);
                          strcat(humidityStr, "%");
                          strcat(temperatureStr, "°C");
                          if (sensorTable[i].is_mux)    
                          {
                              sprintf(nameStr, "%d-Mux-%d", sensorTable[i].bus+1, sensorTable[i].mux_channel+1);
                          } else    
                          {
                              sprintf(nameStr, "%d", sensorTable[i].bus+1);
                          }
                          lv_table_set_cell_value(objects.table1, currentindex+2, 0, nameStr);
                          lv_table_set_cell_value(objects.table1, currentindex+2, 1, humidityStr);
                          lv_table_set_cell_value(objects.table1, currentindex+2, 2, temperatureStr);
                          currentindex++;
                      }
                      
                    }
                    
                  


                    if (shtPresent)    
                    {   
                        dtostrf(sharedHumiditeAmbiante, 4, 0, humidityStr);
                        dtostrf(sharedTempAmbiante, 4, 1, temperatureStr);
                        strcat(humidityStr, "%H");
                        strcat(temperatureStr, "°C");
                        lv_bar_set_value(objects.bar_amb_hum, sharedHumiditeAmbiante, LV_ANIM_ON);
                        lv_bar_set_value(objects.bar_amb_temp, sharedTempAmbiante, LV_ANIM_ON);
                        lv_label_set_text_fmt(objects.data_h_amb_text, humidityStr);
                        lv_label_set_text_fmt(objects.data_t_amb_text, temperatureStr);
                        lv_table_set_cell_value(objects.table1, 1, 0, "AMB");
                        lv_table_set_cell_value(objects.table1, 1, 1, humidityStr);
                        lv_table_set_cell_value(objects.table1, 1, 2, temperatureStr);
                    }
                    else
                    {  
                        lv_table_set_cell_value(objects.table1, 1, 1, "N/C");
                        lv_table_set_cell_value(objects.table1, 1, 2, "N/C");
                    }
                    
                    xSemaphoreGive(data_mutex);               
                    
                }
                xSemaphoreGive(gui_mutex);
              }
            
            }
                    
        
        
    }


void Task_LVGL(void *pvParameters) 
{
  printf("\n[LVGL] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
    
  while (1) 
  {
/*       unsigned int tickPeriod = millis() - lastTickMillis;
      lv_tick_inc(tickPeriod);
      lastTickMillis = millis();   */
      if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) 
      { 
          
          lv_task_handler();
          ui_tick();
          xSemaphoreGive(gui_mutex);
      }
      vTaskDelay(pdMS_TO_TICKS(1));
  }
}
void Task_Tick(void *pvParameters) 
{
  // Start LVGL
  printf("\n[LVGLTICK] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
    
    
  while (1) 
  {
    lv_tick_inc(5);
    vTaskDelay(5);  // Adjust as needed
    // Delay to control LVGL's refresh rate
    
  }
}

 void draw_event_cb(lv_event_t * e) {
  lv_draw_task_t * draw_task = lv_event_get_draw_task(e);
  lv_draw_dsc_base_t * base_dsc = (lv_draw_dsc_base_t*) draw_task->draw_dsc;
  if(base_dsc->part == LV_PART_ITEMS) {
    uint32_t row = base_dsc->id1;
    uint32_t col = base_dsc->id2;

    if(row == 0) {
      lv_draw_label_dsc_t * label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
      if(label_draw_dsc) {
        label_draw_dsc->align = LV_TEXT_ALIGN_CENTER;
      }
      lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
      if(fill_draw_dsc) {
        fill_draw_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_TEAL), fill_draw_dsc->color, LV_OPA_20);
        fill_draw_dsc->opa = LV_OPA_COVER;
      }
    }
    else if(col == 0) {
      lv_draw_label_dsc_t * label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
      if(label_draw_dsc) {
        label_draw_dsc->align = LV_TEXT_ALIGN_RIGHT;
      }
    }

    if((row != 0 && row % 2) == 0 && col != 0) {
      lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
      if(fill_draw_dsc) {
        fill_draw_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_GREY), fill_draw_dsc->color, LV_OPA_10);
        fill_draw_dsc->opa = LV_OPA_COVER;
      }
    }
    if(col == 0 && row != 0) {
      lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
      if(fill_draw_dsc) {
        fill_draw_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_TEAL), fill_draw_dsc->color, LV_OPA_10);
        fill_draw_dsc->opa = LV_OPA_COVER;
      }
    }
    if(col == 0 && row == 0) {
      lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
      if(fill_draw_dsc) {
        fill_draw_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_GREEN), fill_draw_dsc->color, LV_OPA_10);
        fill_draw_dsc->opa = LV_OPA_COVER;
      }
    }
  }
}
 