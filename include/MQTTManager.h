// MQTTManager.h
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <functional>
#include "certificates.h"

class MQTTManager {
public:
    static MQTTManager& getInstance() {
        static MQTTManager instance;
        return instance;
    }

    MQTTManager();
    void begin();
    bool maintainConnection();
    bool connected();
    bool publish(const char* topic, const char* payload, bool retained = true);
    bool subscribe(const char* topic);

    // Callback type to match PubSubClient
    using MQTTCallback = std::function<void(const String& topic, const String& payload)>;
    void setCallback(std::function<void(char*, uint8_t*, unsigned int)> callback);
    void handleMessage(char* topic, byte* payload, unsigned int length);

private:
    WiFiClientSecure wifiClient;
    PubSubClient mqttClient;
    String clientId;
    unsigned long lastReconnectAttempt;
    unsigned long lastPublishTime;
    unsigned int currentReconnectDelay;
    MQTTCallback userCallback;

    bool connect();
    void setupSecureClient();
    void logState(const char* context);
    void handleCallback(char* topic, byte* payload, unsigned int length);

    static constexpr unsigned int INITIAL_RECONNECT_DELAY = 1000;
    static constexpr unsigned int MAX_RECONNECT_DELAY = 60000;
    static constexpr unsigned int PUBLISH_RATE_LIMIT = 100;
    static constexpr unsigned int RECONNECT_INTERVAL = 5000;
};
