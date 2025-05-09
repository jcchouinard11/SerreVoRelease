#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *main3;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *table1_1;
    lv_obj_t *obj2;
    lv_obj_t *todata_2;
    lv_obj_t *data_c_text_2;
    lv_obj_t *data_f_text_2;
    lv_obj_t *data_c_2;
    lv_obj_t *data_f_2;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_MAIN3 = 2,
};

void create_screen_main();
void delete_screen_main();
void tick_screen_main();

void create_screen_main3();
void delete_screen_main3();
void tick_screen_main3();

void create_screen_by_id(enum ScreensEnum screenId);
void delete_screen_by_id(enum ScreensEnum screenId);
void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/