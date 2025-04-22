#include "touchscreen.h"
#include "actions.h"


#define BACKLIGHT_PIN 17  // Set this to your actual MOSFET control pin
//#define DIM_TIMEOUT 10000 // Time in milliseconds before dimming (10s)
#define FULL_BRIGHTNESS 0  // Max PWM value (fully on)
#define DIM_BRIGHTNESS 220    // Dim level (adjust as needed)

unsigned long lastTouchTime = 0;  // Stores the last time screen was touched
bool isDimmed = false;  // Track if the screen is dimmed

void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) 
{
  if (touchscreen.touched()) 
  {
    TS_Point p = touchscreen.getPoint();
  /*   x = map(p.x, 220, 3850, 1, SCREEN_HEIGHT);
    y = map(p.y, 310, 3773, 1, SCREEN_WIDTH); */
    x = map(p.x, 200, 3850, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3950, 1, SCREEN_HEIGHT);
    z = p.z;
    if(x<0){
      x=0;
    }
    if(y<0){
      y=0;
    }

    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;

    lastTouchTime = millis(); // Update last touch time

    // Restore brightness if dimmed
    if (isDimmed) {
      analogWrite(BACKLIGHT_PIN, FULL_BRIGHTNESS);
      
      isDimmed = false;
    }
  }
  else 
  {
    data->state = LV_INDEV_STATE_RELEASED;
    
    // Check if it's time to dim the screen
    if (!isDimmed && millis() - lastTouchTime >= veille_delay) {
      analogWrite(BACKLIGHT_PIN, DIM_BRIGHTNESS);
      
      isDimmed = true;
    }
  }
}
void action_save(lv_event_t *e) {
    putConfig();
}