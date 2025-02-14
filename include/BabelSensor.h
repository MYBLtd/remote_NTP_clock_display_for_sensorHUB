#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WString.h>

class BabelSensor {
    public:
        BabelSensor(const char* serverUrl);
        bool init();
        bool login(const char* username, const char* password);
        float getRemoteTemperature();
        bool isAuthenticated() const { return !authToken.isEmpty(); }
    
    private:
        String serverUrl;
        String authToken;
        unsigned long lastUpdate;
        float lastTemperature;
        static constexpr unsigned long UPDATE_INTERVAL = 30000; // 30 seconds
};