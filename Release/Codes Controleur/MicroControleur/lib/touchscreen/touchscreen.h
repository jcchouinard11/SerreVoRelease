#ifndef TOUCHSCREEN_H
#define TOUCHSCREEN_H

#include "utils.h"
#include <XPT2046_Touchscreen.h>
#include <lvgl.h>

void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data);

#endif
