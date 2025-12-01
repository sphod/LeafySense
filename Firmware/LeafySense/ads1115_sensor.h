// ads1115_sensor.h
#ifndef ADS1115_SENSOR_H
#define ADS1115_SENSOR_H

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

// Soil sensor data structure
struct SoilSensorData {
  int raw_moisture;
  int raw_temperature;
  float moisture_percentage;
  float temperature_celsius;
  bool sensor_working;
  String last_error;
};

// ADS1115 data structure
struct ADS1115_Data {
  SoilSensorData sensor1;  // A0 (moisture) & A1 (temp)
  SoilSensorData sensor2;  // A2 (moisture) & A3 (temp)
  bool ads1115_found;
  String last_error;
};

// Function declarations
bool initADS1115();
ADS1115_Data readAllSoilSensors();
SoilSensorData readSoilSensor(uint8_t moisture_channel, uint8_t temp_channel, const char* sensor_name);
float calculateMoisturePercentage(int raw_value);
float readTemperatureFromADC(int16_t adcValue);
void printADS1115Data(const ADS1115_Data& data);

extern ADS1115_Data currentADS1115Data; // Global variable to store latest readings

#endif