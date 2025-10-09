#ifndef DATA_STRUCTURES_HPP
#define DATA_STRUCTURES_HPP

struct BATTERY_DATA {
    float busVoltage;
    float shuntVoltage;
    float current;
    float power;
    // float maxBusVoltage;
    // float minBusVoltage;
    // float maxCurrent;
    // float minCurrent;
};

struct FRAME_DATA {
    int wifiStrength;
    float temperature;
    float humidity;
    int batteryLevel;
    BATTERY_DATA bd;
};

#endif // DATA_STRUCTURES_HPP