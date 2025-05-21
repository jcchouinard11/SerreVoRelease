#include "affichage.h"
#include "ui.h"
#include "utils.h"
#include "capteurs.h"

// Variables temporaires pour stocker les réglages de délais LVGL
char current_delay_setting[10];
char current_veille_setting[10];
char current_update_BLE_delay_setting[10];
char current_update_local_delay_setting[10]; 
TaskHandle_t affichagehandle;
// Fonction de log pour LVGL (redirige vers le port série)
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

// Fonction appelée régulièrement pour mettre à jour les arcs de données sur l'écran
void updateArcs() {
    static uint32_t lastUpdate = 0;
    static uint8_t currentSensor = 0;
    static bool reset = true;

    // Lecture des réglages depuis les rollers de l'interface
    lv_roller_get_selected_str(objects.refresh_delay_config, current_delay_setting, 4);
    lv_roller_get_selected_str(objects.veille_delay_config, current_veille_setting, 4);
    lv_roller_get_selected_str(objects.local_update_config_1, current_update_local_delay_setting, 4);
    lv_roller_get_selected_str(objects.ble_update_config_2, current_update_BLE_delay_setting, 4);

    // Conversion des réglages en valeurs numériques
    update_BLE_delay = atoi(current_update_BLE_delay_setting) * 60000;
    update_local_delay = atoi(current_update_local_delay_setting) * 1000;
    veille_delay = atoi(current_veille_setting) * 1000;
    current_delay = (millis() - lastUpdate);

    // Mutex requis pour l'affichage LVGL
    if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) {
        lv_bar_set_value(objects.update_bar, current_delay, LV_ANIM_OFF);
        lv_bar_set_range(objects.update_bar, 0, atoi(current_delay_setting) * 1000);

        // Si assez de temps s'est écoulé, on change de capteur affiché
        if (millis() - lastUpdate >= atoi(current_delay_setting) * 1000 || reset) {
            lastUpdate = millis();

            if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) {
                do {
                    currentSensor++;
                } while (currentSensor < MAX_TOTAL_SENSORS && !sensorTable[currentSensor].present);

                if (sensorTable[0].present && reset) {
                    currentSensor = 0;
                }
                reset = false;

                if (currentSensor < MAX_TOTAL_SENSORS) {
                    static char humidityStr[10];
                    static char temperatureStr[10];
                    static char nameStr[10];

                    // Nom du capteur (bus + canal mux ou non)
                    if (sensorTable[currentSensor].is_mux) {
                        sprintf(nameStr, "%d-Mux-%d", sensorTable[currentSensor].bus + 1, sensorTable[currentSensor].mux_channel + 1);
                    } else {
                        sprintf(nameStr, "%d", sensorTable[currentSensor].bus + 1);
                    }

                    // Format des données
                    dtostrf(sensorTable[currentSensor].humidity, 4, 0, humidityStr);
                    dtostrf(sensorTable[currentSensor].temperature, 4, 0, temperatureStr);

                    // Mise à jour des arcs et labels
                    lv_arc_set_value(objects.data_c_1, (int)sensorTable[currentSensor].temperature);
                    lv_arc_set_value(objects.data_h_1, sensorTable[currentSensor].humidity);
                    lv_label_set_text_fmt(objects.home_ch_label, nameStr);
                    lv_label_set_text_fmt(objects.data_c_text_1, temperatureStr);
                    lv_label_set_text_fmt(objects.data_h_text_1, humidityStr);
                } else {
                    currentSensor = 0;
                    reset = true;
                }
                if(lv_obj_get_style_text_opa(objects.obj8,LV_PART_MAIN | LV_STATE_DEFAULT ))
                {
                    lv_obj_set_style_text_opa(objects.obj8, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    }
                xSemaphoreGive(data_mutex);
            }
        }

        xSemaphoreGive(gui_mutex);
    }
}

// Mise à jour complète du tableau à l'écran avec toutes les données
void Task_Screen_Update() {
    static uint32_t lastUpdate = 0;
    static char label_char[100];
    static char strVarTAmbiante[20], strVarHAmbiante[20], srtVarTsoil[20], strVarHsoil[20], strVarTAmbiante2[20], strVarHAmbiante2[20];
    static char nameStr[10];
    static char humidityStr[10];
    static char temperatureStr[10];
    static int currentindex = 0;

    if (millis() - lastUpdate >= update_local_delay) {
        lastUpdate = millis();

        if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) {
            if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) {
                readSensors(); // Lecture de tous les capteurs
                currentindex = 0;
                lv_table_set_row_count(objects.table1, sensorCount + 1);

                for (uint8_t i = 0; i < MAX_TOTAL_SENSORS; i++) {
                    if (sensorTable[i].present) {
                        dtostrf(sensorTable[i].humidity, 4, 0, humidityStr);
                        dtostrf(sensorTable[i].temperature, 4, 1, temperatureStr);
                        strcat(humidityStr, "%");
                        strcat(temperatureStr, "°C");

                        if (sensorTable[i].is_mux) {
                            sprintf(nameStr, "%d-Mux-%d", sensorTable[i].bus + 1, sensorTable[i].mux_channel + 1);
                        } else {
                            sprintf(nameStr, "%d", sensorTable[i].bus + 1);
                        }

                        lv_table_set_cell_value(objects.table1, currentindex + 2, 0, nameStr);
                        lv_table_set_cell_value(objects.table1, currentindex + 2, 1, humidityStr);
                        lv_table_set_cell_value(objects.table1, currentindex + 2, 2, temperatureStr);
                        currentindex++;
                    }
                }

                // Capteur ambiant
                if (shtPresent) {
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
                } else {
                    lv_table_set_cell_value(objects.table1, 1, 1, "N/C");
                    lv_table_set_cell_value(objects.table1, 1, 2, "N/C");
                }

                xSemaphoreGive(data_mutex);
            }

            xSemaphoreGive(gui_mutex);
        }
    }
}

// Tâche qui exécute la boucle principale de LVGL
void Task_LVGL(void *pvParameters) {
    printf("\n[LVGL] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));

    while (1) {
        if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) {
            lv_task_handler(); // Rafraîchit les objets LVGL
            ui_tick();         // Met à jour les animations ou timers personnalisés
            xSemaphoreGive(gui_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // Attente minimale
    }
}

// Tâche qui génère les ticks temporels pour le moteur LVGL
void Task_Tick(void *pvParameters) {
    printf("\n[LVGLTICK] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));

    while (1) {
        lv_tick_inc(5);       // Avance le temps de 5 ms
        vTaskDelay(5);        // Répète toutes les 5 ms
    }
}

// Callback personnalisé pour la coloration et l’alignement des cellules dans la table LVGL
void draw_event_cb(lv_event_t * e) {
    lv_draw_task_t * draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t * base_dsc = (lv_draw_dsc_base_t*) draw_task->draw_dsc;

    if (base_dsc->part == LV_PART_ITEMS) {
        uint32_t row = base_dsc->id1;
        uint32_t col = base_dsc->id2;

        if (row == 0) {
            // En-tête : centrage et fond vert pâle
            lv_draw_label_dsc_t * label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
            if (label_draw_dsc) label_draw_dsc->align = LV_TEXT_ALIGN_CENTER;

            lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
            if (fill_draw_dsc) {
                fill_draw_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_GREEN), fill_draw_dsc->color, LV_OPA_10);
                fill_draw_dsc->opa = LV_OPA_COVER;
            }
        } else if (col == 0) {
            // Colonne d'index : alignement à droite
            lv_draw_label_dsc_t * label_draw_dsc = lv_draw_task_get_label_dsc(draw_task);
            if (label_draw_dsc) label_draw_dsc->align = LV_TEXT_ALIGN_RIGHT;
        }

        // Alternance de couleurs sur lignes paires
        if ((row != 0 && row % 2) == 0 && col != 0) {
            lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
            if (fill_draw_dsc) {
                fill_draw_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_GREY), fill_draw_dsc->color, LV_OPA_10);
                fill_draw_dsc->opa = LV_OPA_COVER;
            }
        }

        // Teinte légère sur les IDs des lignes
        if (col == 0 && row != 0) {
            lv_draw_fill_dsc_t * fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
            if (fill_draw_dsc) {
                fill_draw_dsc->color = lv_color_mix(lv_palette_main(LV_PALETTE_TEAL), fill_draw_dsc->color, LV_OPA_10);
                fill_draw_dsc->opa = LV_OPA_COVER;
            }
        }
    }
}
