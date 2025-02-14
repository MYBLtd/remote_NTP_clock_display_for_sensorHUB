#include "PreferencesManager.h"
#include <SPIFFS.h>

// Initialize static members
PreferenceStorage* PreferencesManager::storage = nullptr;
SemaphoreHandle_t PreferencesManager::prefsMutex = nullptr;
PreferencesManager::PreferencesChangedCallback PreferencesManager::onPreferencesChanged = nullptr;

void PreferencesManager::begin() {
    // Enhanced SPIFFS initialization with detailed logging
    if (!SPIFFS.begin(true)) {
        Serial.println("[CRITICAL] Failed to mount SPIFFS filesystem");
        return;
    }

    // Create base preferences directory if it doesn't exist
    if (!SPIFFS.exists(SPIFFSPreferenceStorage::getBasePath())) {
        if (SPIFFS.mkdir(SPIFFSPreferenceStorage::getBasePath())) {
            Serial.println("[INFO] Created base preferences directory");
        } else {
            Serial.println("[ERROR] Failed to create base preferences directory");
        }
    }

    prefsMutex = xSemaphoreCreateMutex();
    if (!prefsMutex) {
        Serial.println("[CRITICAL] Failed to create preferences mutex");
        return;
    }

    if (!storage) {
        storage = new SPIFFSPreferenceStorage();
    }
    
    if (!storage) {
        Serial.println("[CRITICAL] Failed to create preferences storage");
        return;
    }

    if (!storage->begin("display", false)) {
        Serial.println("[ERROR] Failed to begin preferences storage");
        return;
    }

    Serial.println("[SUCCESS] Preferences system initialized");
}

void PreferencesManager::setPreferencesChangedCallback(PreferencesChangedCallback callback) {
    onPreferencesChanged = callback;
}

void PreferencesManager::saveDisplayPreferences(const DisplayPreferences& prefs) {
    if (!storage || !prefsMutex) {
        Serial.println("Preferences system not initialized");
        return;
    }

    if (xSemaphoreTake(prefsMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        storage->putBool("nightMode", prefs.nightModeDimmingEnabled);
        
        // Store actual 1-75 range values
        uint8_t dayBright = constrain(prefs.dayBrightness, 1, 75);
        uint8_t nightBright = constrain(prefs.nightBrightness, 1, 75);
        
        storage->putUChar("dayBright", dayBright);
        storage->putUChar("nightBright", nightBright);
        storage->putUChar("nightStart", prefs.nightStartHour);
        storage->putUChar("nightEnd", prefs.nightEndHour);
        
        Serial.printf("Saving display preferences - Day: %d%%, Night: %d%%\n", 
                     dayBright, nightBright);
        
        xSemaphoreGive(prefsMutex);

        if (onPreferencesChanged) {
            onPreferencesChanged(prefs);
        }
    }
}

DisplayPreferences PreferencesManager::loadDisplayPreferences() {
    DisplayPreferences prefs;
    
    if (!storage || !prefsMutex) {
        Serial.println("[ERROR] Preferences system not initialized when loading");
        return prefs;
    }

    if (xSemaphoreTake(prefsMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Log detailed loading process
        Serial.println("[DEBUG] Loading display preferences");
        
        prefs.nightModeDimmingEnabled = storage->getBool("nightMode", false);
        Serial.printf("[DEBUG] Night mode dimming: %s\n", 
                     prefs.nightModeDimmingEnabled ? "Enabled" : "Disabled");
        
        uint8_t rawDayBright = storage->getUChar("dayBright", 75);
        uint8_t rawNightBright = storage->getUChar("nightBright", 10);
        
        prefs.dayBrightness = constrain(rawDayBright, 1, 75);
        prefs.nightBrightness = constrain(rawNightBright, 1, 25);
        
        prefs.nightStartHour = storage->getUChar("nightStart", 22);
        prefs.nightEndHour = storage->getUChar("nightEnd", 6);
        
        Serial.printf("[DEBUG] Loaded preferences:\n"
                     "  Night Mode: %s\n"
                     "  Day Brightness: %d%%\n"
                     "  Night Brightness: %d%%\n"
                     "  Night Start: %d\n"
                     "  Night End: %d\n",
                     prefs.nightModeDimmingEnabled ? "Enabled" : "Disabled",
                     prefs.dayBrightness,
                     prefs.nightBrightness,
                     prefs.nightStartHour,
                     prefs.nightEndHour);
        
        xSemaphoreGive(prefsMutex);
    }

    return prefs;
}

