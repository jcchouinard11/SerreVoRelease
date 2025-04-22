#ifndef AFFICHAGE_H
#define AFFICHAGE_H

#include "utils.h"
#include <lvgl.h>
#include "ui.h"
// Fonctions de déboggage et d'affichage
void log_print(lv_log_level_t level, const char * buf);
void updateArcs();
void Task_Screen_Update();
void Task_LVGL(void *pvParameters);
void Task_Tick(void *pvParameters);
void draw_event_cb(lv_event_t * e);

#endif
