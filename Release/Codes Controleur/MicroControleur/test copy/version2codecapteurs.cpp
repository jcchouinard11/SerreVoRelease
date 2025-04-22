/*  Install the "lvgl" library version 9.2 by kisvegabor to interface with the TFT Display - https://lvgl.io/
    *** IMPORTANT: lv_conf.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE lv_conf.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <lvgl.h>

/*  Install the "TFT_eSPI" library by Bodmer to interface with the TFT Display - https://github.com/Bodmer/TFT_eSPI
    *** IMPORTANT: User_Setup.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE User_Setup.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
#include <TFT_eSPI.h>

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen - Note: this library doesn't require further configuration
#include <XPT2046_Touchscreen.h>
#include <BLEDevice.h>       // Fonctionnalités BLE (Bluetooth Low Energy)
#include <BLEUtils.h>
#include <BLEServer.h>
#include "ui.h"
#include "Adafruit_SHT31.h"  // Capteur SHT31 (température et humidité)
#include "Adafruit_seesaw.h" // Contrôleur de capteur seesaw (température, humidité, etc.)

#define debug true // Debug

#define SensorSDA 45
#define SensorSCL 48
#define SHT31ADRESS 0x44
#define SEESAWSOILADRESS 0x36
#define SD_CHIPSELECT 10

// Touchscreen pins
#define XPT2046_IRQ  255  // T_IRQ
#define XPT2046_MOSI 4  // T_DIN
#define XPT2046_MISO 15  // T_OUT
#define XPT2046_CLK 47   // T_CLK
#define XPT2046_CS 16    // T_CS

SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
Adafruit_SHT31 sht31 = Adafruit_SHT31(); // Capteur d'humidité/température SHT31
Adafruit_seesaw ss;                     // Capteur seesaw pour la température du sol
Adafruit_SHT31 sht312 = Adafruit_SHT31(&Wire3); // Capteur d'humidité/température SHT31
//Adafruit_seesaw ssBUS1(&Bus1);    
Adafruit_seesaw ssBUS2(&Wire3);                     // Capteur seesaw pour la température du sol
Adafruit_seesaw ssBUS3();                     // Capteur seesaw pour la température du sol
Adafruit_seesaw ssBUS4();                     // Capteur seesaw pour la température du sol 

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// ------------------------------------------------
// Configuration du BLE (Bluetooth Low Energy)
// ------------------------------------------------
BLEAdvertising *advertising;   // Gestion des données publicitaires BLE
BLEServer *bleServer;          // Serveur BLE
BLEUUID SERVICE_UUID("91bad492-b950-4226-aa2b-4ede9fa42f59"); // UUID pour identifier le service BLE
// ------------------------------------------------

// The used commands use up to 48 bytes. On some Arduino's the default buffer
// space is not large enough
#define MAXBUF_REQUIREMENT 48

#if (defined(I2C_BUFFER_LENGTH) && (I2C_BUFFER_LENGTH >= MAXBUF_REQUIREMENT)) || (defined(BUFFER_LENGTH) && BUFFER_LENGTH >= MAXBUF_REQUIREMENT)
#define USE_PRODUCT_INFO
#endif

enum PMStatus {
  Green,
  Yellow,
  Red
};

PMStatus _pmStatus;

struct SensorSoil {
    String name; //
    TwoWire Port; //Au minum ont va savoir quel port est le capteur ensuite il faudrait determiner si il est sur un multiplexeur ou non (voir si ont fait un tableau avec les multiplexeur)
    uint8_t address; //
    float temperature; //
    float humidity; //
};
#define TCAADDR 0x70
#define MAX_SENSORS 10  // Max number of sensors you want to track
// Define Queue handle
QueueHandle_t QueueHandle;
QueueHandle_t QueueHandleBLE;
TaskHandle_t bleTaskHandle;        // Gestionnaire pour la tâche BLE
const int QueueElementSize = 15;
typedef struct {
  float sharedTempAmbiante, sharedHumiditeAmbiante, sharedTempSol,sharedTempAmbiante2,sharedHumiditeAmbiante2; // Température et humidité partagées
uint16_t sharedEauSol; 

} message_t;

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

SemaphoreHandle_t gui_mutex;


typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *main3;
    lv_obj_t *obj0;
    lv_obj_t *table1;
} objects_t;

extern objects_t objects;
int _arrow_angle = 0;
unsigned long lastTickMillis = 0;

/* Display flushing */
/* void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.writePixels((lgfx::rgb565_t *)px_map, w * h);
  tft.endWrite();

  lv_disp_flush_ready(disp);
} */
void tcaselect(uint8_t i) {
  if (i > 7) return;
 
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}
// Get the Touchscreen data`
// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;
void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z)
  if(touchscreen.touched()) {
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
  else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void Task_Dummy_Sensor(void *pvParameters) {
  message_t message;
  while (1) {
    message.sharedEauSol = random(30, 33);
    message.sharedTempSol = random(30, 35);
    
    int ret = xQueueSend(QueueHandle, (void *)&message, 0);
    if (ret == pdTRUE) {
      // The message was successfully sent.
      Serial.println("The message was successfully sent.");
    } else if (ret == errQUEUE_FULL) {
      // Since we are checking uxQueueSpacesAvailable this should not occur, however if more than one task should
      //   write into the same queue it can fill-up between the test and actual send attempt
      Serial.println("Task Dummy Sensor was unable to send data into the Queue");
    }  // Queue send check

    vTaskDelay(1000);
  }
}

void Task_Sensor(void *pvParameters) {
Wire.setPins(SensorSDA,SensorSCL);
Wire.begin();
tcaselect(0);
Wire3.setPins(41,40);
sht31.begin(SHT31ADRESS); //Adresse du SHT 31 et le begin 
sht312.begin(SHT31ADRESS); //Adresse du SHT 31 et le begin 
ss.begin(SEESAWSOILADRESS); //Adresse du SeeSaw Soil Sensor et le begin

  message_t message;
  message_t messageBLE;

  while (1) {
    Serial.printf("\n[Task_Sensor] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
    message.sharedTempAmbiante = sht31.readTemperature();
    message.sharedHumiditeAmbiante= sht31.readHumidity();
  
    message.sharedTempAmbiante2 = sht312.readTemperature();
    message.sharedHumiditeAmbiante2= sht312.readHumidity();
    message.sharedTempSol= ss.getTemp();
    message.sharedEauSol= ss.touchRead(0);

    xQueueSend(QueueHandleBLE, (void *)&message, 0);
    int ret = xQueueSend(QueueHandle, (void *)&message, 0);
      
      if (ret == pdTRUE) {
        // The message was successfully sent.
        Serial.println("[Task_Sensor] The message was successfully sent.");
      } else if (ret == errQUEUE_FULL) {
        // Since we are checking uxQueueSpacesAvailable this should not occur, however if more than one task should
        //   write into the same queue it can fill-up between the test and actual send attempt
        Serial.println("[Task_Sensor] Task Sensor was unable to send data into the Queue");
      }  // Queue send check
    delay(1000);
    }
    vTaskDelay(10000);
  }
static uint32_t my_tick(void)
{
    return millis();
}
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}
void Task_LVGL(void *pvParameters) {
  // Start LVGL
  Serial.printf("\n[LVGL] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));

  while (1) {
    
    unsigned int tickPeriod = millis() - lastTickMillis;
    lv_tick_inc(tickPeriod);
    lastTickMillis = millis();

    // Take the semaphore to access LVGL resources
    if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) {
      // Call LVGL's main task handler
      lv_timer_handler();
      lv_task_handler();

      // Release the semaphore after LVGL operations
      xSemaphoreGive(gui_mutex);
    }

    // Delay to control LVGL's refresh rate
    vTaskDelay(pdMS_TO_TICKS(10));  // Adjust as needed
  }
}

void Task_Screen_Update(void *pvParameters) {

  message_t message;
  char label_char[100];
    char strVarTAmbiante[20], strVarHAmbiante[20], srtVarTsoil[20], strVarHsoil[20],strVarTAmbiante2[20], strVarHAmbiante2[20];
  while (1) {
    Serial.printf("\n[Task_Screen_Update] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
    if (QueueHandle != NULL) {  // Sanity check just to make sure the queue actually exists
      if (xQueueReceive(QueueHandle, &message, portMAX_DELAY)) {
        Serial.println("[Task_Screen_Update] The message was successfully received.");

        if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) {
            dtostrf(message.sharedTempAmbiante, 4, 2, strVarTAmbiante);
            dtostrf(message.sharedHumiditeAmbiante, 4, 2, strVarHAmbiante);
            dtostrf(message.sharedTempAmbiante2, 4, 2, strVarTAmbiante2);
            dtostrf(message.sharedHumiditeAmbiante2, 4, 2, strVarHAmbiante2);
            dtostrf(message.sharedTempSol, 4, 2, srtVarTsoil);
            dtostrf(message.sharedEauSol, 4, 0, strVarHsoil);
            lv_table_set_cell_value(objects.table1,1,1,strVarTAmbiante);
            lv_table_set_cell_value(objects.table1,1,2,strVarHAmbiante);
            lv_table_set_cell_value(objects.table1,3,1,strVarTAmbiante2);
            lv_table_set_cell_value(objects.table1,3,2,strVarHAmbiante2);
            lv_table_set_cell_value(objects.table1,2,1,srtVarTsoil);
            lv_table_set_cell_value(objects.table1,2,2,strVarHsoil);


          // Release the semaphore after LVGL operations
          xSemaphoreGive(gui_mutex);
        }
      } else {
        Serial.println("[Task_Screen_Update] It was unable to receive data from the Queue.");
      }
    }  // Sanity check
  }
}

/**
 * @brief Fonction pour préparer les données en BLE (Fonction asyncrone)
 * @param temp1, hum1,temp2,hum2
 * @return Rien
 */
void updateAdvertisementData(float temp1, float hum1, float temp2, uint16_t hum2) 
{
    // Création d'un objet de donnée BLE
    BLEAdvertisementData advertisementData;
    const char* dataCStr;  //Variable temporaire pour stocker les donnée BLE en une ligne
    //Convertit les données passées en parametres en string de une ligne
    String data = String(temp1) + "," + String(hum1) + "," + String(temp2) + "," + String(hum2);  // Adding a comma for readability
    dataCStr = data.c_str();

    //Definition des differents parametres pour le message BLE 
    advertisementData.setFlags(0x06);      // Mode générale détectable, BR/EDR Non supporté
    if(debug)
    {
      Serial.println("Updated :");
      Serial.println(dataCStr);
    }
    //advertisementData.setName("ESP32_SerVo"); 
    advertisementData.addData(dataCStr);  // Ajout des données a la prochaine publication 

    // Mise en paramete des données a publier 
    advertising->setAdvertisementData(advertisementData);
    if(debug)
    {
      Serial.println("Updated advertising data with new sensor values:");
      Serial.println(data);
    }
    
}
/**
 * @brief Fonction pour transmettre les données en BLE ansi que SD (Fonction asyncrone)
 * @param void *parameter
 * @return Rien
 */
void Task_BLE(void *pvParameter) {
    Serial.printf("\n[BLETASK] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
    message_t message;
    while (true) {
      if (QueueHandleBLE != NULL) {  // Sanity check just to make sure the queue actually exists
      if (xQueueReceive(QueueHandleBLE, &message, portMAX_DELAY)) {
        Serial.println("[BLE] The message was successfully received.");
          updateAdvertisementData(message.sharedTempAmbiante, message.sharedHumiditeAmbiante, message.sharedTempSol, message.sharedEauSol); //Mise a jour des données et mise en attente du BLE
          advertising->start();
          delay(50);
          advertising->stop();    
      }
        //Acces aux données partagés
        
        delay(5000);
    }
    }
}

void setup() {
  Serial.begin(115200);
  lv_init();
  lv_log_register_print_cb(log_print);
  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
  touchscreen.setRotation(0);
  
  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
    
  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  // Set the callback function to read Touchscreen input
  lv_indev_set_read_cb(indev, touchscreen_read);

  // Function to draw the GUI (text, buttons and sliders)
  ui_init();
  lv_table_set_cell_value(objects.table1,0,1,"Temperature");
  lv_table_set_cell_value(objects.table1,0,2,"Humidité");
  lv_table_set_cell_value(objects.table1,1,0,"Ambiante");
  lv_table_set_cell_value(objects.table1,2,0,"Sol");
  lv_table_set_cell_value(objects.table1,3,0,"Ambiant");
  // Create the queue which will have <QueueElementSize> number of elements, each of size `message_t` and pass the address to <QueueHandle>.
  QueueHandle = xQueueCreate(QueueElementSize, sizeof(message_t));
  QueueHandleBLE = xQueueCreate(QueueElementSize, sizeof(message_t));
  // Check if the queue was successfully created
  if (QueueHandle == NULL) {
    Serial.println("Queue could not be created. Halt.");
    while (1) delay(1000);  // Halt at this point as is not possible to continue
  }
  // ------------------------------------------------
  // Initialization du BLE  
  // ------------------------------------------------
  BLEDevice::init("ESP32_SerVo");
  bleServer = BLEDevice::createServer();

  // Start advertising
  advertising = BLEDevice::getAdvertising();

  advertising->start();
  if(debug)
  {
    Serial.println("iBeacon + service defined and advertising!");
  }

  gui_mutex = xSemaphoreCreateMutex();
  if (gui_mutex == NULL) {
    // Handle semaphore creation failure
    Serial.println("semaphore creation failure");
    return;
  }

  xTaskCreatePinnedToCore(Task_LVGL,    // Pointer to the task entry function.
                          "Task_LVGL",  // A descriptive name for the task.
                          1024 * 10,    // The size of the task stack specified as the number of bytes
                          NULL,         // Pointer that will be used as the parameter for the task being created.
                          4,            // The priority at which the task should run.
                          NULL,         // Used to pass back a handle by which the created task can be referenced.
                          0);           // The core to which the task is pinned to, or tskNO_AFFINITY if the task has no core affinity.

  xTaskCreatePinnedToCore(Task_Screen_Update,
                          "Task_Screen_Update",
                          1024 * 3,
                          NULL,
                          3,
                          NULL,
                          1);

  // This task is only available when a sensor is connected.
  // If you do not have the Environmental sensor, use the task below.
  xTaskCreatePinnedToCore(Task_Sensor,
                          "Task_Sensor",
                          1024 * 4,
                          NULL,
                          2,
                          NULL,
                          1);

  // xTaskCreatePinnedToCore(Task_Dummy_Sensor,
  //                         "Task_Dummy_Sensor",
  //                         1024 * 2,
  //                         NULL,
  //                         1,
  //                         NULL,
  //                         1);

    xTaskCreatePinnedToCore(Task_BLE,
                          "BLE Bridge data send",
                           1024*4,
                           NULL,
                           1,
                           NULL,
                           1); 
}

void loop() {}


