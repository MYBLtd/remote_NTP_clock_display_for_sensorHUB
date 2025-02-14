#include "BabelSensor.h"
#include "config.h"

BabelSensor::BabelSensor(const char* url) 
    : serverUrl(url), lastUpdate(0), lastTemperature(0.0) {
    Serial.println("BabelSensor initialized with URL: " + String(url));
}

bool BabelSensor::init() {
    Serial.println("BabelSensor init called");
    return true;
}

float BabelSensor::getRemoteTemperature() {
    unsigned long now = millis();
    if (now - lastUpdate < UPDATE_INTERVAL) {
        return lastTemperature;
    }
    
    if (!isAuthenticated()) {
        Serial.println("Not authenticated, attempting login...");
        if (!login(API_USERNAME, API_PASSWORD)) {
            Serial.println("Login failed, returning last temperature");
            return lastTemperature;
        }
    }
    
    HTTPClient http;
    String sensorUrl = serverUrl + String(API_SENSORS_ENDPOINT);
    
    http.begin(sensorUrl);
    http.addHeader("Authorization", "Bearer " + authToken);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String response = http.getString();
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error) {
            bool babelSensorFound = false;
            JsonArray array = doc.as<JsonArray>();
            
            for (JsonVariant sensor : array) {
                if (sensor["isBabelSensor"].as<bool>()) {
                    babelSensorFound = true;
                    float newTemp = sensor["babelTemperature"].as<float>();
                    if (newTemp != lastTemperature) {
                        lastTemperature = newTemp;
                        Serial.printf("BabelSensor temperature updated: %.2f\n", lastTemperature);
                    }
                    lastUpdate = now;
                    break;
                }
            }
            if (!babelSensorFound) {
                Serial.println("No BabelSensor found in response");
            }
        }
    } else {
        Serial.printf("HTTP request failed, code: %d\n", httpCode);
    }
    
    http.end();
    return lastTemperature;
}
bool BabelSensor::login(const char* username, const char* password) {
    HTTPClient http;
    String loginUrl = serverUrl + String(API_LOGIN_ENDPOINT);
    
    http.begin(loginUrl);
    http.addHeader("Content-Type", "application/json");
    
    StaticJsonDocument<200> credentials;
    credentials["username"] = username;
    credentials["password"] = password;
    
    String requestBody;
    serializeJson(credentials, requestBody);
    
    int httpCode = http.POST(requestBody);
    Serial.printf("Login attempt - HTTP response code: %d\n", httpCode);
    
    if (httpCode == 200) {
        String response = http.getString();
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, response);
        
        if (!error && doc.containsKey("token")) {
            authToken = doc["token"].as<String>();
            Serial.println("Authentication successful, token received");
            http.end();
            return true;
        }
        Serial.println("Invalid authentication response format");
    } else {
        Serial.printf("Authentication failed with code: %d\n", httpCode);
    }
    
    http.end();
    return false;
}
