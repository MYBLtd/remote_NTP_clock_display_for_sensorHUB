// MQTTManager.cpp
#include "MQTTManager.h"
#include "config.h"
#include "RelayControlHandler.h"  // Add this include for RelayState enum

MQTTManager::MQTTManager() 
    : wifiClient()
    , mqttClient(wifiClient)
    , lastReconnectAttempt(0)
    , lastPublishTime(0)
    , currentReconnectDelay(INITIAL_RECONNECT_DELAY) {
    setupSecureClient();
}

void MQTTManager::begin() {
    wifiClient.setCACert(letsencrypt_root_ca);
    wifiClient.setHandshakeTimeout(15000);
    
    mqttClient.setBufferSize(8192);
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setSocketTimeout(15);
    mqttClient.setKeepAlive(60);
    
    uint8_t mac[6];
    WiFi.macAddress(mac);
    clientId = String("NTPClock-") + String(mac[3], HEX) + String(mac[4], HEX) + String(mac[5], HEX);
    
    Serial.printf("MQTT: Configured for broker %s:%d with client ID %s\n", 
                 MQTT_BROKER, MQTT_PORT, clientId.c_str());
}

bool MQTTManager::maintainConnection() {
    if (!connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= RECONNECT_INTERVAL) {
            lastReconnectAttempt = now;
            mqttClient.disconnect();
            wifiClient.stop();
            delay(100);
            
            if (connect()) {
                currentReconnectDelay = INITIAL_RECONNECT_DELAY;
                return true;
            } else {
                currentReconnectDelay = min(currentReconnectDelay * 2, MAX_RECONNECT_DELAY);
                return false;
            }
        }
        return false;
    }
    
    mqttClient.loop();
    return true;
}

bool MQTTManager::connected() {
    return mqttClient.connected();
}

bool MQTTManager::publish(const char* topic, const char* payload, bool retained) {
    if (!connected()) {
        Serial.println("MQTT: Cannot publish - not connected");
        return false;
    }

    unsigned long now = millis();
    if (now - lastPublishTime < PUBLISH_RATE_LIMIT) {
        delay(50);
    }

    const int maxRetries = 3;
    for (int retry = 0; retry < maxRetries; retry++) {
        if (retry > 0) {
            delay((1 << retry) * 200);
        }

        if (mqttClient.publish(topic, payload, retained)) {
            lastPublishTime = millis();
            return true;
        }
        
        Serial.printf("MQTT: Publish attempt %d failed for topic: %s\n", retry + 1, topic);
    }
    
    return false;
}

bool MQTTManager::connect() {
    if (!wifiClient.connect(MQTT_BROKER, MQTT_PORT)) {
        Serial.println("MQTT: SSL connection failed");
        return false;
    }
    
    String statusTopic = String(MQTT_TOPIC_AUX_DISPLAY) + "/status";
    
    if (mqttClient.connect(clientId.c_str(), 
                          MQTT_USER, 
                          MQTT_PASSWORD,
                          statusTopic.c_str(),
                          1,
                          true,
                          "offline")) {
        // Subscribe to relay command topic
        mqttClient.subscribe("relay/command", 1);
        
        // Set up callback
        mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
            this->handleCallback(topic, payload, length);
        });
        
        Serial.println("MQTT: Connected successfully");
        mqttClient.publish(statusTopic.c_str(), "online", true);
        return true;
    }
    
    Serial.printf("MQTT: Connection failed, rc=%d\n", mqttClient.state());
    return false;
}

bool MQTTManager::subscribe(const char* topic) {
    if (!mqttClient.connected()) {
        Serial.println("MQTT: Cannot subscribe - not connected");
        return false;
    }
    return mqttClient.subscribe(topic, 1);
}

void MQTTManager::setCallback(std::function<void(char*, uint8_t*, unsigned int)> callback) {
    mqttClient.setCallback(callback);
}

void MQTTManager::setupSecureClient() {
    wifiClient.setCACert(letsencrypt_root_ca);
}

void MQTTManager::logState(const char* context) {
    const char* stateStr;
    switch(mqttClient.state()) {
        case -4: stateStr = "TIMEOUT"; break;
        case -3: stateStr = "LOST"; break;
        case -2: stateStr = "FAILED"; break;
        case -1: stateStr = "DISCONNECTED"; break;
        case 0: stateStr = "CONNECTED"; break;
        case 1: stateStr = "BAD_PROTOCOL"; break;
        case 2: stateStr = "BAD_CLIENT_ID"; break;
        case 3: stateStr = "UNAVAILABLE"; break;
        case 4: stateStr = "BAD_CREDENTIALS"; break;
        case 5: stateStr = "UNAUTHORIZED"; break;
        default: stateStr = "UNKNOWN"; break;
    }
    Serial.printf("MQTT State [%s]: %s (%d)\n", context, stateStr, mqttClient.state());
}

// callback handling:
void MQTTManager::handleMessage(char* topic, byte* payload, unsigned int length) {
    String message((char*)payload, length);
    
    // Parse JSON message
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.print(F("JSON parsing failed: "));
        Serial.println(error.c_str());
        return;
    }
    
    // Check if we have the required fields
    if (!doc.containsKey("relay_id") || !doc.containsKey("state")) {
        Serial.println(F("Missing required fields in MQTT message"));
        return;
    }
    
    uint8_t relayId = doc["relay_id"].as<uint8_t>();
    const char* stateStr = doc["state"].as<const char*>();
    
    RelayState state = (strcmp(stateStr, "ON") == 0) ? RelayState::ON : RelayState::OFF;
    
    // Forward command to relay handler
    RelayControlHandler::getInstance().processCommand(relayId, state, RelayCommandSource::MQTT);
}

void MQTTManager::handleCallback(char* topic, byte* payload, unsigned int length) {
    // Convert payload to string
    String payloadStr;
    payloadStr.reserve(length);
    for(unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    
    // Call user callback if set
    if (userCallback) {
        userCallback(String(topic), payloadStr);
    }
}
