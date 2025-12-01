#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Forward declarations
struct AHT20_Data;
struct ADS1115_Data;

// Function declarations
void initMQTT();
bool connectMQTT();
void publishSensorData(const AHT20_Data& ahtData, const ADS1115_Data& soilData);
void mqttLoop();
bool isMQTTConnected();
void checkMQTTConnection();
void mqttCallback(char* topic, byte* payload, unsigned int length);
bool shouldPublishMQTT();

// Topic generators
String getTopic(const String& subtopic);
String getAirTempTopic();
String getAirHumidityTopic();
String getSoilMoistureTopic(int sensorNum);
String getSoilTempTopic(int sensorNum);
String getStatusTopic();

#endif