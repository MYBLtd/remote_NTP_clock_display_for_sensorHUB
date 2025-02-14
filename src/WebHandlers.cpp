#include "WebHandlers.h"
#include "DisplayHandler.h"
#include "icons.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include <ArduinoJson.h>
#include "GlobalState.h"
#include "PreferencesManager.h"
#include "RelayControlHandler.h"
#include <base64.h>

extern GlobalState* g_state;
extern PreferencesManager prefsManager;

void addCorsHeaders(WebServer* server) {
    if (!server) return;
    server->sendHeader("Access-Control-Allow-Origin", "*");
    server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void handleRoot() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    // Add cache control headers
    server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server->sendHeader("Pragma", "no-cache");
    server->sendHeader("Expires", "-1");

    // Serve the appropriate page based on the connection state
    if (webManager.isInAPMode()) {
        server->send(200, "text/html", SETUP_PAGE_HTML);
    } else {
        server->send(200, "text/html", PREFERENCES_PAGE_HTML);
    }
}

void handleScan() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    String networks = getNetworksJson();
    server->send(200, "application/json", networks);
}

String getNetworksJson() {
    String json = "[";
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + 
                String(WiFi.RSSI(i)) + ",\"encrypted\":" + 
                String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false") + "}";
    }
    json += "]";
    return json;
}

void handleConnect() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    String ssid = server->arg("ssid");
    String password = server->arg("password");
    
    if (ssid.isEmpty()) {
        server->send(400, "text/plain", "SSID is required");
        return;
    }

    webManager.setWiFiCredentials(ssid, password);
    
    if (webManager.connectWithStoredCredentials()) {
        server->send(200, "text/plain", "Connected successfully. Device will restart...");
        delay(1000);
        ESP.restart();
    } else {
        server->send(500, "text/plain", "Failed to connect to WiFi");
    }
}

void handleGetPreferences() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    DisplayHandler* display = g_state->getDisplay();
    if (!display) {
        addCorsHeaders(server);
        server->send(500, "application/json", "{\"success\":false,\"error\":\"Display not initialized\"}");
        return;
    }

    DisplayPreferences prefs = display->getDisplayPreferences();
    
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    
    JsonObject data = doc.createNestedObject("data");
    data["nightDimming"] = prefs.nightModeDimmingEnabled;
    data["dayBrightness"] = map(constrain(prefs.dayBrightness, 0, 255), 0, 255, 1, 25);
    data["nightBrightness"] = map(constrain(prefs.nightBrightness, 0, 255), 0, 255, 1, 25);
    data["nightStartHour"] = prefs.nightStartHour;
    data["nightEndHour"] = prefs.nightEndHour;
    
    String response;
    serializeJson(doc, response);
    
    addCorsHeaders(server);
    server->send(200, "application/json", response);
}

void handleSetPreferences() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    addCorsHeaders(server);

    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"No data received\"}");
        return;
    }

    String jsonData = server->arg("plain");
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    try {
        DisplayHandler* display = g_state->getDisplay();
        if (!display) {
            server->send(500, "application/json", 
                "{\"success\":false,\"error\":\"Display not initialized\"}");
            return;
        }

        DisplayPreferences prefs;
        prefs.nightModeDimmingEnabled = doc["nightDimming"].as<bool>();
        
        int dayBrightness = doc["dayBrightness"].as<int>();
        int nightBrightness = doc["nightBrightness"].as<int>();
        
        if (dayBrightness < 1 || dayBrightness > 25 || 
            nightBrightness < 1 || nightBrightness > 25) {
            server->send(400, "application/json", 
                "{\"success\":false,\"error\":\"Brightness must be between 1 and 25\"}");
            return;
        }

        prefs.dayBrightness = map(dayBrightness, 1, 25, 1, 75);
        prefs.nightBrightness = map(nightBrightness, 1, 25, 1, 75);

        prefs.nightStartHour = doc["nightStartHour"].as<uint8_t>();
        prefs.nightEndHour = doc["nightEndHour"].as<uint8_t>();
        
        if (prefs.nightStartHour >= 24 || prefs.nightEndHour >= 24) {
            server->send(400, "application/json", 
                "{\"success\":false,\"error\":\"Hours must be between 0 and 23\"}");
            return;
        }

        display->setDisplayPreferences(prefs);
        PreferencesManager::saveDisplayPreferences(prefs);

        server->send(200, "application/json", "{\"success\":true}");

    } catch (const std::exception& e) {
        String errorMsg = "{\"success\":false,\"error\":\"Internal error: ";
        errorMsg += e.what();
        errorMsg += "\"}";
        server->send(500, "application/json", errorMsg);
    }
}

void handleOptionsPreferences() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;
    
    addCorsHeaders(server);
    server->send(204);
}

void handleCaptivePortal() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    if (webManager.isInAPMode() && server->hostHeader() != WiFi.softAPIP().toString()) {
        server->sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
        server->send(302, "text/plain", "");
    } else {
        handleRoot();
    }
}

void handleIcon() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    String path = server->uri();
    String iconName = path.substring(path.lastIndexOf('/') + 1);
    
    addCorsHeaders(server);
    server->sendHeader("Cache-Control", "public, max-age=31536000");
    server->sendHeader("Content-Type", "image/svg+xml");
    
    const char* iconContent = nullptr;
    
    if (iconName == "lock") {
        iconContent = LOCK_ICON;
    } 
    else if (iconName == "signal-1") {
        iconContent = SIGNAL_WEAK;
    }
    else if (iconName == "signal-2") {
        iconContent = SIGNAL_FAIR;
    }
    else if (iconName == "signal-3") {
        iconContent = SIGNAL_GOOD;
    }
    else if (iconName == "signal-4") {
        iconContent = SIGNAL_STRONG;
    }
    
    if (iconContent) {
        server->send(200, "image/svg+xml", iconContent);
    } else {
        server->send(404, "text/plain", "Icon not found");
    }
}

void handleGetRelayState() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    addCorsHeaders(server);
    
    auto& relayHandler = RelayControlHandler::getInstance();
    String response;
    
    if (!relayHandler.getRelayStates(response)) {
        server->send(500, "application/json", "{\"error\":\"Failed to fetch relay status\"}");
        return;
    }

    // Pass the response directly since it's already in JSON format
    server->send(200, "application/json", response);
}

void handleSetRelayState() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    addCorsHeaders(server);

    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"No data received\"}");
        return;
    }

    String jsonData = server->arg("plain");
    Serial.printf("Received relay command: %s\n", jsonData.c_str());

    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc.containsKey("relay_id") || !doc.containsKey("state")) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"Missing relay_id or state\"}");
        return;
    }

    uint8_t relayId = doc["relay_id"].as<uint8_t>();
    String stateStr = doc["state"].as<String>();
    Serial.printf("Setting relay %d to %s\n", relayId, stateStr.c_str());

    auto& relayHandler = RelayControlHandler::getInstance();
    RelayState newState = (stateStr == "ON") ? RelayState::ON : RelayState::OFF;
    
    // Use setState which already handles authentication internally
    if (relayHandler.setState(relayId, newState)) {
        server->send(200, "application/json", "{\"success\":true}");
    } else {
        server->send(500, "application/json", 
            "{\"success\":false,\"error\":\"Failed to set relay state\"}");
    }
}

void handleRelayControl() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    if (server->method() == HTTP_OPTIONS) {
        addCorsHeaders(server);
        server->send(204);
        return;
    }

    if (server->method() == HTTP_GET) {
        handleGetRelayState();
        return;
    }

    if (server->method() == HTTP_POST) {
        handleSetRelayState();
        return;
    }

    server->send(405, "application/json", "{\"success\":false,\"error\":\"Method not allowed\"}");
}

void setupWebHandlers() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    // Basic handlers
    server->on("/", HTTP_GET, handleRoot);
    server->onNotFound(handleCaptivePortal);

    // WiFi configuration handlers
    server->on("/scan", HTTP_GET, handleScan);
    server->on("/connect", HTTP_POST, handleConnect);
    
    // Device preferences handlers
    server->on("/api/preferences", HTTP_GET, handleGetPreferences);
    server->on("/api/preferences", HTTP_POST, handleSetPreferences);
    server->on("/api/preferences", HTTP_OPTIONS, handleOptionsPreferences);

    // Relay control handlers
    server->on("/api/relay", HTTP_GET, handleGetRelayState);
    server->on("/api/relay", HTTP_POST, handleSetRelayState);
    server->on("/api/relay", HTTP_OPTIONS, []() {
        auto& webManager = WebServerManager::getInstance();
        WebServer* server = webManager.getServer();
        if (!server) return;
        
        addCorsHeaders(server);
        server->send(204);
    });

    // Icon handler
    server->on("/icon.svg", HTTP_GET, handleIcon);

    Serial.println("Web handlers initialized");
}