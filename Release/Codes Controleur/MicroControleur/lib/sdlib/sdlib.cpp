// SDLib.cpp
#include "SDLib.h"
extern const char* filePath = "/data_log.csv";
extern bool cardType = false; //Variable pour établir si une carte SD est connectée ou non 
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) {
        Serial.println("Failed to open directory or not a directory");
        return;
    }
    File file = root.openNextFile();
    while (file) {
        Serial.print(file.isDirectory() ? "  DIR : " : "  FILE: ");
        Serial.print(file.name());
        if (!file.isDirectory()) {
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        if (levels && file.isDirectory()) listDir(fs, file.path(), levels - 1);
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char *path) {
    Serial.printf("Creating Dir: %s\n", path);
    Serial.println(fs.mkdir(path) ? "Dir created" : "mkdir failed");
}

void removeDir(fs::FS &fs, const char *path) {
    Serial.printf("Removing Dir: %s\n", path);
    Serial.println(fs.rmdir(path) ? "Dir removed" : "rmdir failed");
}

void readFile(fs::FS &fs, const char *path) {
    Serial.printf("Reading file: %s\n", path);
    File file = fs.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }
    while (file.available()) Serial.write(file.read());
    file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    Serial.println(file.print(message) ? "File written" : "Write failed");
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    Serial.println(file.print(message) ? "Message appended" : "Append failed");
    file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    Serial.println(fs.rename(path1, path2) ? "File renamed" : "Rename failed");
}

void deleteFile(fs::FS &fs, const char *path) {
    Serial.printf("Deleting file: %s\n", path);
    Serial.println(fs.remove(path) ? "File deleted" : "Delete failed");
}

void appendSensorTableToCSV(fs::FS &fs, const char *path, SensorData sensorTable[], int sensorCount, 
                            float batteryVoltage, int pumpSpeed, 
                            const char* pumpState, const char* fanState, const char* windowState) {
    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        printf("Failed to open file for appending");
        return;
    }

    // Vérifie si le fichier est vide pour écrire l'en-tête
    if (file.size() == 0) {
        file.println("Timestamp,Index,Bus,MuxChannel,IsMux,Humidity,Temperature,"
                     "BatteryVoltage,PumpSpeed,PumpState,FanState,WindowState");
    }

    // Écriture des capteurs
    for (int i = 0; i < sensorCount; i++) {
        file.printf("%lu,%d,%d,%d,%d,%.2f,%.2f,%.2f,%d,%s,%s,%s\n",
                    millis(), sensorTable[i].index, sensorTable[i].bus, 
                    sensorTable[i].mux_channel, sensorTable[i].is_mux, 
                    sensorTable[i].humidity, sensorTable[i].temperature, 
                    batteryVoltage, pumpSpeed, pumpState, fanState, windowState);
    }

    file.close();
    printf("Sensor table appended to CSV");
}
void testFileIO(fs::FS &fs, const char *path) {
    File file = fs.open(path);
    uint8_t buf[512];
    size_t len = file ? file.size() : 0;
    uint32_t start = millis();
    if (file) {
        while (len) {
            size_t toRead = len > 512 ? 512 : len;
            file.read(buf, toRead);
            len -= toRead;
        }
        Serial.printf("%u bytes read for %u ms\n", file.size(), millis() - start);
        file.close();
    }
    file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    start = millis();
    for (size_t i = 0; i < 2048; i++) file.write(buf, 512);
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, millis() - start);
    file.close();
}
