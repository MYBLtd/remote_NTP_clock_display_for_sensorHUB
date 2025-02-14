/**
 * BME280Handler.h
 * 
 * This class provides a wrapper around the Bosch BME280 sensor driver,
 * offering a clean interface for sensor operations while handling the
 * low-level details of sensor communication.
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "bme280.h"
#include "config.h"
#include <esp_task_wdt.h>
#include <esp_core_dump.h>
#include <esp_task_wdt.h>
#include <esp_system.h>

// BME280 I2C Address (default)
#define BME280_I2C_ADDR               UINT8_C(0x76)  // or 0x77 depending on SDO pin

// Reset command
#define BME280_RESET_CMD              UINT8_C(0xB6)

// BME280 sensor constants
static constexpr float BME280_INVALID_TEMP = -999.0f;  // Invalid temperature indicator
static constexpr float BME280_INVALID_HUM = -999.0f;   // Invalid humidity indicator
static constexpr float BME280_INVALID_PRES = -999.0f;  // Invalid pressure indicator

// Valid reading ranges based on BME280 datasheet
constexpr float BME280_TEMP_MIN = -40.0f;    // Minimum operating temperature
constexpr float BME280_TEMP_MAX = 85.0f;     // Maximum operating temperature
constexpr float BME280_HUM_MIN = 0.0f;       // Minimum humidity reading
constexpr float BME280_HUM_MAX = 100.0f;     // Maximum humidity reading
constexpr float BME280_PRES_MIN = 300.0f;    // Minimum pressure in hPa
constexpr float BME280_PRES_MAX = 1100.0f;   // Maximum pressure in hPa

class BME280Handler {
private:
    uint8_t deviceAddress;
    static constexpr float INVALID_TEMP = BME280_INVALID_TEMP;
    
public:
    BME280Handler(); // Remove the inline implementation
    
    bool init();
    bool takeMeasurement();
    
    // Sensor reading accessors
    float getTemperature() const { return temperature; }
    float getHumidity() const { return humidity; }
    float getPressure() const { return pressure; }
    bool isValid() const { return sensorValid; }
    unsigned long getLastReadTime() const { return lastReadTime; }
    
private:
    // Bosch BME280 driver structure
    struct bme280_dev bme280;
    struct bme280_calib_data calibData;  // Calibration data structure
    
    int32_t rawPressure;    // Raw pressure reading
    int32_t rawTemperature; // Raw temperature reading
    int32_t rawHumidity;    // Raw humidity reading
    
    // Cached sensor readings
    float temperature;
    float humidity;
    float pressure;
    bool sensorValid;
    unsigned long lastReadTime;
    
    // Private helper methods
    bool readCalibrationData();
    float compensateTemperature(int32_t adc_T);
    float compensatePressure(int32_t adc_P);
    float compensateHumidity(int32_t adc_H);
    
    static int8_t i2cRead(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
    static int8_t i2cWrite(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
    static void delayMs(uint32_t period, void *intf_ptr);
    bool validateReadings(float temp, float hum, float pres);
    void setupSensorSettings();
    bool initI2C();
    bool tryAddress(uint8_t address);
    void processRawMeasurements(uint8_t* buffer);
};