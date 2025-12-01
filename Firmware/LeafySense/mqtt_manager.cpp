#include "mqtt_manager.h"
#include "config.h"
#include "wifi_manager.h"
#include "aht20_sensor.h"
#include "ads1115_sensor.h"
#include "ntp_time.h"
#include <ArduinoJson.h>
#include <Arduino.h>

// MQTT client objects
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Internal variables
static unsigned long lastMQTTPublish = 0;
static bool mqttConnected = false;

void initMQTT() {
    Serial.println("📡 Initializing MQTT client...");
    
    // Use configured MQTT server
    mqttClient.setServer(MQTT_SERVER.c_str(), MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setKeepAlive(60);
    Serial.println("✅ MQTT client initialized");
}

bool connectMQTT() {
    if (MQTT_SERVER.length() == 0) {
        Serial.println("⚠️ No MQTT server configured");
        return false;
    }
    
    Serial.print("🔗 Connecting to MQTT broker...");
    
    // Generate unique client ID
    String clientId = MQTT_CLIENT_ID + "_" + String(WiFi.macAddress());
    
    bool connected = false;
    if (MQTT_USER.length() > 0 && MQTT_PASSWORD.length() > 0) {
        connected = mqttClient.connect(clientId.c_str(), MQTT_USER.c_str(), MQTT_PASSWORD.c_str());
    } else {
        connected = mqttClient.connect(clientId.c_str());
    }
    
    if (connected) {
        mqttConnected = true;
        Serial.println(" SUCCESS");
        Serial.println("   Server: " + MQTT_SERVER + ":" + String(MQTT_PORT));
        Serial.println("   Client ID: " + clientId);
        return true;
    } else {
        mqttConnected = false;
        Serial.println(" FAILED");
        Serial.println("   Error: " + String(mqttClient.state()));
        return false;
    }
}

void mqttLoop() {
    mqttClient.loop();
}

bool isMQTTConnected() {
    return mqttClient.connected();
}

void checkMQTTConnection() {
    if (MQTT_SERVER.length() > 0 && !mqttClient.connected()) {
        mqttConnected = false;
        Serial.println("⚠️ MQTT connection lost, attempting to reconnect...");
        connectMQTT();
    }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("📨 Message arrived [");
    Serial.print(topic);
    Serial.print("]: ");
    
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);
}

bool shouldPublishMQTT() {
    return (millis() - lastMQTTPublish >= MQTT_PUBLISH_INTERVAL);
}
// Add this function to mqtt_manager.cpp (before publishSensorData function)
void publishIndividualTopics(const AHT20_Data& ahtData, const ADS1115_Data& soilData, const String& deviceId) {
    String baseTopic = MQTT_TOPIC_PREFIX + "/" + deviceId;
    
    // Air temperature
    if (ahtData.sensor_found) {
        mqttClient.publish((baseTopic + "/air/temperature").c_str(), String(ahtData.temperature).c_str());
        mqttClient.publish((baseTopic + "/air/humidity").c_str(), String(ahtData.humidity).c_str());
    }
    
    // Soil sensor 1
    if (soilData.sensor1.sensor_working) {
        mqttClient.publish((baseTopic + "/soil/1/moisture").c_str(), String(soilData.sensor1.moisture_percentage).c_str());
        mqttClient.publish((baseTopic + "/soil/1/temperature").c_str(), String(soilData.sensor1.temperature_celsius).c_str());
    }
    
    // Soil sensor 2
    if (soilData.sensor2.sensor_working) {
        mqttClient.publish((baseTopic + "/soil/2/moisture").c_str(), String(soilData.sensor2.moisture_percentage).c_str());
        mqttClient.publish((baseTopic + "/soil/2/temperature").c_str(), String(soilData.sensor2.temperature_celsius).c_str());
    }
    
    // Device status
    mqttClient.publish((baseTopic + "/status").c_str(), "online");
    mqttClient.publish((baseTopic + "/wifi_rssi").c_str(), String(WiFi.RSSI()).c_str());
}

void publishSensorData(const AHT20_Data& ahtData, const ADS1115_Data& soilData) {
    if (MQTT_SERVER.length() == 0) return;
    
    if (!mqttConnected) {
        Serial.println("⚠️ MQTT not connected, attempting to reconnect...");
        if (!connectMQTT()) {
            Serial.println("❌ Cannot publish data - MQTT not connected");
            return;
        }
    }
    
    Serial.println("📤 Publishing sensor data to MQTT...");
    
    String timestamp = getTimestamp();
    String deviceId = String(WiFi.macAddress());
    
    // Create JSON document with all sensor data
    StaticJsonDocument<512> doc;
    doc["device_id"] = deviceId;
    doc["timestamp"] = timestamp;
    
    // Air sensor data (AHT20)
    if (ahtData.sensor_found && ahtData.last_error.isEmpty()) {
        JsonObject air = doc.createNestedObject("air");
        air["temperature"] = ahtData.temperature;
        air["humidity"] = ahtData.humidity;
    }
    
    // Soil sensor data
    JsonObject soil = doc.createNestedObject("soil");
    
    // Sensor 1
    if (soilData.sensor1.sensor_working) {
        JsonObject sensor1 = soil.createNestedObject("sensor1");
        sensor1["moisture"] = soilData.sensor1.moisture_percentage;
        sensor1["temperature"] = soilData.sensor1.temperature_celsius;
        sensor1["moisture_raw"] = soilData.sensor1.raw_moisture;
        sensor1["temp_raw"] = soilData.sensor1.raw_temperature;
    }
    
    // Sensor 2
    if (soilData.sensor2.sensor_working) {
        JsonObject sensor2 = soil.createNestedObject("sensor2");
        sensor2["moisture"] = soilData.sensor2.moisture_percentage;
        sensor2["temperature"] = soilData.sensor2.temperature_celsius;
        sensor2["moisture_raw"] = soilData.sensor2.raw_moisture;
        sensor2["temp_raw"] = soilData.sensor2.raw_temperature;
    }
    
    // System info
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["free_heap"] = ESP.getFreeHeap();
    
    // Publish to main topic
    String topic = MQTT_TOPIC_PREFIX + "/" + deviceId + "/sensors";
    String jsonOutput;
    serializeJson(doc, jsonOutput);
    
    if (mqttClient.publish(topic.c_str(), jsonOutput.c_str())) {
        Serial.println("✅ Data published to MQTT");
        Serial.println("   Topic: " + topic);
        Serial.println("   JSON: " + jsonOutput);
    } else {
        Serial.println("❌ Failed to publish data to MQTT");
    }
    
    // Also publish individual topics for easier parsing
    publishIndividualTopics(ahtData, soilData, deviceId);
    
    lastMQTTPublish = millis();
}

// Topic generation functions (simplified)
String getTopic(const String& subtopic) {
    String deviceId = String(WiFi.macAddress());
    return MQTT_TOPIC_PREFIX + "/" + deviceId + "/" + subtopic;
}

String getAirTempTopic() {
    return getTopic("air/temperature");
}

String getAirHumidityTopic() {
    return getTopic("air/humidity");
}

String getSoilMoistureTopic(int sensorNum) {
    return getTopic("soil/" + String(sensorNum) + "/moisture");
}

String getSoilTempTopic(int sensorNum) {
    return getTopic("soil/" + String(sensorNum) + "/temperature");
}

String getStatusTopic() {
    return getTopic("status");
}