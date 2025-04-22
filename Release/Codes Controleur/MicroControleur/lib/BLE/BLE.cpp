#include "BLE.h"
BLEUUID SERVICE_UUID("91bad492-b950-4226-aa2b-4ede9fa42f59"); 
BLEAdvertising *advertising;
BLEServer *bleServer;
static BLEAdvertisementData advertisementData;
String message_humidite = "";
String message_temperature = "";
byte dataByte;
String message;


// Do your data sending he

void updateAdvertisementData(const String &messageCSTR) {
    advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x06);
    advertisementData.addData(messageCSTR.c_str());
    advertising->setAdvertisementData(advertisementData);
     printf("\nData Advertised: ");
    printf("%s",message);
} 

String creerMessage(int *donnees, byte type, int longueur) {
    message = String((char)type);
    for (int i = 0; i < longueur; i++) {
        dataByte = donnees[i];
        if (donnees[i] == 0) {
            message += String((char)0xC8);
        } else {
            message += String((char)dataByte);
        }
    }
    return message;
}

float round_to_half_integer(float x) {
    return 0.5 * round(2.0 * x);
}

void setupBLE() {
    BLEDevice::init("ESP32_SerVo");
    bleServer = BLEDevice::createServer();
    advertising = BLEDevice::getAdvertising();
    //heap_caps_malloc_extmem_enable(0);
    
}
void Task_BLE(void *pvParameters) {
      printf("\n[BLE] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
    while(1){
        unsigned long start = millis();
        //printf("\nRAM USAGE BEFORE:%i",ESP.getFreeHeap()); 
        int temperature[MAX_BLE] = {0xFF};
        int humidite[MAX_BLE] = {0xFF};
        for (int i = 0; i < MAX_BLE; i++) {
            temperature[i] = 0xFF;
            humidite[i] = 0xFF;
        }
        if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) 
        {   
            for (int i = 0; i < MAX_TOTAL_SENSORS; i++) {
                    
                if (sensorTable[i].present) {
                    int index = sensorTable[i].is_mux ?
                        MAX_SENSORS_PER_MUX * sensorTable[i].bus + sensorTable[i].mux_channel + 1 + sensorTable[i].bus :
                        MAX_SENSORS_PER_MUX * sensorTable[i].bus + sensorTable[i].bus;
                    temperature[index] = round_to_half_integer(sensorTable[i].temperature) * 2;
                    humidite[index] = sensorTable[i].humidity;
                }
                //printf("\n%d", sensorTable[i].temperature);
            }
            if(shtPresent) {
                temperature[MAX_BLE - 1] = round_to_half_integer(sharedTempAmbiante) * 2;
                humidite[MAX_BLE - 1] = sharedHumiditeAmbiante;
            }
            xSemaphoreGive(data_mutex);     
        }
        
    
        message_humidite = creerMessage(humidite, 0xAA, MAX_BLE);
        message_temperature = creerMessage(temperature, 0xEE, MAX_BLE);
        updateAdvertisementData(message_humidite);
        advertising->start();
        delay(TEMPS_ENVOIE);
        advertising->stop();
        delay(30);
        updateAdvertisementData(message_temperature);
        advertising->start();
        delay(TEMPS_ENVOIE);
        advertising->stop();
        //printf("\nRAM USAGE AFTER:%i",ESP.getFreeHeap());
        /*  appendSensorTableToCSV(SD, "/data.csv", tempTable, sensorCount, 
                        10, 10, "ON", "OFF", "OPEN"); */
        //printf("\nRAM USAGE AFTER:%i",ESP.getFreeHeap()); 
        
        delay(update_BLE_delay - TEMPS_ENVOIE * 2 - 30);
    }
}


