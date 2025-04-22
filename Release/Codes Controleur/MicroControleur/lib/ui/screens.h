#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *temperature_panel;
    lv_obj_t *data_h_text_1;
    lv_obj_t *data_c_text_1;
    lv_obj_t *data_h_1;
    lv_obj_t *tag_temp;
    lv_obj_t *data_c_1;
    lv_obj_t *tag_hum;
    lv_obj_t *data_t_amb_text;
    lv_obj_t *data_h_amb_text;
    lv_obj_t *bar_amb_temp;
    lv_obj_t *bar_amb_hum;
    lv_obj_t *obj2;
    lv_obj_t *home_ch_label;
    lv_obj_t *obj3;
    lv_obj_t *update_bar;
    lv_obj_t *table1;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *refresh_delay_config;
    lv_obj_t *local_update_config_1;
    lv_obj_t *ble_update_config_2;
    lv_obj_t *plug_sensor_config_1;
    lv_obj_t *plug_sensor_config_2;
    lv_obj_t *plug_sensor_config_3;
    lv_obj_t *plug_sensor_config_4;
    lv_obj_t *plug_sensor_config_5;
    lv_obj_t *plug_sensor_config_6;
    lv_obj_t *veille_delay_config;
    lv_obj_t *plug_sensor_config_7;
    lv_obj_t *plug_sensor_config_8;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
};

void create_screen_main();
void delete_screen_main();
void tick_screen_main();

void create_screen_by_id(enum ScreensEnum screenId);
void delete_screen_by_id(enum ScreensEnum screenId);
void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/