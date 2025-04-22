import json
import sqlite3
import paho.mqtt.client as mqtt
import os
import time

DEBUG = True
TIME_MIN_SEC = 2.5
# SQLite database setup
conn = sqlite3.connect("SerreVo.db", timeout=10)
cur = conn.cursor()
cur.execute('''
CREATE TABLE IF NOT EXISTS TemperatureData (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    Timestamp TEXT,
    SensorA TEXT,
    SensorA_1 TEXT,
    SensorA_2 TEXT,
    SensorA_3 TEXT,
    SensorA_4 TEXT,
    SensorB TEXT,
    SensorB_1 TEXT,
    SensorB_2 TEXT,
    SensorB_3 TEXT,
    SensorB_4 TEXT,
    SensorC TEXT,
    SensorC_1 TEXT,
    SensorC_2 TEXT,
    SensorC_3 TEXT,
    SensorC_4 TEXT,
    SensorD TEXT,
    SensorD_1 TEXT,
    SensorD_2 TEXT,
    SensorD_3 TEXT,
    SensorD_4 TEXT,
    SensorE TEXT,
    SensorE_1 TEXT,
    SensorE_2 TEXT,
    SensorE_3 TEXT,
    SensorE_4 TEXT,
    SensorAir TEXT
)
''')

cur.execute('''
CREATE TABLE IF NOT EXISTS HumidityData (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    Timestamp TEXT,
    SensorA TEXT,
    SensorA_1 TEXT,
    SensorA_2 TEXT,
    SensorA_3 TEXT,
    SensorA_4 TEXT,
    SensorB TEXT,
    SensorB_1 TEXT,
    SensorB_2 TEXT,
    SensorB_3 TEXT,
    SensorB_4 TEXT,
    SensorC TEXT,
    SensorC_1 TEXT,
    SensorC_2 TEXT,
    SensorC_3 TEXT,
    SensorC_4 TEXT,
    SensorD TEXT,
    SensorD_1 TEXT,
    SensorD_2 TEXT,
    SensorD_3 TEXT,
    SensorD_4 TEXT,
    SensorE TEXT,
    SensorE_1 TEXT,
    SensorE_2 TEXT,
    SensorE_3 TEXT,
    SensorE_4 TEXT,
    SensorAir TEXT
)
''')
conn.execute("PRAGMA journal_mode=WAL;")  # Enable WAL mode
conn.commit()
conn.close()

# MQTT Broker details
HOST = "cgs.altecoop.ca"
PORT = 1883
USERNAME = "projetTSO"
PASSWORD = "LesProjets2025SontCool!!!"
SUBSCRIBE_TOPIC = "CegepSherbrooke/iot/Minew/gw/ac233fc1151c/#"

# Print received data
def print_data(timestamp, raw_data):
    os.system('cls' if os.name == 'nt' else 'clear')
    print(f"\nTimestamp: {timestamp}\nRaw Data: {raw_data}\n")

last_timestamp_hum = None
last_timestamp_temp = None
# Process data and store it
def process_data(data):
    global last_timestamp_hum, last_timestamp_temp  # Access global variables
    timestamp = data.get("timestamp", "N/A")
    raw_data = data.get("rawData", "")

    # Separate the components
    header = raw_data[:6]  # First 6 hex characters
    indicator = raw_data[6:8]  # Next 2 hex characters
    data_part = raw_data[8:]  # Remaining data
    temp = []
    hum = []

    if(indicator == "EE"):
        # Compute time difference
        current_time = time.time()
        delta_time = current_time - last_timestamp_temp if last_timestamp_temp else 99

        if(delta_time > TIME_MIN_SEC):
            for i in range(0, len(data_part), 2):  # Step by 2 to read full bytes
                byte_str = data_part[i:i+2]  # Extract two hex characters
                byte_value = int(byte_str, 16)  # Convert to integer

                if byte_str == "C8":
                    temp.append(0)  # Special case: C8 means 0°C
                elif byte_str == "FF":
                    temp.append(-1)  # Special case: FF means sensor not present
                else:
                    temp.append(byte_value / 2)  # Convert to temperature
            while len(temp) < 27:
                temp.append(-1)  # Fill missing values with NULL

            print(f"Temp: PROCESSED : {delta_time:.2f} {temp}")
            last_timestamp_temp = current_time  # Update last timestamp

            # Open database connection here (inside the loop for each insert)
            conn = sqlite3.connect("SerreVo.db", timeout=10)
            cur = conn.cursor()

            # Insert into the database
            cur.execute('''
            INSERT INTO TemperatureData (Timestamp, SensorA, SensorA_1, SensorA_2, SensorA_3, SensorA_4,
                                        SensorB, SensorB_1, SensorB_2, SensorB_3, SensorB_4,
                                        SensorC, SensorC_1, SensorC_2, SensorC_3, SensorC_4,
                                        SensorD, SensorD_1, SensorD_2, SensorD_3, SensorD_4,
                                        SensorE, SensorE_1, SensorE_2, SensorE_3, SensorE_4,
                                        SensorAir)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ''', (timestamp, *temp))
            conn.commit()
            conn.close()

    elif indicator == "AA":  # Humidity data
         # Compute time difference
        current_time = time.time()
        delta_time = current_time - last_timestamp_hum if last_timestamp_hum else 99

        if(delta_time > TIME_MIN_SEC):
            for i in range(0, len(data_part), 2):  # Step by 2 to read full bytes
                byte_str = data_part[i:i+2]  # Extract two hex characters
                byte_value = int(byte_str, 16)  # Convert to integer

                if byte_str == "C8":
                    hum.append(0)  # Special case: C8 means 0°C
                elif byte_str == "FF":
                    hum.append(-1)  # Special case: FF means sensor not present
                else:
                    hum.append(byte_value)  # humidity
            while len(hum) < 27:
                hum.append(-1)  # Fill missing values with NULL

            print(f"Hum: PROCESSED : {delta_time:.2f} {hum}")
            last_timestamp_hum = current_time  # Update last timestamp

            # Open database connection here (inside the loop for each insert)
            conn = sqlite3.connect("SerreVo.db", timeout=10)
            cur = conn.cursor()
                
            # Insert into the database
            cur.execute('''
            INSERT INTO HumidityData (Timestamp, SensorA, SensorA_1, SensorA_2, SensorA_3, SensorA_4,
                                        SensorB, SensorB_1, SensorB_2, SensorB_3, SensorB_4,
                                        SensorC, SensorC_1, SensorC_2, SensorC_3, SensorC_4,
                                        SensorD, SensorD_1, SensorD_2, SensorD_3, SensorD_4,
                                        SensorE, SensorE_1, SensorE_2, SensorE_3, SensorE_4,
                                       SensorAir)
            VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ''', (timestamp, *hum))
            conn.commit()
            conn.close()

    

# MQTT callbacks
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker")
        client.subscribe(SUBSCRIBE_TOPIC)
    else:
        print(f"Failed to connect: {rc}")

def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode())
        if isinstance(payload, dict):
            process_data(payload)
        elif isinstance(payload, list):
            for item in payload:
                process_data(item)
    except Exception as e:
        print(f"Error processing message: {e}")

# MQTT client setup
client = mqtt.Client()
client.username_pw_set(USERNAME, PASSWORD)
client.on_connect = on_connect
client.on_message = on_message

try:
    client.connect(HOST, PORT, keepalive=60)
    client.loop_forever()
except Exception as e:
    print(f"Connection error: {e}")
