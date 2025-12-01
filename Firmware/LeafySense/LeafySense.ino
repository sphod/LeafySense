// SmartGarden_ESP32C6.ino
#include "config.h"
#include "wifi_manager.h"
#include "aht20_sensor.h"
#include "ads1115_sensor.h"
#include "led_controller.h"
#include "ntp_time.h"
#include "mqtt_manager.h"
#include "reset_manager.h"
#include "ota_manager.h"

// Timers
unsigned long lastSensorRead = 0;
unsigned long lastMQTTPublish = 0;
bool allSensorsWorking = false;
void setup() {
  pinMode(4, OUTPUT); //fast charging, high 500mA/low 100mA
  digitalWrite(4, HIGH); //enable fast charging

  // Disable brownout detector
  #ifdef DISABLE_BROWNOUT_DETECTION
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  #endif

  // Start Serial communication
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect
  }
  Serial.println("🚀 Smart Garden ESP32-C6 Booting...");
  Serial.println("   Firmware: Basic MQTT Version");
  Serial.println("   MAC: " + WiFi.macAddress());
  
  // Initialize components
  initLED();
  setLEDColor(LED_BLUE);
  initResetManager();
  
  // Initialize I2C devices
  Serial.println("📡 Initializing I2C Sensors...");
  bool aht20Working = initAHT20();
  bool ads1115Working = initADS1115();
  allSensorsWorking = aht20Working || ads1115Working;

  // Initialize WiFi (this may start captive portal)
  setupWiFi();
    
    // Initialize OTA and check
    initOTA();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n--- Initial OTA Check on Startup ---");
        checkForFirmwareUpdate();
    }
  
  // Force immediate OTA check on startup if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n--- Initial OTA Check on Startup ---");
    checkForFirmwareUpdate();  // This will check immediately
  } else {
    Serial.println("⚠️ Skipping OTA check - WiFi not connected");
  }

  // Initialize MQTT if server is configured
  if (MQTT_SERVER.length() > 0) {
    initMQTT();
    connectMQTT();
  } else {
    Serial.println("⚠️ MQTT not configured - skipping MQTT initialization");
  }

  // Set initial LED status
  if (isCaptivePortalRunning()) {
    setLEDColor(LED_CYAN);
    Serial.println("💡 LED Status: Cyan (Captive Portal Mode)");
  } else {
    bool mqttConnected = (MQTT_SERVER.length() > 0) ? isMQTTConnected() : true;
    setLEDStatus(WiFi.status() == WL_CONNECTED, mqttConnected, allSensorsWorking);
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("💡 LED Status: Green (WiFi Connected)");
    } else {
      Serial.println("💡 LED Status: Red (WiFi Disconnected)");
    }
  }

  Serial.println("✅ System initialization complete!");
  Serial.println("📊 System Status:");
  Serial.println("   - WiFi: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
  Serial.println("   - Captive Portal: " + String(isCaptivePortalRunning() ? "Active" : "Inactive"));
  Serial.println("   - MQTT: " + String(MQTT_SERVER.length() > 0 ? (isMQTTConnected() ? "Connected" : "Disconnected") : "Not Configured"));
  Serial.println("   - AHT20 Sensor: " + String(aht20Working ? "Working" : "Not Found"));
  Serial.println("   - ADS1115 Sensor: " + String(ads1115Working ? "Working" : "Not Found"));
  Serial.println("   - Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  
  // Print MQTT configuration if set
  if (MQTT_SERVER.length() > 0) {
    Serial.println("📡 MQTT Configuration:");
    Serial.println("   - Server: " + MQTT_SERVER + ":" + String(MQTT_PORT));
    Serial.println("   - Client ID: " + MQTT_CLIENT_ID);
    Serial.println("   - Topic Prefix: " + MQTT_TOPIC_PREFIX);
  }
  
  blinkLED(LED_GREEN, 3, 100);
}

void loop() {
  // Handle reset button
  handleResetButton();
  
  // Check if system should reset
  if (shouldResetSystem()) {
    resetSystem();
    return;
  }
  
  // Handle WiFi manager (captive portal)
  handleWiFiManagerLoop();
  
  // If in captive portal mode, skip normal operations
  if (isCaptivePortalRunning()) {
    // Update LED to indicate captive portal mode
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
      lastBlink = millis();
      static bool ledState = false;
      if (ledState) {
        setLEDColor(LED_CYAN);
      } else {
        setLEDColor(LED_OFF);
      }
      ledState = !ledState;
    }
    delay(100);
    return;
  }

  // Handle MQTT if server is configured
  if (MQTT_SERVER.length() > 0) {
    mqttLoop();
    checkMQTTConnection();
  }

  // Read sensors at regular intervals
  if (millis() - lastSensorRead >= SENSOR_READ_INTERVAL) {
    lastSensorRead = millis();
    
    Serial.println("\n--- Reading Sensors ---");
    
    // Read AHT20 data (air temperature & humidity)
    AHT20_Data ahtData = readAHT20();
    printAHT20Data(ahtData);
    
    // Read ADS1115 data (soil moisture & temperature)
    ADS1115_Data soilData = readAllSoilSensors();
    printADS1115Data(soilData);

    // Update LED status
    bool mqttConnected = (MQTT_SERVER.length() > 0) ? isMQTTConnected() : true;
    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    setLEDStatus(wifiConnected, mqttConnected, allSensorsWorking);
    
    Serial.println("--- End of Readings ---\n");
  }

  // Publish to MQTT at regular intervals (only if MQTT is enabled)
  if (MQTT_SERVER.length() > 0 && millis() - lastMQTTPublish >= MQTT_PUBLISH_INTERVAL) {
    if (isMQTTConnected()) {
      lastMQTTPublish = millis();
      
      Serial.println("📤 Publishing sensor data to MQTT...");
      AHT20_Data ahtData = readAHT20();
      ADS1115_Data soilData = readAllSoilSensors();
      publishSensorData(ahtData, soilData);
      
      // Print current time and system info
      printCurrentTime();
      Serial.println("💾 Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
      Serial.println("📶 WiFi RSSI: " + String(WiFi.RSSI()) + " dBm");
    } else {
      Serial.println("⚠️ MQTT not connected, skipping publish");
      // Try to reconnect
      connectMQTT();
    }
  }

  // Handle WiFi connection loss
  if (WiFi.status() != WL_CONNECTED && !isCaptivePortalRunning()) {
    Serial.println("⚠️ WiFi connection lost, attempting to reconnect...");
    WiFi.reconnect();
    delay(1000);
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("✅ WiFi reconnected!");
      // Reinitialize NTP and MQTT
      initNTP();
      if (MQTT_SERVER.length() > 0) {
        connectMQTT();
      }
    }
  }

  // Small delay to prevent overwhelming the system
  delay(100);
}