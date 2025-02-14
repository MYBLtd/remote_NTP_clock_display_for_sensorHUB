#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include "config.h"

enum class RelayCommandSource {
    USER,
    MQTT
};

enum class RelayState {
    OFF,
    ON,
    UNKNOWN
};

struct RelayCommand {
    uint8_t relayId;
    RelayState state;
    RelayCommandSource source;
    unsigned long timestamp;
};

struct RelayStatus {
    RelayState state;
    bool override;
};

class RelayControlHandler {
public:
    static RelayControlHandler& getInstance();

    bool begin();
    void processCommand(uint8_t relayId, RelayState state, RelayCommandSource source);
    RelayStatus getRelayStatus(uint8_t relayId) const;
    bool isOverridden(uint8_t relayId) const { return userOverride[relayId]; }
    void clearOverride(uint8_t relayId) { userOverride[relayId] = false; }
    
    static void handleMqttMessage(const String& topic, const String& payload);
    bool setState(uint8_t relayId, RelayState newState);
    bool setState(bool on);
    bool getState();
    bool getOverride();
    void printRelayStatus();
    bool getRelayStates(String& response);
    String getAuthToken() const { return authToken; }
    
    static constexpr uint8_t NUM_RELAYS = 2;  // Total number of relays

private:
    static RelayControlHandler* instance;

    RelayControlHandler();
    RelayControlHandler(const RelayControlHandler&) = delete;
    RelayControlHandler& operator=(const RelayControlHandler&) = delete;

    bool authenticate();  // Single authenticate method declaration
    bool refreshAuthToken();
    bool makeAuthenticatedRequest(const char* endpoint, const char* method, const char* payload);
    
    String authToken;
    unsigned long tokenExpiry;
    RelayState currentState[NUM_RELAYS];
    bool userOverride[NUM_RELAYS];
    SemaphoreHandle_t relayMutex;
    QueueHandle_t commandQueue;
    unsigned long lastStateChange[NUM_RELAYS];
    
    static constexpr size_t QUEUE_SIZE = 10;
    static constexpr TickType_t COMMAND_TIMEOUT = pdMS_TO_TICKS(5000);
    
    bool relayState;
};