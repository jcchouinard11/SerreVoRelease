#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui.h>
#include <XPT2046_Touchscreen.h>

// TODO: Re-Define according to actual wiring of touch display
#define YP A1
#define XM A0
#define YM 4
#define XP 5

// TODO: Replace with your screen resolution
static const uint16_t screenWidth  = 240;
static const uint16_t screenHeight = 320;
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

bool touched = false;
bool coordinatesSet = false;
// Touchscreen pins
#define XPT2046_IRQ  255  // T_IRQ
#define XPT2046_MOSI 4  // T_DIN
#define XPT2046_MISO 15  // T_OUT
#define XPT2046_CLK 47   // T_CLK
#define XPT2046_CS 16    // T_CS
// TODO: Replace with your screen's touch controller
TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight);
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
int _arrow_angle = 0;
unsigned long lastTickMillis = 0;
int x, y, z; //Position du touch 
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) 
{
  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
  if(touchscreen.touched()) 
  {
    // Get Touchscreen points
    TS_Point p = touchscreen.getPoint();
    // Calibrate Touchscreen points with map function to the correct width and height
    x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    z = p.z;

    data->state = LV_INDEV_STATE_PRESSED;

    // Set the coordinates
    data->point.x = x;
    data->point.y = y;
  }
  else 
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}
/**
 * @brief Fonction pour le debug de la libraire LVGL
 * @param void *parameter
 * @return Rien
 */

void setup()
{
  lv_init();

  tft.begin();
  tft.setRotation(2);

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);

  // Register the custom display function
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init( &disp_drv );
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register( &disp_drv );

  // Register the custom touch-input handler
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init( &indev_drv );
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register( &indev_drv );

  // Init EEZ-Studio UI
  ui_init();
}

void loop() {
  lv_timer_handler();
  // Update EEZ-Studio UI
  ui_tick();
}