#include "BLE.h"

// UUID du service BLE utilisé pour identifier l’annonce
BLEUUID SERVICE_UUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
TaskHandle_t BLEtaskHandle;
// Pointeurs vers les objets BLE de la librairie ArduinoBLE
BLEAdvertising *advertising;
BLEServer *bleServer;
static BLEAdvertisementData advertisementData;

// Messages publicitaires envoyés (séparés pour humidité et température)
String message_humidite = "";
String message_temperature = "";

// Byte temporaire pour formatage de données
byte dataByte;

// Message BLE final à transmettre (temp ou humidité)
String message;

/**
 * @brief Met à jour le message BLE à envoyer et le publie
 * @param messageCSTR Message brut à annoncer (String -> const char*)
 */
void updateAdvertisementData(const String &messageCSTR) {
    advertisementData = BLEAdvertisementData();
    advertisementData.setFlags(0x06); // BLE général + pas de BR/EDR
    advertisementData.addData(messageCSTR.c_str()); // Ajout du payload
    advertising->setAdvertisementData(advertisementData);
    printf("\nData Advertised: ");
    printf("%s", message); // Affichage debug
}

/**
 * @brief Crée une trame BLE formatée à partir d’un tableau de données
 * @param donnees Tableau de température ou humidité (entiers)
 * @param type Type du message : 0xAA = humidité, 0xEE = température
 * @param longueur Nombre de capteurs
 * @return Trame BLE prête à envoyer
 */
String creerMessage(int *donnees, byte type, int longueur) {
    message = String((char)type); // Premier octet = type
    for (int i = 0; i < longueur; i++) {
        dataByte = donnees[i];
        if (donnees[i] == 0) {
            message += String((char)0xC8); // 0 remplacé par C8
        } else {
            message += String((char)dataByte); // Sinon, valeur convertie
        }
    }
    return message;
}

/**
 * @brief Arrondit une valeur float au demi le plus proche
 * @param x Entrée en float
 * @return Arrondi (0.5, 1.0, 1.5, etc.)
 */
float round_to_half_integer(float x) {
    return 0.5 * round(2.0 * x);
}

/**
 * @brief Initialisation de la pile Bluetooth Low Energy
 */
void setupBLE() {
    BLEDevice::init("ESP32_SerVo"); // Nom visible par le pont BLE
    bleServer = BLEDevice::createServer(); // Création d’un serveur fictif
    advertising = BLEDevice::getAdvertising(); // Accès à l’annonceur
}

/**
 * @brief Tâche FreeRTOS de transmission BLE cyclique des capteurs
 * @note Envoie successivement l’humidité puis la température
 */
void Task_BLE(void *pvParameters) {
    printf("\n[BLE] running on core: %d, Free stack space: %d\n", xPortGetCoreID(), uxTaskGetStackHighWaterMark(NULL));
    BLEtaskHandle = xTaskGetCurrentTaskHandle(); // Récupération de l'identifiant de la tâche BLE
    while(1) {
        unsigned long start = millis();

        // Initialisation des tableaux avec valeur par défaut 0xFF
        int temperature[MAX_BLE] = {0xFF};
        int humidite[MAX_BLE] = {0xFF};

        // Remplit manuellement tous les indices avec 0xFF
        for (int i = 0; i < MAX_BLE; i++) {
            temperature[i] = 0xFF;
            humidite[i] = 0xFF;
        }

        // Verrouille les données pour lecture sécurisée
        if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) 
        {   
            for (int i = 0; i < MAX_TOTAL_SENSORS; i++) {
                if (sensorTable[i].present) {
                    // Calcul d’un index unique pour chaque capteur
                    int index = sensorTable[i].is_mux ?
                        MAX_SENSORS_PER_MUX * sensorTable[i].bus + sensorTable[i].mux_channel + 1 + sensorTable[i].bus :
                        MAX_SENSORS_PER_MUX * sensorTable[i].bus + sensorTable[i].bus;

                    // Stocke les valeurs dans les tableaux BLE
                    temperature[index] = round_to_half_integer(sensorTable[i].temperature) * 2;
                    humidite[index] = sensorTable[i].humidity;
                }
            }

            // Ajout du capteur ambiant (SHT) à la fin du tableau
            if (shtPresent) {
                temperature[MAX_BLE - 1] = round_to_half_integer(sharedTempAmbiante) * 2;
                humidite[MAX_BLE - 1] = sharedHumiditeAmbiante;
            }

            xSemaphoreGive(data_mutex);     
        }

        // Création et envoi des trames BLE
        message_humidite = creerMessage(humidite, 0xAA, MAX_BLE);
        message_temperature = creerMessage(temperature, 0xEE, MAX_BLE);

        // Envoie humidité
        updateAdvertisementData(message_humidite);
        advertising->start();
        delay(TEMPS_ENVOIE);
        advertising->stop();

        // Petite pause pour éviter chevauchement
        delay(30);

        // Envoie température
        updateAdvertisementData(message_temperature);
        advertising->start();
        delay(TEMPS_ENVOIE);
        advertising->stop();

        // Attente avant prochaine boucle d'envoi (ajustable via paramètre)
        delay(update_BLE_delay - TEMPS_ENVOIE * 2 - 30);
    }
}

void singleTask_BLE(){
    unsigned long start = millis();

        // Initialisation des tableaux avec valeur par défaut 0xFF
        int temperature[MAX_BLE] = {0xFF};
        int humidite[MAX_BLE] = {0xFF};

        // Remplit manuellement tous les indices avec 0xFF
        for (int i = 0; i < MAX_BLE; i++) {
            temperature[i] = 0xFF;
            humidite[i] = 0xFF;
        }

        // Verrouille les données pour lecture sécurisée
        if (xSemaphoreTake(data_mutex, portMAX_DELAY) == pdTRUE) 
        {   
            for (int i = 0; i < MAX_TOTAL_SENSORS; i++) {
                if (sensorTable[i].present) {
                    // Calcul d’un index unique pour chaque capteur
                    int index = sensorTable[i].is_mux ?
                        MAX_SENSORS_PER_MUX * sensorTable[i].bus + sensorTable[i].mux_channel + 1 + sensorTable[i].bus :
                        MAX_SENSORS_PER_MUX * sensorTable[i].bus + sensorTable[i].bus;

                    // Stocke les valeurs dans les tableaux BLE
                    temperature[index] = round_to_half_integer(sensorTable[i].temperature) * 2;
                    humidite[index] = sensorTable[i].humidity;
                }
            }

            // Ajout du capteur ambiant (SHT) à la fin du tableau
            if (shtPresent) {
                temperature[MAX_BLE - 1] = round_to_half_integer(sharedTempAmbiante) * 2;
                humidite[MAX_BLE - 1] = sharedHumiditeAmbiante;
            }

            xSemaphoreGive(data_mutex);     
        }

        // Création et envoi des trames BLE
        message_humidite = creerMessage(humidite, 0xAA, MAX_BLE);
        message_temperature = creerMessage(temperature, 0xEE, MAX_BLE);

        // Envoie humidité
        updateAdvertisementData(message_humidite);
        advertising->start();
        delay(TEMPS_ENVOIE);
        advertising->stop();

        // Petite pause pour éviter chevauchement
        delay(30);

        // Envoie température
        updateAdvertisementData(message_temperature);
        advertising->start();
        delay(TEMPS_ENVOIE);
        advertising->stop();

}