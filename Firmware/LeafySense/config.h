#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

// WiFi Configuration
#define WIFI_AP_SSID_PREFIX "SmartGarden_"
#define WIFI_AP_PASSWORD ""  // Leave empty for open network
#define WIFI_AP_IP "192.168.4.1"
#define CAPTIVE_PORTAL_DNS "setup.smartgarden"

// Reset Button Configuration
#define RESET_BUTTON_PIN 9
#define RESET_HOLD_TIME 5000    // 5 seconds
#define RESET_CONFIRM_TIME 10000 // 10 seconds

// Existing configurations...
extern String WIFI_SSID;
extern String WIFI_PASSWORD; 
// I2C Configuration for AHT20 and ADS1115
#define I2C_SDA_PIN 6
#define I2C_SCL_PIN 7
#define I2C_FREQUENCY 100000  // 100kHz

// ADS1115 Configuration
#define ADS1115_I2C_ADDRESS 0x48  // Default I2C address

// ADS1115 Channel Assignments
#define SOIL_MOISTURE_1_CHANNEL 0  // A0 - First soil moisture sensor
#define SOIL_TEMP_1_CHANNEL 1      // A1 - First soil temperature (NTC thermistor)
#define SOIL_MOISTURE_2_CHANNEL 2  // A2 - Second soil moisture sensor  
#define SOIL_TEMP_2_CHANNEL 3      // A3 - Second soil temperature (NTC thermistor)

// Sensor Calibration Values (adjust based on your sensors)
#define SOIL_MOISTURE_DRY 25000    // ADC value when dry - will need calibration
#define SOIL_MOISTURE_WET 15000    // ADC value when wet - will need calibration

// NTC Thermistor Configuration
extern const float R_FIXED;        // 10K series resistor
extern const float BETA;           // B-constant for B3950 NTC
extern const float T0;             // 25°C in Kelvin (298.15K = 25°C)
extern const float R0;             // 10K at 25°C
extern const float VCC;            // System voltage

// ADS1115 Gain Settings
#define ADS1115_GAIN GAIN_TWOTHIRDS  // ±6.144V range (187.5µV per bit)

// WS2812B LED Configuration
#define WS2812B_PIN 3
#define NUM_LEDS 1

// MQTT Configuration - Make them mutable
extern String MQTT_SERVER;        // Change from const char*
extern int MQTT_PORT;             // Change from const int
extern String MQTT_USER;          // Change from const char*
extern String MQTT_PASSWORD;      // Change from const char*
extern String MQTT_CLIENT_ID;     // Change from const char*
extern String MQTT_TOPIC_PREFIX;  // Change from const char*

// NTP Configuration
extern const char* NTP_SERVER;
extern const long GMT_OFFSET_SEC;
extern const int DAYLIGHT_OFFSET_SEC;

// Sensor Reading Intervals
#define SENSOR_READ_INTERVAL 5000    // Read sensors every 5 seconds
#define MQTT_PUBLISH_INTERVAL 30000  // Publish to MQTT every 30 seconds

// OTA Configuration
#define CURRENT_FIRMWARE_VERSION "1.0.1"
#define GITHUB_OWNER "sphod"
#define GITHUB_REPO "test"
#define FIRMWARE_ASSET_NAME "SmartGarden_ESP32C6.ino.bin"
#define UPDATE_CHECK_INTERVAL 10000  // 5 minutes in milliseconds

#endif