#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *data;
    lv_obj_t *home;
    lv_obj_t *table1;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *data_c_text_1;
    lv_obj_t *data_f_text_1;
    lv_obj_t *data_c_1;
    lv_obj_t *data_f_1;
    lv_obj_t *obj2;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_DATA = 1,
    SCREEN_ID_HOME = 2,
};

void create_screen_data();
void delete_screen_data();
void tick_screen_data();

void create_screen_home();
void delete_screen_home();
void tick_screen_home();

void create_screen_by_id(enum ScreensEnum screenId);
void delete_screen_by_id(enum ScreensEnum screenId);
void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/