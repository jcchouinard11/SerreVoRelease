#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H
class SensorManager {
public struct SensorData {
    uint8_t index;
    uint8_t bus;
    uint8_t mux_channel;
    bool is_mux;
    float humidity;
    float temperature;
};

public SensorData sensorTable[MAX_TOTAL_SENSORS];
#endif