#ifndef BLE_H
#define BLE_H

#include <Arduino.h>

#include "utils.h"

extern SensorData tempTable[MAX_TOTAL_SENSORS+1];

#define MAX_BLE 26
#define TEMPS_ENVOIE 2000
#define DELAY_MESSAGE_MS 10000

void Task_BLE(void *pvParameters);
void setupBLE();
void updateAdvertisementData(const String &message);
String creerMessage(int *donnees, byte type, int longueur);
float round_to_half_integer(float x);
extern BLEUUID SERVICE_UUID;
extern BLEAdvertising *advertising;
extern BLEServer *bleServer;   

#endif // BLE_H
