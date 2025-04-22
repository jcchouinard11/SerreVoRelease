#ifndef CAPTEURS_H
#define CAPTEURS_H

#include "utils.h"
#include "Adafruit_seesaw.h"
#include "Adafruit_SHT31.h"

// Initialisation du capteur de température interne
void initTempSensor();

// Fonctions de gestion du multiplexeur et des capteurs
void selectMuxChannel(uint8_t bus, uint8_t channel);
bool isMuxPresent(uint8_t bus);
int findExistingSensor(uint8_t bus, uint8_t mux_channel, bool is_mux);
void displayData();
void readSensors();
void detectSensors(void *pvParameters);

#endif
