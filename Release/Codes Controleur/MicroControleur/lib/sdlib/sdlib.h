
// SDLib.h
#ifndef SDLIB_H
#define SDLIB_H

#include <Arduino.h>
#include <Wire.h>
#include "utils.h"
#include "FS.h"
#include "SD.h"
extern const char* filePath;
extern bool cardType; //Variable pour établir si une carte SD est connectée ou non 
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void createDir(fs::FS &fs, const char *path);
void removeDir(fs::FS &fs, const char *path);
void readFile(fs::FS &fs, const char *path);
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void renameFile(fs::FS &fs, const char *path1, const char *path2);
void deleteFile(fs::FS &fs, const char *path);
void appendSensorTableToCSV(fs::FS &fs, const char *path, SensorData sensorTable[], int sensorCount, 
                            float batteryVoltage, int pumpSpeed, 
                            const char* pumpState, const char* fanState, const char* windowState);
void testFileIO(fs::FS &fs, const char *path);

#endif