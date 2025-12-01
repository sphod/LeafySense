// wifi_manager.cpp
#include "wifi_manager.h"
#include "config.h"
#include "aht20_sensor.h"
#include "ads1115_sensor.h"
#include "ntp_time.h"
#include <Arduino.h>

// Web server and DNS
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

WiFiConfig wifiConfig;

const char* captivePortalHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Smart Garden Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; margin: 40px; background: #f0f0f0; }
        .container { background: white; padding: 20px; border-radius: 10px; }
        input { width: 100%; padding: 10px; margin: 5px 0; box-sizing: border-box; }
        button { background: #4CAF50; color: white; padding: 10px; border: none; width: 100%; margin: 10px 0; }
        .info { background: #e6f7ff; padding: 10px; border-radius: 5px; margin: 10px 0; }
        .sensor-data { background: #f9f9f9; padding: 15px; border-radius: 5px; margin: 10px 0; border: 1px solid #ddd; }
        table { width: 100%; border-collapse: collapse; }
        td { padding: 8px; border-bottom: 1px solid #eee; }
        .status-online { color: green; font-weight: bold; }
        .status-offline { color: red; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h2>🌱 Smart Garden Setup</h2>
        
        <div class="sensor-data">
            <h3>📊 Live Sensor Data</h3>
            <div id="sensorData">Loading sensor data...</div>
        </div>
        
        <form action="/save" method="POST">
            <h3>WiFi Configuration</h3>
            <input type="text" name="ssid" placeholder="WiFi SSID" value="%SSID%" required>
            <input type="password" name="password" placeholder="WiFi Password" required>
            
            <h3>Device Name</h3>
            <input type="text" name="deviceName" placeholder="Device Name" value="%DEVICENAME%">
            
            <h3>MQTT Configuration (Optional)</h3>
            <input type="text" name="mqttServer" placeholder="MQTT Server (e.g., 192.168.1.100)" value="%MQTT_SERVER%">
            <input type="number" name="mqttPort" placeholder="MQTT Port (default: 1883)" value="%MQTT_PORT%">
            <input type="text" name="mqttUser" placeholder="MQTT Username (optional)" value="%MQTT_USER%">
            <input type="password" name="mqttPassword" placeholder="MQTT Password (optional)" value="%MQTT_PASSWORD%">
            <input type="text" name="mqttTopic" placeholder="MQTT Topic Prefix" value="%MQTT_TOPIC%">
            
            <button type="submit">Save & Connect</button>
        </form>
        
        <div class="info">
            <strong>Device ID:</strong> %DEVICE_ID%<br>
            <strong>MAC Address:</strong> %MAC_ADDRESS%
        </div>
    </div>
    
    <script>
        function updateSensorData() {
            fetch('/sensor-data')
                .then(response => response.text())
                .then(data => {
                    document.getElementById('sensorData').innerHTML = data;
                })
                .catch(error => {
                    document.getElementById('sensorData').innerHTML = 'Error loading sensor data';
                });
        }
        
        // Update sensor data every 5 seconds
        setInterval(updateSensorData, 5000);
        updateSensorData(); // Initial load
    </script>
</body>
</html>
)rawliteral";

String formatHTMLWithValues(const String& html) {
    String result = html;
    result.replace("%SSID%", wifiConfig.ssid);
    result.replace("%DEVICENAME%", wifiConfig.deviceName);
    result.replace("%MQTT_SERVER%", wifiConfig.mqttServer);
    result.replace("%MQTT_PORT%", String(wifiConfig.mqttPort));
    result.replace("%MQTT_USER%", wifiConfig.mqttUser);
    result.replace("%MQTT_PASSWORD%", wifiConfig.mqttPassword);
    result.replace("%MQTT_TOPIC%", MQTT_TOPIC_PREFIX);
    
    String deviceId = String(WiFi.macAddress());
    deviceId.replace(":", "");
    result.replace("%DEVICE_ID%", deviceId);
    result.replace("%MAC_ADDRESS%", WiFi.macAddress());
    
    return result;
}

void handleRoot() {
    String html = formatHTMLWithValues(captivePortalHTML);
    server.send(200, "text/html", html);
}

void handleSensorData() {
    String html = "<table>";
    
    // Add current time
    DateTime currentTime = getCurrentTime();
    html += "<tr><td><strong>Time:</strong></td><td>" + currentTime.timestamp + "</td></tr>";
    
    // Add AHT20 data
    AHT20_Data ahtData = readAHT20();
    if (ahtData.sensor_found && ahtData.last_error.isEmpty()) {
        html += "<tr><td><strong>Air Temperature:</strong></td><td>" + String(ahtData.temperature, 1) + " °C</td></tr>";
        html += "<tr><td><strong>Air Humidity:</strong></td><td>" + String(ahtData.humidity, 1) + " %</td></tr>";
        html += "<tr><td><strong>Air Sensor:</strong></td><td class='status-online'>✅ Working</td></tr>";
    } else {
        html += "<tr><td><strong>Air Sensor:</strong></td><td class='status-offline'>❌ " + ahtData.last_error + "</td></tr>";
    }
    
    // Add ADS1115 data
    ADS1115_Data soilData = readAllSoilSensors();
    if (soilData.ads1115_found) {
        html += "<tr><td><strong>Soil Sensor:</strong></td><td class='status-online'>✅ Working</td></tr>";
        
        if (soilData.sensor1.sensor_working) {
            html += "<tr><td><strong>Soil 1 Moisture:</strong></td><td>" + String(soilData.sensor1.moisture_percentage, 1) + " %</td></tr>";
            html += "<tr><td><strong>Soil 1 Temperature:</strong></td><td>" + String(soilData.sensor1.temperature_celsius, 1) + " °C</td></tr>";
        } else {
            html += "<tr><td><strong>Soil Sensor 1:</strong></td><td class='status-offline'>❌ " + soilData.sensor1.last_error + "</td></tr>";
        }
        
        if (soilData.sensor2.sensor_working) {
            html += "<tr><td><strong>Soil 2 Moisture:</strong></td><td>" + String(soilData.sensor2.moisture_percentage, 1) + " %</td></tr>";
            html += "<tr><td><strong>Soil 2 Temperature:</strong></td><td>" + String(soilData.sensor2.temperature_celsius, 1) + " °C</td></tr>";
        } else {
            html += "<tr><td><strong>Soil Sensor 2:</strong></td><td class='status-offline'>❌ " + soilData.sensor2.last_error + "</td></tr>";
        }
    } else {
        html += "<tr><td><strong>Soil Sensor:</strong></td><td class='status-offline'>❌ " + soilData.last_error + "</td></tr>";
    }
    
    // Add WiFi info
    html += "<tr><td><strong>WiFi RSSI:</strong></td><td>" + String(WiFi.RSSI()) + " dBm</td></tr>";
    if (WiFi.status() == WL_CONNECTED) {
        html += "<tr><td><strong>IP Address:</strong></td><td>" + WiFi.localIP().toString() + "</td></tr>";
    } else {
        html += "<tr><td><strong>Network:</strong></td><td class='status-offline'>Captive Portal Mode</td></tr>";
    }
    
    // Add memory info
    html += "<tr><td><strong>Free Memory:</strong></td><td>" + String(ESP.getFreeHeap()) + " bytes</td></tr>";
    
    html += "</table>";
    
    server.send(200, "text/html", html);
}

void handleSave() {
    // Get form data
    wifiConfig.ssid = server.arg("ssid");
    wifiConfig.password = server.arg("password");
    wifiConfig.deviceName = server.arg("deviceName");
    wifiConfig.mqttServer = server.arg("mqttServer");
    wifiConfig.mqttPort = server.arg("mqttPort").toInt();
    wifiConfig.mqttUser = server.arg("mqttUser");
    wifiConfig.mqttPassword = server.arg("mqttPassword");
    
    // Update global MQTT configuration
    updateMQTTConfigFromWiFiConfig(wifiConfig);
    
    // Save configuration
    if (saveWiFiConfig(wifiConfig)) {
        server.send(200, "text/html", 
            "<!DOCTYPE html>"
            "<html>"
            "<head>"
            "<title>Configuration Saved</title>"
            "<meta name='viewport' content='width=device-width, initial-scale=1'>"
            "<style>"
            "body { font-family: Arial; margin: 40px; background: #f0f0f0; }"
            ".container { background: white; padding: 20px; border-radius: 10px; text-align: center; }"
            ".success { color: #4CAF50; font-size: 24px; }"
            "</style>"
            "</head>"
            "<body>"
            "<div class='container'>"
            "<div class='success'>✅ Configuration Saved!</div>"
            "<p>Device is restarting and will connect to your WiFi network.</p>"
            "<p><strong>SSID:</strong> " + wifiConfig.ssid + "</p>"
            "<p>You can close this page and connect to your normal WiFi network.</p>"
            "</div>"
            "</body>"
            "</html>"
            "<script>setTimeout(function(){ window.location.href = '/'; }, 5000);</script>"
        );
        delay(2000);
        ESP.restart();
    } else {
        server.send(500, "text/html", 
            "<div style='font-family: Arial; margin: 40px;'>"
            "<div style='background: white; padding: 20px; border-radius: 10px;'>"
            "<h2 style='color: red;'>❌ Error Saving Configuration!</h2>"
            "<p>Please try again.</p>"
            "<a href='/'>Go Back</a>"
            "</div>"
            "</div>"
        );
    }
}

void handleNotFound() {
    server.send(200, "text/html", formatHTMLWithValues(captivePortalHTML));
}

void startWebServer() {
    Serial.println("🌐 Starting Web Server...");
    
    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/sensor-data", handleSensorData);
    server.onNotFound(handleNotFound);
    
    server.begin();
    Serial.println("✅ Web Server Started!");
    Serial.println("   Access at: http://" + WiFi.localIP().toString());
}

void updateMQTTConfigFromWiFiConfig(const WiFiConfig& config) {
    MQTT_SERVER = config.mqttServer;
    MQTT_PORT = config.mqttPort > 0 ? config.mqttPort : 1883;
    MQTT_USER = config.mqttUser;
    MQTT_PASSWORD = config.mqttPassword;
    if (!config.deviceName.isEmpty()) {
        MQTT_CLIENT_ID = config.deviceName;
    }
    MQTT_TOPIC_PREFIX = config.deviceName.isEmpty() ? "smartgarden" : config.deviceName;
}

bool loadWiFiConfig() {
    preferences.begin("wifi-config", true);
    
    wifiConfig.ssid = preferences.getString("ssid", "");
    wifiConfig.password = preferences.getString("password", "");
    wifiConfig.deviceName = preferences.getString("deviceName", "SmartGarden");
    wifiConfig.mqttServer = preferences.getString("mqttServer", "");
    wifiConfig.mqttPort = preferences.getInt("mqttPort", 1883);
    wifiConfig.mqttUser = preferences.getString("mqttUser", "");
    wifiConfig.mqttPassword = preferences.getString("mqttPassword", "");
    
    preferences.end();
    
    // Update global MQTT config
    if (!wifiConfig.ssid.isEmpty()) {
        updateMQTTConfigFromWiFiConfig(wifiConfig);
    }
    
    return !wifiConfig.ssid.isEmpty();
}

bool saveWiFiConfig(const WiFiConfig& config) {
    preferences.begin("wifi-config", false);
    
    preferences.putString("ssid", config.ssid);
    preferences.putString("password", config.password);
    preferences.putString("deviceName", config.deviceName);
    preferences.putString("mqttServer", config.mqttServer);
    preferences.putInt("mqttPort", config.mqttPort);
    preferences.putString("mqttUser", config.mqttUser);
    preferences.putString("mqttPassword", config.mqttPassword);
    
    preferences.end();
    
    Serial.println("✅ WiFi configuration saved:");
    Serial.println("   SSID: " + config.ssid);
    Serial.println("   Device Name: " + config.deviceName);
    Serial.println("   MQTT Server: " + config.mqttServer + ":" + String(config.mqttPort));
    
    return true;
}

void setupWiFi() {
    Serial.println("📡 Starting WiFi Manager...");
    
    if (loadWiFiConfig() && !wifiConfig.ssid.isEmpty()) {
        Serial.println("🔌 Attempting to connect to saved WiFi...");
        Serial.println("   SSID: " + wifiConfig.ssid);
        Serial.println("   Device Name: " + wifiConfig.deviceName);
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(wifiConfig.ssid.c_str(), wifiConfig.password.c_str());
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            Serial.print(".");
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n✅ Connected to WiFi!");
            Serial.println("   IP Address: " + WiFi.localIP().toString());
            Serial.println("   RSSI: " + String(WiFi.RSSI()) + " dBm");
            
            // Start mDNS for smartgarden.local
            if (MDNS.begin("smartgarden")) {
                Serial.println("✅ mDNS started: http://smartgarden.local");
            } else {
                Serial.println("❌ mDNS failed");
            }
            
            // Initialize NTP if connected to WiFi
            initNTP();
            
            // START WEB SERVER IN NORMAL MODE
            startWebServer();
            
            return;
        } else {
            Serial.println("\n❌ Failed to connect to saved WiFi");
        }
    }
    
    // If no saved config or connection failed, start captive portal
    startCaptivePortal();
}

void startCaptivePortal() {
    Serial.println("🌐 Starting Captive Portal...");
    
    // Generate unique AP name with MAC address
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    String apName = String(WIFI_AP_SSID_PREFIX) + mac.substring(8);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName.c_str(), WIFI_AP_PASSWORD);
    delay(100);
    
    // Start DNS server for captive portal
    dnsServer.start(53, "*", WiFi.softAPIP());
    
    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/sensor-data", handleSensorData);
    server.onNotFound(handleNotFound);
    
    server.begin();
    
    Serial.println("✅ Captive Portal Started!");
    Serial.println("   AP Name: " + apName);
    Serial.println("   IP: " + WiFi.softAPIP().toString());
    Serial.println("   Visit: http://192.168.4.1");
    Serial.println("   Or: http://setup.smartgarden");
}

void stopCaptivePortal() {
    server.stop();
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
}

bool isCaptivePortalRunning() {
    return WiFi.getMode() == WIFI_AP;
}

void handleWiFiManagerLoop() {
    // Always handle client requests, whether in captive portal or normal mode
    server.handleClient();
    
    // Only process DNS in captive portal mode
    if (isCaptivePortalRunning()) {
        dnsServer.processNextRequest();
    }
}

bool testWiFiConnection(const String& ssid, const String& password) {
    Serial.println("🔍 Testing WiFi connection to: " + ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    bool connected = (WiFi.status() == WL_CONNECTED);
    
    if (connected) {
        Serial.println(" SUCCESS");
        Serial.println("   IP: " + WiFi.localIP().toString());
        // Disconnect after test
        WiFi.disconnect();
    } else {
        Serial.println(" FAILED");
    }
    
    return connected;
}