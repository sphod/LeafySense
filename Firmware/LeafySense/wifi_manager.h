#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <ESPmDNS.h> 

struct WiFiConfig {
    String ssid;
    String password;
    String deviceName;
    String mqttServer;
    int mqttPort;
    String mqttUser;
    String mqttPassword;
};

// Function declarations only (no implementations)
void setupWiFi();
void startCaptivePortal();
void stopCaptivePortal();
bool isCaptivePortalRunning();
void handleWiFiManagerLoop();
bool loadWiFiConfig();
bool saveWiFiConfig(const WiFiConfig& config);
void updateMQTTConfigFromWiFiConfig(const WiFiConfig& config);
bool testWiFiConnection(const String& ssid, const String& password);
void startWebServer();

// Web server handler declarations
void handleRoot();
void handleSensorData();
void handleSave();
void handleNotFound();

// Helper function declarations
String formatHTMLWithValues(const String& html);

extern WiFiConfig wifiConfig;

#endif