#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *home;
    lv_obj_t *data;
    lv_obj_t *buttons;
    lv_obj_t *btn_home;
    lv_obj_t *btn_prev_sensor;
    lv_obj_t *btn_next_sensor;
    lv_obj_t *btn_play_pause;
    lv_obj_t *btn_settings;
    lv_obj_t *obj0;
    lv_obj_t *temperature_panel;
    lv_obj_t *data_h_text_1;
    lv_obj_t *data_c_text_1;
    lv_obj_t *data_c_1;
    lv_obj_t *data_h_1;
    lv_obj_t *tag_temp;
    lv_obj_t *tag_hum;
    lv_obj_t *capteur_info;
    lv_obj_t *cont_ambiant;
    lv_obj_t *data_t_amb_text;
    lv_obj_t *data_h_amb_text;
    lv_obj_t *bar_amb_temp;
    lv_obj_t *bar_amb_hum;
    lv_obj_t *buttons_1;
    lv_obj_t *btn_home_1;
    lv_obj_t *btn_settings_1;
    lv_obj_t *refresh_delay_config;
    lv_obj_t *table1;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_HOME = 1,
    SCREEN_ID_DATA = 2,
};

void create_screen_home();
void delete_screen_home();
void tick_screen_home();

void create_screen_data();
void delete_screen_data();
void tick_screen_data();

void create_screen_by_id(enum ScreensEnum screenId);
void delete_screen_by_id(enum ScreensEnum screenId);
void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/