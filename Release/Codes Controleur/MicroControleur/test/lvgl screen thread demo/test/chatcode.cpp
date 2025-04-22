#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "Adafruit_SHT31.h"
#include "Adafruit_seesaw.h"

// Définition des pins et adresses
#define SensorSDA 45
#define SensorSCL 48
#define SHT31ADRESS 0x44
#define SEESAWSOILADRESS 0x36
#define XPT2046_IRQ 255
#define XPT2046_MOSI 4
#define XPT2046_MISO 15
#define XPT2046_CLK 47
#define XPT2046_CS 2

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// Objets pour les capteurs
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_seesaw ss;
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// BLE
BLEAdvertising *advertising;
BLEServer *bleServer;
BLEUUID SERVICE_UUID("91bad492-b950-4226-aa2b-4ede9fa42f59");

// Variables partagées protégées par un mutex
SemaphoreHandle_t dataMutex;
float sharedTempAmbiante, sharedHumiditeAmbiante, sharedTempSol;
uint16_t sharedEauSol;

// Tâches
TaskHandle_t bleTaskHandle;
TaskHandle_t guiTaskHandle;
TaskHandle_t lvglTaskHandle;

// Fonction de mise à jour des données BLE
void updateAdvertisementData(float temp1, float hum1, float temp2, uint16_t hum2) {
    BLEAdvertisementData advertisementData;
    String data = String(temp1) + "," + String(hum1) + "," + String(temp2) + "," + String(hum2);
    advertisementData.setFlags(0x06);
    advertisementData.addData(data.c_str());
    advertising->setAdvertisementData(advertisementData);
}

// Tâche BLE
void bleTask(void *parameter) {
    while (true) {
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
            updateAdvertisementData(sharedTempAmbiante, sharedHumiditeAmbiante, sharedTempSol, sharedEauSol);
            xSemaphoreGive(dataMutex);
        }
        advertising->start();
        delay(1000);
        advertising->stop();
    }
}

// Tâche GUI
void guiTask(void *parameter) {
    while (true) {
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
            // Simulation de la lecture des capteurs
            sharedTempAmbiante = sht31.readTemperature();
            sharedHumiditeAmbiante = sht31.readHumidity();
            sharedTempSol = ss.getTemp();
            sharedEauSol = ss.touchRead(0);
            xSemaphoreGive(dataMutex);
        }
        delay(1000);
    }
}

// Tâche LVGL pour gérer l'affichage
void lvglTask(void *parameter) {
    while (true) {
        lv_timer_handler();
        delay(5); // LVGL recommande un appel toutes les 5 ms
    }
}

// Configuration BLE
void initBLE() {
    BLEDevice::init("ESP32_SerVo");
    bleServer = BLEDevice::createServer();
    advertising = bleServer->getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
}

// Initialisation LVGL
void initLVGL() {
    lv_init();
    lv_log_register_print_cb([](lv_log_level_t level, const char *buf) { Serial.println(buf); });
    lv_display_t *disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
    touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(0);
}

// Setup
void setup() {
    Serial.begin(115200);

    Wire.setPins(SensorSDA, SensorSCL);
    sht31.begin(SHT31ADRESS);
    ss.begin(SEESAWSOILADRESS);

    initBLE();
    initLVGL();

    // Création des tâches FreeRTOS
    dataMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(bleTask, "BLE Task", 4096, NULL, 3, &bleTaskHandle, 1);
    xTaskCreatePinnedToCore(guiTask, "GUI Task", 4096, NULL, 2, &guiTaskHandle, 1);
    xTaskCreatePinnedToCore(lvglTask, "LVGL Task", 4096, NULL, 1, &lvglTaskHandle, 0);
}

// Boucle vide (tout est géré par les tâches)
void loop() {}
