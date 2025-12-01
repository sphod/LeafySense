#include "config.h"

// Default values - will be overwritten by captive portal settings
String WIFI_SSID = "";
String WIFI_PASSWORD = "";

// NTC Thermistor Configuration
const float R_FIXED = 10000.0;    // 10K series resistor
const float BETA = 3950.0;        // B-constant for B3950 NTC
const float T0 = 298.15;          // 25°C in Kelvin (298.15K = 25°C)
const float R0 = 10000.0;         // 10K at 25°C
const float VCC = 3.3;            // System voltage

// MQTT Configuration - will be set via captive portal
String MQTT_SERVER = "";
int MQTT_PORT = 1883;
String MQTT_USER = "";           //MQTT Username
String MQTT_PASSWORD = "";          //MQTT Password
String MQTT_CLIENT_ID = "smartgarden_esp32c6";          //MQTT Client ID
String MQTT_TOPIC_PREFIX = "smartgarden";           //MQTT Topic Prefix

// NTP Configuration
const char* NTP_SERVER = "pool.ntp.org";
// Change GMT offset based on your timezone
const long GMT_OFFSET_SEC = 5 * 3600 + 30 * 60;  // GMT+5:30 for India
const int DAYLIGHT_OFFSET_SEC = 0;