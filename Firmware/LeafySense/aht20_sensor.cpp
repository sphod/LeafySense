#include "aht20_sensor.h"
#include "config.h"
#include <Arduino.h>

// Global sensor object and data
Adafruit_AHTX0 aht;
AHT20_Data currentAHT20Data = {0.0, 0.0, false, ""};

bool initAHT20() {
  Serial.println("🔍 Initializing AHT20 sensor...");
  
  // Initialize I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQUENCY);
  
  // Try to initialize the sensor
  if (!aht.begin()) {
    currentAHT20Data.last_error = "Failed to find AHT20 chip";
    currentAHT20Data.sensor_found = false;
    Serial.println("❌ AHT20 initialization failed!");
    Serial.println("   Please check wiring: SDA->GPIO" + String(I2C_SDA_PIN) + ", SCL->GPIO" + String(I2C_SCL_PIN));
    return false;
  }
  
  currentAHT20Data.sensor_found = true;
  currentAHT20Data.last_error = "";
  Serial.println("✅ AHT20 sensor initialized successfully!");
  return true;
}

AHT20_Data readAHT20() {
  AHT20_Data data = {0.0, 0.0, currentAHT20Data.sensor_found, ""};
  
  if (!data.sensor_found) {
    data.last_error = "Sensor not initialized";
    return data;
  }
  
  sensors_event_t humidity, temp;
  
  if (!aht.getEvent(&humidity, &temp)) {
    data.last_error = "Failed to read data from AHT20";
    Serial.println("❌ Failed to read data from AHT20");
    return data;
  }
  
  data.temperature = temp.temperature;
  data.humidity = humidity.relative_humidity;
  data.last_error = "";
  
  // Update global data
  currentAHT20Data = data;
  
  return data;
}

void printAHT20Data(const AHT20_Data& data) {
  Serial.println("=== AHT20 Sensor Readings ===");
  
  if (!data.sensor_found) {
    Serial.println("   Status: Sensor not found!");
    return;
  }
  
  if (!data.last_error.isEmpty()) {
    Serial.println("   Status: Error - " + data.last_error);
    return;
  }
  
  Serial.println("   Status: ✅ OK");
  Serial.println("   Temperature: " + String(data.temperature) + " °C");
  Serial.println("   Humidity: " + String(data.humidity) + " %");
  Serial.println("==============================");
}