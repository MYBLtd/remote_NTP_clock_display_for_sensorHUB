// TaskManager.h
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include "config.h"
#include "GlobalState.h"
#include "PreferencesManager.h"
#include "displayHandler.h"
#include "auth_manager.h"
#include "rate_limiter.h"
#include "MQTTManager.h"

class TaskManager {
public:
    static bool initializeTasks();
    static void startWatchdog();
    static void stopTasks();
    
private:
    static void createTask(
        TaskFunction_t taskFunction,
        const char* taskName,
        uint32_t stackSize,
        UBaseType_t priority,
        TaskHandle_t* taskHandle,
        BaseType_t core
    );
    
    static void monitorTaskStacks();
    static void configureWatchdog();
};

// Task function declarations
void displayTask(void* parameter);
void sensorTask(void* parameter);
void networkTask(void* parameter);
void watchdogTask(void* parameter);