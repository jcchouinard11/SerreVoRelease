#include <Wire.h>
#include <lvgl.h>
#include <Arduino.h>
#include "Adafruit_seesaw.h"
#include <XPT2046_Touchscreen.h>
#include "ui.h"
#include "Wire.h"
#include <TFT_eSPI.h>
#include <BLEDevice.h>       // Fonctionnalités BLE (Bluetooth Low Energy)
#include <BLEUtils.h>
#include <BLEServer.h>

#define NUM_BUSES 4
#define NUM_MUX 4
#define MUX_ADDR 0x70
#define MAX_SENSORS_PER_MUX 4
#define MAX_TOTAL_SENSORS (NUM_MUX * MAX_SENSORS_PER_MUX)
// Touchscreen pins
#define XPT2046_IRQ  255  // T_IRQ
#define XPT2046_MOSI 4  // T_DIN
#define XPT2046_MISO 15  // T_OUT
#define XPT2046_CLK 47   // T_CLK
#define XPT2046_CS 16    // T_CS

TwoWire i2c_buses[NUM_BUSES] = {Wire, Wire1, Wire2, Wire3};
Adafruit_seesaw* sensors[MAX_TOTAL_SENSORS];

struct SensorData {
    uint8_t index;
    uint8_t bus;
    uint8_t mux_channel;
    bool is_mux;
    float humidity;
    float temperature;
};

SensorData sensorTable[MAX_TOTAL_SENSORS];
SemaphoreHandle_t gui_mutex;
QueueHandle_t QueueHandle;
const int QueueElementSize = 15;

SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *main3;
    lv_obj_t *obj0;
    lv_obj_t *table1;
} objects_t;

extern objects_t objects;
int _arrow_angle = 0;
unsigned long lastTickMillis = 0;
int x, y, z; //Position du touch 
// ------------------------------------------------
// Configuration du BLE (Bluetooth Low Energy)
// ------------------------------------------------
BLEAdvertising *advertising;   // Gestion des données publicitaires BLE
BLEServer *bleServer;          // Serveur BLE
BLEUUID SERVICE_UUID("91bad492-b950-4226-aa2b-4ede9fa42f59"); // UUID pour identifier le service BLE

// ------------------------------------------------
// Traitement et detection des capteurs
// ------------------------------------------------
void selectMuxChannel(uint8_t bus, uint8_t channel) {
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    i2c_buses[bus].write(1 << channel);
    i2c_buses[bus].endTransmission();
}

bool isMuxPresent(uint8_t bus) {
    i2c_buses[bus].beginTransmission(MUX_ADDR);
    return (i2c_buses[bus].endTransmission() == 0);
}

void detectSensors(void *pvParameters) {
    SensorData sensorTable[MAX_TOTAL_SENSORS];
    uint8_t sensorCount = 0;
    while (1) 
    {
            for (uint8_t bus = 0; bus < NUM_BUSES; bus++) 
            {
            // Check for direct sensors on the bus
            sensors[sensorCount] = new Adafruit_seesaw(&i2c_buses[bus]);
            if (sensors[sensorCount]->begin(0x36)) 
            {
                sensorTable[sensorCount] = {sensorCount, bus, 255, false, 0, 0};
                sensorCount++;
            } else 
            {
                delete sensors[sensorCount];
                sensors[sensorCount] = nullptr;
            }
            
            // Check for sensors connected via multiplexers only if a mux is present
            if (isMuxPresent(bus)) 
            {
                for (uint8_t mux = 0; mux < NUM_MUX; mux++) 
                {
                    selectMuxChannel(bus, mux);
                    sensors[sensorCount] = new Adafruit_seesaw(&i2c_buses[bus]);
                    if (sensors[sensorCount]->begin(0x36)) 
                    {
                        sensorTable[sensorCount] = {sensorCount, bus, mux, true, 0, 0};
                        sensorCount++;
                    } else 
                    {
                        delete sensors[sensorCount];
                        sensors[sensorCount] = nullptr;
                    }
                }
            }
        }
        readSensors();
        int ret = xQueueSend(QueueHandle, (void *)&sensorTable, 0);
      
        if (ret == pdTRUE) {
            // The message was successfully sent.
            Serial.println("[Task_Sensor] The message was successfully sent.");
        } else if (ret == errQUEUE_FULL) {
            // Since we are checking uxQueueSpacesAvailable this should not occur, however if more than one task should
            //   write into the same queue it can fill-up between the test and actual send attempt
            Serial.println("[Task_Sensor] Task Sensor was unable to send data into the Queue");
        }  // Queue send check
        }
        delay(5000); // Update data every second
        
      
    }
    

void readSensors() {
    uint8_t sensorCount = 0;
    for (uint8_t i = 0; i < sensorCount; i++) {
        if (sensorTable[i].is_mux) {
            selectMuxChannel(sensorTable[i].bus, sensorTable[i].mux_channel);
        }
        if (sensors[i]) {
            sensorTable[i].humidity = sensors[i]->touchRead(0);
            sensorTable[i].temperature = sensors[i]->getTemp();
        }
    }
}
//Utilisé pour des fins de debug
/* void displayData() {
    uint8_t sensorCount = 0;
    for (uint8_t i = 0; i < sensorCount; i++) {
        Serial.printf("Sensor %d (Bus %d, %s %d): Humidity = %.2f, Temp = %.2f\n", 
                      sensorTable[i].index, sensorTable[i].bus, 
                      sensorTable[i].is_mux ? "Mux" : "Direct", 
                      sensorTable[i].is_mux ? sensorTable[i].mux_channel : 255,
                      sensorTable[i].humidity, sensorTable[i].temperature);
    }
} */
// ------------------------------------------------
// LVGL
// ------------------------------------------------
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
/**
 * @brief Fonction pour le debug de la libraire LVGL
 * @param void *parameter
 * @return Rien
 */
void log_print(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}

/**
 * @brief Tache qui met a jour l'écran avec les données des capteurs.
 * @param void *parameter
 * @return Rien
 */
void Task_Screen_Update(void *pvParameters) 
{
    char label_char[100];
        char strVarTAmbiante[20], strVarHAmbiante[20], srtVarTsoil[20], strVarHsoil[20],strVarTAmbiante2[20], strVarHAmbiante2[20];
    while (1)
        {
            Serial.printf("\n[Task_Screen_Update] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
            if (QueueHandle != NULL) {  // Sanity check just to make sure the queue actually exists
            if (xQueueReceive(QueueHandle, &sensorTable, portMAX_DELAY))   
            {
                Serial.println("[Task_Screen_Update] The message was successfully received.");

                if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) 
                {
                    uint8_t sensorCount = 0; // Vous pouvez remplacer ceci par la variable réelle qui contient le nombre de capteurs
                    char humidityStr[10];
                    char temperatureStr[10];
                    
                // Supposons que le tableau d'objets LVGL soit déjà configuré
                    for (uint8_t i = 0; i < sensorCount; i++) 
                    {
                        // Convertir les valeurs de l'humidité et de la température en chaîne de caractères
                        dtostrf(sensorTable[i].humidity, 4, 2, humidityStr);
                        dtostrf(sensorTable[i].temperature, 4, 2, temperatureStr);
                        
                        // Remplir les cellules du tableau LVGL avec les données des capteurs
                        lv_table_set_cell_value(objects.table1, i+1, 0, humidityStr); // Humidité dans la première colonne
                        lv_table_set_cell_value(objects.table1, i+1, 1, temperatureStr); // Température dans la deuxième colonne
                        
                        // Release the semaphore after LVGL operations
                    
                    }
                    xSemaphoreGive(gui_mutex);
                }
            }  // Sanity check
        }
    }
}
/**
 * @brief Mise a jour de l'écran (TACHE SUR UN THREAD)
 * @param void *parameter
 * @return Rien
 */
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


void setup() {
    Serial.begin(115200);
    for (uint8_t i = 0; i < NUM_BUSES; i++) {
        i2c_buses[i].begin();
    }
    lv_init();
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
  QueueHandle = xQueueCreate(QueueElementSize, sizeof(SensorData));
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
  xTaskCreatePinnedToCore(detectSensors,
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

void loop() {

}
