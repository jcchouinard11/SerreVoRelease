# README

## Description
Cette partie du projet s'occupe de récupérer les donnés envoyés par BLE grâce à un bridge BLE/MQTT et les stocker dans une base de donné. Ces donnés sont alors afficher sur une page web à l'aide de Grafana
---

## Structure des fichiers

### Scripts Python
- `ScriptMQTT/MQTT_decodage.py` : Code principal qui récupère les données MQTT et les insère dans la base de données.
- `Grafana/dashboardSerreVO.json` : Ficher de dashboard pour import

### Bases de données SQLite
- `SerreVo` : Fichier de base de données principal utilisé pour stocker les informations.


### Documentation
- `README.md` : Ce fichier, expliquant la structure et l'utilisation du projet.

---

## Utilisation

1. Assurez-vous que votre broker MQTT est opérationnel et que le bridge BLE/MQTT envoie correctement les données.
2. Exécutez `MQTT_decodage.py` dans un terminal pour collecter et stocker les données.
3. Vous pourrez visualiser les données grâce à SQLite  studio ou Grafana.

---

---

Aidé par ChatGPT