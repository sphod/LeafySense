#ifndef AHT20_SENSOR_H
#define AHT20_SENSOR_H

#include <Wire.h>
#include <Adafruit_AHTX0.h>

// Data structure to hold sensor readings and status
struct AHT20_Data {
  float temperature;
  float humidity;
  bool sensor_found;
  String last_error;
};

// Function declarations
bool initAHT20();
AHT20_Data readAHT20();
void printAHT20Data(const AHT20_Data& data);

extern AHT20_Data currentAHT20Data; // Global variable to store latest readings

#endif