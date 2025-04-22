# 📟 Projet de Gestion de Capteurs avec Affichage LVGL et BLE

Ce projet est une implémentation sur microcontrôleur permettant la gestion de plusieurs capteurs (température et humidité) via des bus I2C multiplexés, l'affichage des données sur un écran tactile avec LVGL, ainsi que la transmission Bluetooth Low Energy (BLE).

## 📌 Fonctionnalités

- **Gestion de capteurs multiples** : 
  - Supporte plusieurs bus I2C via `FlexWire`
  - Gestion de multiplexeurs pour l'ajout de plusieurs capteurs
  - Lecture des données depuis des capteurs `Adafruit_seesaw` et `SHT31`
- **Interface graphique avec LVGL** :
  - Affichage des mesures sur un écran TFT
  - Gestion tactile avec `XPT2046`
- **Communication BLE** :
  - Serveur BLE pour l'envoi des mesures vers un appareil mobile ou un ordinateur
  - Publicité BLE avec UUID spécifique
- **Gestion des tâches concurrentes** :
  - Synchronisation des accès aux capteurs avec `Semaphore`
  - Mise à jour de l'affichage et des données via `FreeRTOS`

## 🛠 Matériel Utilisé

- Microcontrôleur compatible avec **Arduino**
- Capteur de température et d'humidité **SHT31**
- Capteurs capacitifs **Adafruit Seesaw**
- Multiplexeur **I2C (TCA9548A ou équivalent)**
- Écran **TFT 240x320** avec contrôleur **ILI9341**
- Contrôleur tactile **XPT2046**
- Module **Bluetooth Low Energy (BLE)** intégré ou externe

## 🏗 Bibliothèques Requises

Avant de compiler, assure-toi d'installer les bibliothèques suivantes dans **Arduino IDE** ou **PlatformIO** :

- `FlexWire`
- `lvgl`
- `Adafruit_seesaw`
- `Adafruit_SHT31`
- `XPT2046_Touchscreen`
- `TFT_eSPI`
- `BLEDevice`

## 🚀 Installation et Compilation

1. **Cloner le projet** :
   ```sh
   git clone https://github.com/ton-repo/gestion-capteurs.git
   cd gestion-capteurs
