#include "RelayControlHandler.h"

// Define static members
RelayControlHandler* RelayControlHandler::instance = nullptr;
RelayControlHandler* g_relayHandler = nullptr;  // Global instance

RelayControlHandler::RelayControlHandler() {
    // Initialize arrays
    for (uint8_t i = 0; i < NUM_RELAYS; i++) {
        currentState[i] = RelayState::UNKNOWN;
        userOverride[i] = false;
        lastStateChange[i] = 0;
    }
    
    relayMutex = xSemaphoreCreateMutex();
    commandQueue = xQueueCreate(QUEUE_SIZE, sizeof(RelayCommand));
    tokenExpiry = 0;
}

RelayControlHandler& RelayControlHandler::getInstance() {
    static RelayControlHandler instance;
    g_relayHandler = &instance;  // Set global pointer
    return instance;
}

bool RelayControlHandler::begin() {
    if (!relayMutex || !commandQueue) {
        Serial.println("Failed to create relay synchronization primitives");
        return false;
    }

    // Initial authentication
    if (!authenticate()) {
        Serial.println("Initial authentication failed, will retry later");
    }

    return true;
}

void RelayControlHandler::handleMqttMessage(const String& topic, const String& payload) {
    StaticJsonDocument<128> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        Serial.println("Failed to parse MQTT message");
        return;
    }

    if (!doc.containsKey("relay_id") || !doc.containsKey("state")) {
        Serial.println("MQTT message missing required fields");
        return;
    }

    uint8_t relayId = doc["relay_id"].as<uint8_t>();
    bool stateValue = doc["state"].as<bool>();
    
    if (relayId >= NUM_RELAYS) {
        Serial.println("Invalid relay ID in MQTT message");
        return;
    }

    RelayState newState = stateValue ? RelayState::ON : RelayState::OFF;
    getInstance().processCommand(relayId, newState, RelayCommandSource::MQTT);
}

bool RelayControlHandler::setState(uint8_t relayId, RelayState newState) {
    if (relayId >= NUM_RELAYS) {
        Serial.println("[RELAY] Error: Invalid relay ID");
        return false;
    }

    // First ensure we have a valid token
    if (authToken.isEmpty() || millis() > tokenExpiry) {
        if (!authenticate()) {
            Serial.println("[RELAY] Failed to authenticate for relay control");
            return false;
        }
    }

    // Prepare the control request
    HTTPClient http;
    String url = String("http://") + SENSORHUB_URL + "/api/relay";
    
    http.begin(url);
    http.addHeader("Authorization", "Bearer " + authToken);
    http.addHeader("Content-Type", "application/json");
    
    // Create payload exactly like the shell script
    StaticJsonDocument<128> doc;
    doc["relay_id"] = relayId;
    doc["state"] = (newState == RelayState::ON);  // Convert to boolean for API
    
    String payload;
    serializeJson(doc, payload);
    
    Serial.printf("[RELAY] Setting relay %d to %s\n", relayId, newState == RelayState::ON ? "ON" : "OFF");
    Serial.printf("[RELAY] Sending payload: %s\n", payload.c_str());
    
    int httpCode = http.POST(payload);
    Serial.printf("[RELAY] HTTP response code: %d\n", httpCode);

    // Handle 401 by re-authenticating once
    if (httpCode == 401) {
        Serial.println("[RELAY] Token expired, re-authenticating...");
        if (authenticate()) {
            http.addHeader("Authorization", "Bearer " + authToken);
            httpCode = http.POST(payload);
            Serial.printf("[RELAY] Retry response code: %d\n", httpCode);
        }
    }

    bool success = (httpCode == 200);
    
    if (success) {
        String response = http.getString();
        Serial.printf("[RELAY] Response: %s\n", response.c_str());
        
        if (xSemaphoreTake(relayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            currentState[relayId] = newState;
            lastStateChange[relayId] = millis();
            xSemaphoreGive(relayMutex);
        }
    } else {
        Serial.printf("[RELAY] Failed with code %d\n", httpCode);
    }
    
    http.end();
    return success;
}

void RelayControlHandler::processCommand(uint8_t relayId, RelayState state, RelayCommandSource source) {
    Serial.printf("[RELAY] Processing command - Relay %d to %s from %s\n",
                 relayId, 
                 state == RelayState::ON ? "ON" : "OFF",
                 source == RelayCommandSource::USER ? "USER" : "MQTT");

    // Apply command directly
    setState(relayId, state);
}

bool RelayControlHandler::getRelayStates(String& response) {
    HTTPClient http;
    String url = String("http://") + SENSORHUB_URL + "/api/relay";
    
    http.begin(url);
    http.addHeader("Authorization", "Bearer " + authToken);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String rawResponse = http.getString();
        
        // Parse and reformat response
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, rawResponse);
        
        if (!error) {
            JsonArray relays = doc.as<JsonArray>();
            for (JsonVariant relay : relays) {
                relay["state"] = relay["state"].as<bool>() ? "ON" : "OFF";
            }
            serializeJson(doc, response);
            http.end();
            return true;
        }
    }
    
    http.end();
    return false;
}

// Authentication methods
bool RelayControlHandler::authenticate() {
    HTTPClient http;
    String url = String("http://") + SENSORHUB_URL + "/api/login";
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<200> doc;
    doc["username"] = API_USERNAME;
    doc["password"] = API_PASSWORD;
    String payload;
    serializeJson(doc, payload);
    
    int httpCode = http.POST(payload);
    
    if (httpCode == 200) {
        String response = http.getString();
        StaticJsonDocument<512> responseDoc;
        DeserializationError error = deserializeJson(responseDoc, response);
        
        if (!error && responseDoc.containsKey("token")) {
            authToken = responseDoc["token"].as<String>();
            tokenExpiry = millis() + TOKEN_REFRESH_INTERVAL;
            http.end();
            return true;
        }
    }
    
    http.end();
    return false;
}

bool RelayControlHandler::makeAuthenticatedRequest(const char* endpoint, const char* method, const char* payload) {
    if (authToken.isEmpty() || millis() > tokenExpiry) {
        if (!authenticate()) {
            return false;
        }
    }

    HTTPClient http;
    String url = String("http://") + SENSORHUB_URL + endpoint;
    
    http.begin(url);
    http.addHeader("Authorization", "Bearer " + authToken);
    http.addHeader("Content-Type", "application/json");
    
    int httpCode;
    if (strcmp(method, "GET") == 0) {
        httpCode = http.GET();
    } else if (strcmp(method, "POST") == 0) {
        httpCode = http.POST(payload);
    } else {
        http.end();
        return false;
    }
    
    if (httpCode == 401) {
        // Token expired, retry once
        authToken = "";
        if (authenticate()) {
            http.addHeader("Authorization", "Bearer " + authToken);
            httpCode = (strcmp(method, "GET") == 0) ? http.GET() : http.POST(payload);
        }
    }
    
    http.end();
    return httpCode == 200;
}