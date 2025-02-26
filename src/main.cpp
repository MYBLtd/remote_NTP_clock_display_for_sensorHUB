#include <Arduino.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include <time.h>
#include "GlobalState.h"
#include "DisplayHandler.h"
#include "BME280Handler.h"
#include "MQTTManager.h"
#include <ESPmDNS.h>
#include "WebServerManager.h"
#include "config.h"
#include "PreferencesManager.h"
#include "RelayControlHandler.h" 
#include "WebHandlers.h"
#include "BabelSensor.h"

// System Constants
constexpr uint32_t BOOT_DELAY_MS = 250;
constexpr uint32_t WIFI_TIMEOUT_MS = 10000;
constexpr uint32_t WDT_TIMEOUT_S = 30;
constexpr uint32_t TASK_STACK_SIZE = 4096;
constexpr uint32_t QUEUE_SIZE = 10;

// External declarations from GlobalDefinitions.cpp
extern GlobalState* g_state;
extern QueueHandle_t displayQueue;
extern QueueHandle_t sensorQueue;
extern RelayControlHandler* g_relayHandler;

// Local global objects that are only used in main.cpp
static DisplayHandler* display = nullptr;
static BME280Handler bme280;
static MQTTManager mqtt;
BabelSensor babelSensor(API_SERVER_URL);

// Task Declarations
void displayTask(void* parameter);
void sensorTask(void* parameter);

static unsigned long lastWdtReset = 0;
static const unsigned long REMOTE_TEMP_UPDATE_INTERVAL = 30000; // 30 seconds between sensor reads
static unsigned long lastRemoteTempUpdate = 0;
static float lastBabelTemp = 0.0;

// Store device ID globally so it can be used for mDNS later
char deviceIdString[5] = {0};

// Function to display the device ID on the 7-segment display
void displayDeviceId() {
    if (!display || !g_state || !g_state->getDisplay()) {
        Serial.println("[ERROR] Cannot display ID - display not initialized");
        return;
    }
    
    // Get MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    int identifier = (mac[4] << 8) | mac[5];  
    snprintf(deviceIdString, sizeof(deviceIdString), "%04X", identifier);
    
    Serial.printf("[INIT] Device identifier: %s\n", deviceIdString);
    
    // Save current brightness setting
    uint8_t currentBrightness = 75; // Default high brightness for ID display
    
    // Force high brightness for ID display
    display->setBrightness(currentBrightness);
    
    // Show identifier for about 8 seconds with walking dot
    const int DOT_INTERVAL_MS = 400;  // Time between dot movements
    const int TOTAL_DISPLAY_TIME = 8000;  // Time to show the ID
    unsigned long startTime = millis();
    int currentDot = 0;
    
    // Convert and show the ID digits first
    for(int i = 0; i < 4; i++) {
        char c = deviceIdString[i];
        int digit;
        if(c >= '0' && c <= '9') {
            digit = c - '0' + CHAR_0;
        } else {
            digit = c - 'A' + CHAR_A;
        }
        display->setDigit(i, digit, false);  // Initially no decimal points
    }
    
    // Keep updating the display with walking dot until time expires
    while(millis() - startTime < TOTAL_DISPLAY_TIME) {
        // Calculate which digit should have the dot
        currentDot = ((millis() - startTime) / DOT_INTERVAL_MS) % 4;
        
        // Update all digits, only setting dp for current position
        for(int i = 0; i < 4; i++) {
            char c = deviceIdString[i];
            int digit;
            if(c >= '0' && c <= '9') {
                digit = c - '0' + CHAR_0;
            } else {
                digit = c - 'A' + CHAR_A;
            }
            display->setDigit(i, digit, (i == currentDot));
        }
        
        display->update();
        delay(50);  // Short delay for smooth animation
    }
    
    Serial.println("[INIT] ID display complete");
}

void initializeDisplayPreferences() {
    Serial.println("[INIT] Loading display preferences from storage");
    
    // Load the preferences from storage
    DisplayPreferences prefs = PreferencesManager::loadDisplayPreferences();
    
    // Log the loaded preferences for debugging
    Serial.printf("[INIT] Loaded preferences: Night Mode=%s, Day=%d%%, Night=%d%%, Start=%d:00, End=%d:00\n",
                 prefs.nightModeDimmingEnabled ? "Enabled" : "Disabled",
                 prefs.dayBrightness,
                 prefs.nightBrightness,
                 prefs.nightStartHour,
                 prefs.nightEndHour);
    
    // Get the display handler and apply the preferences
    DisplayHandler* display = g_state->getDisplay();
    if (display) {
        Serial.println("[INIT] Applying saved preferences to display");
        display->setDisplayPreferences(prefs);
    } else {
        Serial.println("[ERROR] Cannot apply preferences - display not initialized");
    }
}

bool initializeSystem() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Critical: SPIFFS mount failed");
        return false;
    }
    Serial.println("SPIFFS mounted successfully");

    // Initialize Global State
    g_state = &GlobalState::getInstance();
    if (!g_state) {
        Serial.println("Critical: Failed to initialize global state");
        return false;
    }

    // Create System Queues
    displayQueue = xQueueCreate(QUEUE_SIZE, sizeof(DisplayMode));
    sensorQueue = xQueueCreate(QUEUE_SIZE, sizeof(BME280Data));
    if (!displayQueue || !sensorQueue) {
        Serial.println("Critical: Queue creation failed");
        return false;
    }

    // Initialize Display
    display = new DisplayHandler();
    if (!display || !display->init()) {
        Serial.println("Critical: Display initialization failed");
        return false;
    }
    g_state->setDisplay(display);
   
    // Initialize Hardware Interfaces
    if (!Wire.begin(I2C_SDA, I2C_SCL, 100000)) {
        Serial.println("Critical: I2C initialization failed");
        return false;
    }
    delay(BOOT_DELAY_MS);

    // Display device ID before loading preferences
    displayDeviceId();

    // Initialize preferences and apply them after showing the device ID
    PreferencesManager::begin();
    initializeDisplayPreferences();

    return true;
}

void setupRelayControl() {
    auto& webManager = WebServerManager::getInstance();
    WebServer* server = webManager.getServer();
    if (!server) return;

    // Initialize the global relay handler instance
    g_relayHandler = &RelayControlHandler::getInstance();
    if (!g_relayHandler->begin()) {
        Serial.println("Failed to initialize relay control");
        return;
    }
    
    // Register relay web handlers
    server->on("/api/relay", HTTP_GET, handleGetRelayState);
    server->on("/api/relay", HTTP_POST, handleSetRelayState);
    
    Serial.println("Relay control initialized successfully");
}

bool setupNetwork() {
    WiFi.mode(WIFI_STA);
    WiFi.begin();

    Serial.println("Connecting to WiFi...");
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startAttempt > WIFI_TIMEOUT_MS) {
            Serial.println("WiFi connection timeout");
            return false;
        }
        delay(100);
    }

    Serial.println("WiFi connected");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

    // Use the device ID string that was already calculated and displayed
    String mdnsName = String(MDNS_HOSTNAME) + String(deviceIdString);
    
    Serial.printf("Setting up mDNS with name: %s\n", mdnsName.c_str());
    
    if (!MDNS.begin(mdnsName.c_str())) {
        Serial.println("Error setting up mDNS responder!");
    } else {
        MDNS.addService("http", "tcp", 80);
        Serial.printf("mDNS responder started. Device will be accessible at %s.local\n", mdnsName.c_str());
    }

    // Initialize WebServerManager
    Serial.println("Starting WebServerManager...");
    if (!WebServerManager::getInstance().begin()) {
        Serial.println("Failed to initialize WebServerManager");
        return false;
    }
    Serial.println("WebServerManager initialized successfully");

    // Setup time
    configTime(0, 0, NTP_SERVER);
    setenv("TZ", TZ_INFO, 1);
    tzset();

    return true;
}

void createTasks() {
    xTaskCreatePinnedToCore(
        displayTask,
        "DisplayTask",
        TASK_STACK_SIZE,
        nullptr,
        1,
        nullptr,
        1
    );

    xTaskCreatePinnedToCore(
        sensorTask,
        "SensorTask",
        TASK_STACK_SIZE,
        nullptr,
        1,
        nullptr,
        0
    );
}

void setup() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println("System starting...");

    if (!initializeSystem()) {
        Serial.println("System initialization failed");
        ESP.restart();
    }

    // Try to set up network, but continue even if it fails
    if (!setupNetwork()) {
        Serial.println("Network setup failed, continuing without network");
        // Don't restart, continue with offline operation
    }

    // Initialize BME280 sensor
    if (!bme280.init()) {
        Serial.println("Warning: BME280 initialization failed");
        g_state->setBMEWorking(false);
    } else {
        g_state->setBMEWorking(true);
        Serial.println("BME280 initialized successfully");
    }

    // Initialize MQTT
    mqtt.begin();

    // Setup watchdog
    esp_task_wdt_init(WDT_TIMEOUT_S, true);
    esp_task_wdt_add(nullptr);

    // Create system tasks
    createTasks();
    setupRelayControl();
    
    // Register relay callback
    mqtt.setCallback([](char* topic, uint8_t* payload, unsigned int length) {
        // Convert payload to String for easier handling
        String payloadStr((char*)payload, length);
        RelayControlHandler::handleMqttMessage(topic, payloadStr);
    });

    // Initialize BabelSensor
    if (babelSensor.init()) {
        Serial.println("BabelSensor initialized");
        if (babelSensor.login(API_USERNAME, API_PASSWORD)) {
            Serial.println("BabelSensor authenticated successfully");
        } else {
            Serial.println("BabelSensor authentication failed - will retry in loop");
        }
    } else {
        Serial.println("BabelSensor initialization failed");
    }

    Serial.println("Setup complete");
}

void loop() {
    const unsigned long now = millis();

    // Watchdog handling
    if (now - lastWdtReset >= 1000) {
        esp_task_wdt_reset();
        lastWdtReset = now;
    }
    
    // Handle web server requests if network is up
    if (WiFi.status() == WL_CONNECTED) {
        WebServerManager::getInstance().handleClient();
        mqtt.maintainConnection();
    } else {
        // Attempt reconnection periodically if network is down
        static unsigned long lastReconnectAttempt = 0;
        if (now - lastReconnectAttempt > 30000) { // Try every 30 seconds
            Serial.println("Attempting WiFi reconnection");
            WiFi.reconnect();
            lastReconnectAttempt = now;
        }
    }

    // Update remote temperature at fixed interval if network is up
    if (WiFi.status() == WL_CONNECTED && now - lastRemoteTempUpdate >= REMOTE_TEMP_UPDATE_INTERVAL) {
        float remoteTemp = babelSensor.getRemoteTemperature();
        if (remoteTemp != 0.0 && remoteTemp != lastBabelTemp) {
            g_state->setRemoteTemperature(remoteTemp);
            lastBabelTemp = remoteTemp;
        }
        lastRemoteTempUpdate = now;
    }  
    
    delay(10);
}

void displayTask(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(100);  // 10Hz refresh rate
    struct tm timeinfo;
    DisplayMode mode = DisplayMode::TIME;
    
    // Add mutex health check variables
    unsigned long lastMutexCheck = 0;
    const unsigned long MUTEX_CHECK_INTERVAL = 30000; // 30 seconds
    
    while (true) {
        esp_task_wdt_reset();
        unsigned long now = millis();

        // Periodic mutex health check
        if (now - lastMutexCheck >= MUTEX_CHECK_INTERVAL) {
            if (display && !display->isMutexValid()) {
                Serial.println("[CRITICAL ERROR] Display mutex invalid in task!");
            }
            lastMutexCheck = now;
        }

        if (xQueueReceive(displayQueue, &mode, 0) == pdTRUE) {
            display->setMode(mode);
        }

        // Get current mode
        DisplayMode currentMode = display->getCurrentMode();
        
        // Update display based on current mode
        switch(currentMode) {
            case DisplayMode::TIME:
                if (getLocalTime(&timeinfo)) {
                    display->showTime(timeinfo.tm_hour, timeinfo.tm_min);
                }
                break;
            case DisplayMode::DATE:
                if (getLocalTime(&timeinfo)) {
                    display->showDate(timeinfo.tm_mday, timeinfo.tm_mon + 1);
                }
                break;
            case DisplayMode::TEMPERATURE:
                display->showTemperature(g_state->getTemperature());
                break;
            case DisplayMode::HUMIDITY:
                display->showHumidity(g_state->getHumidity());
                break;
            case DisplayMode::PRESSURE:
                display->showPressure(g_state->getPressure());
                break;
            case DisplayMode::REMOTE_TEMP:
                display->showRemoteTemp(g_state->getRemoteTemperature());
                break;
        }

        display->update();
        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}

void sensorTask(void* parameter) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t frequency = pdMS_TO_TICKS(2000);  // 0.5Hz measurement rate
    
    while (true) {
        esp_task_wdt_reset();

        if (g_state->isBMEWorking() && bme280.takeMeasurement()) {
            float temperature = bme280.getTemperature();
            float humidity = bme280.getHumidity();
            float pressure = bme280.getPressure();

            if (temperature != BME280_INVALID_TEMP && 
                humidity != BME280_INVALID_HUM && 
                pressure != BME280_INVALID_PRES) {
                
                g_state->updateSensorData(temperature, humidity, pressure);
                
                if (mqtt.connected()) {
                    char payload[64];
                    snprintf(payload, sizeof(payload), "%.1f", temperature);
                    String sensorTopic = String(MQTT_TOPIC_AUX_DISPLAY) + "/sensors";
                    mqtt.publish(sensorTopic.c_str(), payload);
                }
            }
        }

        vTaskDelayUntil(&lastWakeTime, frequency);
    }
}