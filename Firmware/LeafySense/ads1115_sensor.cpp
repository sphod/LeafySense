#include "ads1115_sensor.h"
#include "config.h"
#include <Arduino.h>

// Global ADS1115 object and data
Adafruit_ADS1115 ads1115;
ADS1115_Data currentADS1115Data;

bool initADS1115() {
  Serial.println("🔍 Initializing ADS1115 ADC...");
  
  // Initializing the struct with default values
  currentADS1115Data.ads1115_found = false;
  currentADS1115Data.last_error = "";
  
  // Initialize sensor1
  currentADS1115Data.sensor1.raw_moisture = 0;
  currentADS1115Data.sensor1.raw_temperature = 0;
  currentADS1115Data.sensor1.moisture_percentage = 0.0;
  currentADS1115Data.sensor1.temperature_celsius = 0.0;
  currentADS1115Data.sensor1.sensor_working = false;
  currentADS1115Data.sensor1.last_error = "Not initialized";
  
  // Initialize sensor2
  currentADS1115Data.sensor2.raw_moisture = 0;
  currentADS1115Data.sensor2.raw_temperature = 0;
  currentADS1115Data.sensor2.moisture_percentage = 0.0;
  currentADS1115Data.sensor2.temperature_celsius = 0.0;
  currentADS1115Data.sensor2.sensor_working = false;
  currentADS1115Data.sensor2.last_error = "Not initialized";
  
  if (!ads1115.begin(ADS1115_I2C_ADDRESS, &Wire)) {
    currentADS1115Data.last_error = "Failed to find ADS1115 chip";
    currentADS1115Data.ads1115_found = false;
    Serial.println("❌ ADS1115 initialization failed!");
    Serial.println("   Please check wiring and I2C address");
    return false;
  }
  
  ads1115.setGain(ADS1115_GAIN);
  
  currentADS1115Data.ads1115_found = true;
  currentADS1115Data.last_error = "";
  currentADS1115Data.sensor1.last_error = "";
  currentADS1115Data.sensor2.last_error = "";
  
  Serial.println("✅ ADS1115 initialized successfully!");
  Serial.println("   Gain: TWOTHIRDS (±6.144V), Resolution: 0.1875mV");
  return true;
}

SoilSensorData readSoilSensor(uint8_t moisture_channel, uint8_t temp_channel, const char* sensor_name) {
  SoilSensorData data;
  
  // Initialize with default values
  data.raw_moisture = 0;
  data.raw_temperature = 0;
  data.moisture_percentage = 0.0;
  data.temperature_celsius = 0.0;
  data.sensor_working = true;
  data.last_error = "";
  
  // Read soil moisture (raw ADC value)
  int16_t moisture_adc = ads1115.readADC_SingleEnded(moisture_channel);
  if (moisture_adc == 0x7FFF) { // Error value from library
    data.sensor_working = false;
    data.last_error = "Failed to read moisture channel " + String(moisture_channel);
    return data;
  }
  
  // Read soil temperature (raw ADC value)
  int16_t temp_adc = ads1115.readADC_SingleEnded(temp_channel);
  if (temp_adc == 0x7FFF) { // Error value from library
    data.sensor_working = false;
    data.last_error = "Failed to read temperature channel " + String(temp_channel);
    return data;
  }
  
  data.raw_moisture = moisture_adc;
  data.raw_temperature = temp_adc;
  
  // Calculate moisture percentage
  data.moisture_percentage = calculateMoisturePercentage(moisture_adc);
  
  // Calculate temperature using your working formula
  data.temperature_celsius = readTemperatureFromADC(temp_adc);
  
  data.sensor_working = true;
  data.last_error = "";
  
  return data;
}

ADS1115_Data readAllSoilSensors() {
  ADS1115_Data data;
  data.ads1115_found = currentADS1115Data.ads1115_found;
  data.last_error = "";
  
  if (!data.ads1115_found) {
    data.last_error = "ADS1115 not initialized";
    return data;
  }
  
  // Read first soil sensor pair (A0 & A1)
  data.sensor1 = readSoilSensor(SOIL_MOISTURE_1_CHANNEL, SOIL_TEMP_1_CHANNEL, "Soil Sensor 1");
  
  // Small delay between readings
  delay(10);
  
  // Read second soil sensor pair (A2 & A3)  
  data.sensor2 = readSoilSensor(SOIL_MOISTURE_2_CHANNEL, SOIL_TEMP_2_CHANNEL, "Soil Sensor 2");
  
  // Update global data
  currentADS1115Data = data;
  
  return data;
}

float calculateMoisturePercentage(int raw_value) {
  // Capacitive soil moisture sensors typically read lower values when wet
  // Invert and map to percentage
  int inverted_value = SOIL_MOISTURE_DRY - raw_value;
  int range = SOIL_MOISTURE_DRY - SOIL_MOISTURE_WET;
  
  // Avoid division by zero
  if (range <= 0) range = 1;
  
  float percentage = (inverted_value * 100.0) / range;
  
  // Constrain to 0-100%
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  
  return percentage;
}

// working temperature calculation function
float readTemperatureFromADC(int16_t adcValue) {
  // ADS1115 with GAIN_TWOTHIRDS: 1 LSB = 0.1875mV = 0.0001875V
  float voltage = adcValue * 0.0001875; // V

  // Avoid division by zero for invalid voltage readings
  if (voltage >= VCC || voltage <= 0) {
    return -100.0; // Error value
  }

  // Voltage divider: Vout = Vcc * (R_ntc / (R_ntc + R_fixed))
  // Solve for R_ntc:
  float R_ntc = (voltage * R_FIXED) / (VCC - voltage);

  // Avoid invalid resistance values
  if (R_ntc <= 0) {
    return -100.0; // Error value
  }

  // Compute Temperature using Beta equation
  float invT = (1.0 / T0) + (1.0 / BETA) * log(R_ntc / R0);
  float T = 1.0 / invT;       // Kelvin
  float tempC = T - 273.15;   // Celsius

  return tempC;
}

void printADS1115Data(const ADS1115_Data& data) {
  Serial.println("=== ADS1115 Soil Sensor Readings ===");
  
  if (!data.ads1115_found) {
    Serial.println("   Status: ADS1115 not found!");
    return;
  }
  
  if (!data.last_error.isEmpty()) {
    Serial.println("   Status: Error - " + data.last_error);
    return;
  }
  
  // Sensor 1 data (A0: Moisture, A1: Temperature)
  Serial.println("🌱 Soil Sensor 1 (A0=Moisture, A1=Temp):");
  if (data.sensor1.sensor_working) {
    Serial.println("   Moisture - Raw: " + String(data.sensor1.raw_moisture) + 
                   ", Percentage: " + String(data.sensor1.moisture_percentage, 1) + "%");
    Serial.println("   Temperature - Raw: " + String(data.sensor1.raw_temperature) +
                   ", Temp: " + String(data.sensor1.temperature_celsius, 1) + "°C");
  } else {
    Serial.println("   Status: ❌ " + data.sensor1.last_error);
  }
  
  // Sensor 2 data (A2: Moisture, A3: Temperature)  
  Serial.println("🌱 Soil Sensor 2 (A2=Moisture, A3=Temp):");
  if (data.sensor2.sensor_working) {
    Serial.println("   Moisture - Raw: " + String(data.sensor2.raw_moisture) + 
                   ", Percentage: " + String(data.sensor2.moisture_percentage, 1) + "%");
    Serial.println("   Temperature - Raw: " + String(data.sensor2.raw_temperature) +
                   ", Temp: " + String(data.sensor2.temperature_celsius, 1) + "°C");
  } else {
    Serial.println("   Status: ❌ " + data.sensor2.last_error);
  }
  
  Serial.println("====================================");
}