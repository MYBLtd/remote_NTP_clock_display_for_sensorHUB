//SystemDefinitions.h
#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Forward declaration
class GlobalState;

// External declarations for global variables
extern GlobalState* g_state;
extern QueueHandle_t sensorQueue;
extern QueueHandle_t displayQueue;
extern TaskHandle_t displayTaskHandle;
extern TaskHandle_t sensorTaskHandle;
extern TaskHandle_t networkTaskHandle;
extern TaskHandle_t watchdogTaskHandle;

// Display modes for the 7-segment display
enum class DisplayMode {
    TIME,           // Shows current time
    DATE,           // Shows current date
    TEMPERATURE,    // Shows local temperature
    HUMIDITY,       // Shows humidity
    PRESSURE,       // Shows barometric pressure
    REMOTE_TEMP     // Shows remote temperature
};

// Display preferences for brightness control and night mode
struct DisplayPreferences {
    bool nightModeDimmingEnabled = false;
    uint8_t dayBrightness = 255;
    uint8_t nightBrightness = 32;
    uint8_t nightStartHour = 22;
    uint8_t nightEndHour = 6;
};

// Structure for BME280 sensor data
struct BME280Data {
    float temperature;
    float humidity;
    float pressure;
    bool valid;
    unsigned long timestamp;
};

// Structure for system status information
struct SystemStatus {
    bool wifiConnected;
    bool mqttConnected;
    bool bmeWorking;
    bool ntpSynced;
    uint32_t freeHeap;
    uint32_t uptime;
    float temperature;
    float humidity;
    float pressure;
    float remoteTemperature;
    unsigned long lastUpdate;
};