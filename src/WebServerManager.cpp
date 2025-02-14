#include "WebServerManager.h"
#include "WebHandlers.h"
#include <WiFi.h>
#include <esp_wifi.h>

class WebServerManager::WiFiEventHandler {
public:
    uint8_t retry_count = 0;
};

WebServerManager::WebServerManager() 
    : _currentMode(ServerMode::UNDEFINED)
    , _connectionStatus(ConnectionStatus::DISCONNECTED)
    , _initialized(false)
    , _lastReconnectAttempt(0)
    , _wifiHandler(new WiFiEventHandler()) {
    _preferences.begin("wifi_config", false);
    setupWiFiEventHandlers();
}

WebServerManager::~WebServerManager() {
    delete _wifiHandler;
}

void WebServerManager::setupWiFiEventHandlers() {
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch(event) {
            case SYSTEM_EVENT_STA_START:
                updateConnectionStatus(ConnectionStatus::CONNECTING);
                break;

            case SYSTEM_EVENT_STA_CONNECTED:
                Serial.println("WiFi connected, waiting for IP");
                break;

            case SYSTEM_EVENT_STA_GOT_IP:
                updateConnectionStatus(ConnectionStatus::CONNECTED);
                _wifiHandler->retry_count = 0;
                
                if (_currentMode == ServerMode::PORTAL) {
                    startPreferencesMode();
                }
                break;

            case SYSTEM_EVENT_STA_DISCONNECTED: {
                uint8_t reason = info.wifi_sta_disconnected.reason;
                Serial.printf("WiFi disconnected, reason: %d\n", reason);
                updateConnectionStatus(ConnectionStatus::DISCONNECTED);
                
                static uint8_t retry_count = 0;
                if (_currentMode == ServerMode::PREFERENCES) {
                    if (retry_count < 5) {
                        Serial.println("Attempting reconnection...");
                        retry_count++;
                        WiFi.begin();
                        return;
                    }
                }
                
                if (_currentMode != ServerMode::PORTAL) {
                    Serial.println("Max retries reached, switching to AP mode");
                    retry_count = 0;
                    startPortalMode();
                }
                break;
            }
        }
    });
}

bool WebServerManager::begin() {
    if (_initialized) return true;
    
    Serial.println("Starting WebServerManager initialization...");
    
    _server = std::unique_ptr<WebServer>(new WebServer(80));
    _initialized = true;

    if (WiFi.status() == WL_CONNECTED) {
        return startPreferencesMode();
    } else {
        return startPortalMode();
    }
}

void WebServerManager::handleClient() {
    if (!_server) {
        Serial.println("Error: Server is null in handleClient");
        return;
    }
    
    // Add debug logging
    static unsigned long lastLog = 0;
    if (millis() - lastLog > 5000) {  // Log every 5 seconds
        Serial.printf("WebServer status - Mode: %d, Connected: %d\n", 
            static_cast<int>(_currentMode),
            WiFi.status() == WL_CONNECTED);
        lastLog = millis();
    }
    
    if (_dnsServer && _currentMode == ServerMode::PORTAL) {
        _dnsServer->processNextRequest();
    }
    
    _server->handleClient();
}

void WebServerManager::stop() {
    if (_server) {
        _server->stop();
    }
    stopDNSServer();
    WiFi.disconnect(true);
}

bool WebServerManager::startPortalMode() {
    if (!_initialized || !_server) return false;
    
    clearHandlers();
    stopDNSServer();
    _currentMode = ServerMode::PORTAL;
    
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    delay(100);
    
    WiFi.mode(WIFI_AP);
    
    IPAddress local_IP(192,168,4,1);
    IPAddress gateway(192,168,4,1);
    IPAddress subnet(255,255,255,0);
    
    if (!WiFi.softAPConfig(local_IP, gateway, subnet)) {
        Serial.println("AP Config Failed");
        return false;
    }
    
    if (!WiFi.softAP(WIFI_SETUP_AP_NAME, WIFI_SETUP_PASSWORD)) {
        Serial.println("AP Start Failed");
        return false;
    }
    
    setupHandlers();  // Register all handlers
    startDNSServer();
    _server->begin();
    
    Serial.println("Portal mode started successfully");
    return true;
}


bool WebServerManager::startPreferencesMode() {
    if (!_initialized || !_server) return false;
    
    clearHandlers();
    stopDNSServer();
    _currentMode = ServerMode::PREFERENCES;
    
    setupHandlers();  // Register all handlers
    _server->begin();
    
    Serial.println("Preferences mode started successfully");
    return true;
}

void WebServerManager::setupPortalHandlers() {
    _server->on("/", HTTP_GET, handleRoot);
    _server->on("/scan", HTTP_GET, handleScan);
    _server->on("/connect", HTTP_POST, handleConnect);
    _server->on("/icon.svg", HTTP_GET, handleIcon);
    _server->onNotFound(handleCaptivePortal);
}

void WebServerManager::setupPreferencesHandlers() {
    if (!_server) return;

    // Existing handlers...

    // Add relay control handlers
    _server->on("/api/preferences/relay", HTTP_GET, [this]() {
        auto& handler = RelayControlHandler::getInstance();
        
        StaticJsonDocument<256> doc;
        JsonArray relays = doc.createNestedArray("relays");
        
        for (uint8_t i = 0; i < RelayControlHandler::NUM_RELAYS; i++) {
            RelayStatus status = handler.getRelayStatus(i);
            JsonObject relay = relays.createNestedObject();
            relay["relay_id"] = i;
            relay["state"] = status.state == RelayState::ON ? "ON" : "OFF";
            relay["override"] = status.override;
        }
        
        String response;
        serializeJson(doc, response);
        this->_server->send(200, "application/json", response);
    });

    _server->on("/api/preferences/relay", HTTP_POST, [this]() {
        if (!this->_server->hasArg("plain")) {
            this->_server->send(400, "application/json", "{\"error\":\"No data received\"}");
            return;
        }

        StaticJsonDocument<128> doc;
        DeserializationError error = deserializeJson(doc, this->_server->arg("plain"));
        
        if (error) {
            this->_server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        if (!doc.containsKey("relay_id") || !doc.containsKey("state")) {
            this->_server->send(400, "application/json", "{\"error\":\"Missing required fields\"}");
            return;
        }

        uint8_t relayId = doc["relay_id"].as<uint8_t>();
        const char* stateStr = doc["state"].as<const char*>();
        
        if (relayId >= RelayControlHandler::NUM_RELAYS) {
            this->_server->send(400, "application/json", "{\"error\":\"Invalid relay ID\"}");
            return;
        }

        RelayState newState = (strcmp(stateStr, "ON") == 0) ? RelayState::ON : RelayState::OFF;
        RelayControlHandler::getInstance().processCommand(relayId, newState, RelayCommandSource::USER);
        
        this->_server->send(200, "application/json", "{\"success\":true}");
    });
}

void WebServerManager::setupHandlers() {
    if (!_server) return;

    // Register core handlers
    _server->on("/", HTTP_GET, handleRoot);
    _server->on("/scan", HTTP_GET, handleScan);
    _server->on("/connect", HTTP_POST, handleConnect);
    _server->on("/icon.svg", HTTP_GET, handleIcon);
    
    // Register API handlers
    _server->on("/api/preferences", HTTP_GET, handleGetPreferences);
    _server->on("/api/preferences", HTTP_POST, handleSetPreferences);
    _server->on("/api/preferences", HTTP_OPTIONS, handleOptionsPreferences);
    
    // Register relay handlers
    _server->on("/api/relay", HTTP_GET, handleGetRelayState);
    _server->on("/api/relay", HTTP_POST, handleSetRelayState);
    _server->on("/api/relay", HTTP_OPTIONS, [this]() {
        if (_server) {
            addCorsHeaders();
            _server->send(204);
        }
    });

    // Must be last - handle captive portal and 404s
    _server->onNotFound(handleCaptivePortal);
}


void WebServerManager::addCorsHeaders() {
    if (!_server) return;
    _server->sendHeader("Access-Control-Allow-Origin", "*");
    _server->sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    _server->sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void WebServerManager::setWiFiCredentials(const String& ssid, const String& password) {
    _preferences.putString("ssid", ssid);
    _preferences.putString("password", password);
}

bool WebServerManager::hasStoredCredentials() {
    String ssid = _preferences.getString("ssid", "");
    return ssid.length() > 0;
}

void WebServerManager::clearCredentials() {
    _preferences.remove("ssid");
    _preferences.remove("password");
}

bool WebServerManager::connectWithStoredCredentials() {
    String ssid = _preferences.getString("ssid", "");
    String password = _preferences.getString("password", "");
    
    if (ssid.length() == 0) return false;
    
    Serial.printf("Attempting to connect to SSID: %s\n", ssid.c_str());
    
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
    
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    
    wifi_ps_type_t psType = WIFI_PS_NONE;
    esp_wifi_set_ps(psType);
    
    WiFi.begin(ssid.c_str(), password.c_str());
    
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startAttempt > CONNECTION_TIMEOUT) {
            Serial.println("Connection attempt timed out");
            return false;
        }
        delay(100);
    }
    
    if (!validateConnection()) {
        Serial.println("Connection validation failed");
        return false;
    }
    
    Serial.printf("Successfully connected to WiFi. IP: %s\n", WiFi.localIP().toString().c_str());
    return true;
}

bool WebServerManager::reconnect() {
    if (!hasStoredCredentials()) return false;
    
    updateConnectionStatus(ConnectionStatus::CONNECTING);
    return connectWithStoredCredentials();
}

void WebServerManager::updateConnectionStatus(ConnectionStatus status) {
    if (_connectionStatus != status) {
        _connectionStatus = status;
        if (_statusCallback) {
            _statusCallback(status);
        }
    }
}

bool WebServerManager::validateConnection() {
    if (WiFi.status() != WL_CONNECTED) return false;
    
    IPAddress ip = WiFi.localIP();
    if (ip[0] == 0) return false;
    
    WiFi.setAutoReconnect(true);
    
    return true;
}

void WebServerManager::clearHandlers() {
    if (_server) {
        _server->stop();
        _server = std::unique_ptr<WebServer>(new WebServer(80));
    }
}

void WebServerManager::startDNSServer() {
    stopDNSServer();
    _dnsServer = std::unique_ptr<DNSServer>(new DNSServer());
    _dnsServer->start(53, "*", WiFi.softAPIP());
}

void WebServerManager::stopDNSServer() {
    if (_dnsServer) {
        _dnsServer->stop();
        _dnsServer.reset();
    }
}

WebServerManager& WebServerManager::getInstance() {
    static WebServerManager instance;
    return instance;
}

