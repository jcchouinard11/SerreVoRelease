#include "actions.h"
#include "screens.h"
#include "C:\Users\choui\Documents\GitHub\SerreVo\ProjetFinal\Code\lib\ui\ui.h"

static int16_t currentScreen = -1;
static lv_obj_t *getScreenObj(enum ScreensEnum screenId) {
    return ((lv_obj_t **)&objects)[screenId - 1];
}

static void on_screen_unloaded(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_SCREEN_UNLOADED) {
        enum ScreensEnum screenId =
            (enum ScreensEnum)(lv_uintptr_t)lv_event_get_user_data(e);
        delete_screen_by_id(screenId);
    }
}

static void deleteScreenOnUnload(enum ScreensEnum screenId) {
    lv_obj_add_event_cb(
        getScreenObj(screenId),
        on_screen_unloaded,
        LV_EVENT_SCREEN_UNLOADED,
        (void*)(lv_uintptr_t)(screenId)
    );    
}
static void changeToScreen(enum ScreensEnum screenId) {
    if (!getScreenObj(screenId)) {
        create_screen_by_id(screenId);
        if (!getScreenObj(screenId)) {
            return;
        }
        deleteScreenOnUnload(screenId);
    }

    loadScreen(screenId);
}
void action_return_to_home(lv_event_t *e) {
    // TODO: Implement action return_to_home here
    changeToScreen(SCREEN_ID_HOME);
}
void action_go_to_data(lv_event_t *e) {
    // TODO: Implement action go_to_data here
    changeToScreen(SCREEN_ID_DATA);
}