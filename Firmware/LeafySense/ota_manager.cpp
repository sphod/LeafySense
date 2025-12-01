#include "ota_manager.h"
#include "config.h"
#include "secrets.h"  // Contains your github_pat token
#include "led_controller.h"  // Add this for LED functions
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// Update Check Timer
unsigned long lastUpdateCheck = 0;
bool otaCheckedOnStartup = false;

void initOTA() {
    Serial.println("🔄 Initializing OTA Manager...");
    Serial.println("   Firmware Version: " + String(CURRENT_FIRMWARE_VERSION));
    Serial.println("   GitHub Repo: " + String(GITHUB_OWNER) + "/" + String(GITHUB_REPO));
    Serial.println("   Check Interval: " + String(UPDATE_CHECK_INTERVAL/60000) + " minutes");
    Serial.println("✅ OTA Manager initialized!");
    
    otaCheckedOnStartup = false;
    lastUpdateCheck = millis();
}

void downloadAndApplyFirmware(String url) {
    Serial.println("⬇️ Starting firmware download...");
    Serial.println("   URL: " + url);
    
    // Create WiFiClientSecure for HTTPS
    WiFiClientSecure *client = new WiFiClientSecure;
    client->setInsecure(); // Skip SSL verification for GitHub
    
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setUserAgent("SmartGarden-ESP32C6-OTA");
    http.setTimeout(30000); // 30 second timeout
    
    // Begin the request
    if (!http.begin(*client, url)) {
        Serial.println("❌ Failed to begin HTTP client");
        delete client;
        return;
    }
    
    // Add headers
    http.addHeader("Accept", "application/octet-stream");
    if (strlen(github_pat) > 0) {
        http.addHeader("Authorization", "token " + String(github_pat));
    }
    
    Serial.print("📥 Downloading firmware...");
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.println(" FAILED");
        Serial.println("   HTTP Code: " + String(httpCode));
        Serial.println("   Error: " + http.errorToString(httpCode));
        http.end();
        delete client;
        return;
    }
    
    Serial.println(" SUCCESS");
    
    // Get file size
    int contentLength = http.getSize();
    if (contentLength <= 0) {
        Serial.println("❌ Invalid content length: " + String(contentLength));
        http.end();
        delete client;
        return;
    }
    
    Serial.println("   File size: " + String(contentLength) + " bytes");
    Serial.println("   Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    
    // Check if we have enough space
    if (contentLength > ESP.getFreeSketchSpace()) {
        Serial.println("❌ Not enough space for update!");
        Serial.println("   Required: " + String(contentLength) + " bytes");
        Serial.println("   Available: " + String(ESP.getFreeSketchSpace()) + " bytes");
        http.end();
        delete client;
        return;
    }
    
    // Begin OTA update
    Serial.println("🔧 Starting OTA update...");
    if (!Update.begin(contentLength)) {
        Serial.println("❌ Update begin failed!");
        Serial.println("   Error: " + String(Update.errorString()));
        http.end();
        delete client;
        return;
    }
    
    Serial.println("📝 Writing firmware (this may take a moment)...");
    
    // Get the stream
    WiFiClient *stream = http.getStreamPtr();
    
    // Create buffer for reading
    uint8_t buff[1024] = {0};
    size_t totalWritten = 0;
    int lastProgress = -1;
    
    // Read all data from server
    while (http.connected() && (totalWritten < contentLength)) {
        // Get available data size
        size_t size = stream->available();
        
        if (size) {
            // Read up to 1024 bytes
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
            
            // Write to OTA
            if (Update.write(buff, c) != c) {
                Serial.println("❌ Write failed!");
                Serial.println("   Error: " + String(Update.errorString()));
                Update.abort();
                http.end();
                delete client;
                return;
            }
            
            totalWritten += c;
            
            // Show progress
            int progress = (totalWritten * 100) / contentLength;
            if (progress != lastProgress) {
                if (progress % 10 == 0 || progress == 100) {
                    Serial.println("   Progress: " + String(progress) + "%");
                }
                lastProgress = progress;
            }
        }
        delay(1);
    }
    
    http.end();
    delete client;
    
    // Check if we got all data
    if (totalWritten != contentLength) {
        Serial.println("❌ Download incomplete!");
        Serial.println("   Received: " + String(totalWritten) + "/" + String(contentLength) + " bytes");
        Update.abort();
        return;
    }
    
    // Finalize update
    Serial.print("✅ Download complete! Finalizing...");
    if (Update.end()) {
        Serial.println(" SUCCESS!");
        
        // Verify update
        if (Update.isFinished()) {
            Serial.println("✅ Update verified successfully!");
            Serial.println("🔄 Restarting in 3 seconds...");
            
            // Flash LED to indicate success
            for (int i = 0; i < 10; i++) {
                setLEDColor(LED_GREEN);
                delay(150);
                setLEDColor(LED_OFF);
                delay(150);
            }
            
            delay(1000);
            ESP.restart();
        } else {
            Serial.println("❌ Update verification failed!");
            Update.abort();
        }
    } else {
        Serial.println(" FAILED!");
        Serial.println("   Error: " + String(Update.errorString()));
        Update.abort();
    }
}

void checkForFirmwareUpdate() {
    Serial.println("🔍 Checking for firmware updates...");
    
    // Debug WiFi status
    Serial.println("📶 WiFi Status Check:");
    Serial.println("   Status: " + String(WiFi.status()));
    Serial.println("   Connected: " + String(WiFi.status() == WL_CONNECTED ? "Yes" : "No"));
    Serial.println("   IP Address: " + WiFi.localIP().toString());
    Serial.println("   RSSI: " + String(WiFi.RSSI()) + " dBm");
    Serial.println("   Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("❌ WiFi not connected. Skipping update check.");
        Serial.println("   WiFi Status Code: " + String(WiFi.status()));
        return;
    }
    
    // Test internet connectivity first
    Serial.println("🌐 Testing internet connectivity...");
    HTTPClient testHttp;
    testHttp.setTimeout(5000);  // 5 second timeout
    testHttp.begin("http://www.google.com");
    int testCode = testHttp.GET();
    testHttp.end();
    
    if (testCode <= 0) {
        Serial.println("❌ No internet connectivity. Test failed with code: " + String(testCode));
        return;
    }
    Serial.println("✅ Internet connectivity OK");
    
    String apiUrl = "https://api.github.com/repos/" + String(GITHUB_OWNER) + "/" + 
                    String(GITHUB_REPO) + "/releases/latest";

    Serial.println("   API URL: " + apiUrl);
    Serial.println("   Using GitHub Token: " + String(github_pat[0] ? "Yes" : "No"));

    WiFiClientSecure client;
    client.setInsecure(); // Skip SSL verification for GitHub
    
    HTTPClient http;
    http.setTimeout(10000);  // 10 second timeout for GitHub
    http.setReuse(false);    // Don't reuse connection
    http.begin(client, apiUrl);
    
    // Add headers
    if (strlen(github_pat) > 0) {
        http.addHeader("Authorization", "token " + String(github_pat));
        Serial.println("🔑 Using GitHub PAT for authentication");
    } else {
        Serial.println("⚠️ No GitHub PAT provided - using anonymous access");
    }
    
    http.addHeader("Accept", "application/vnd.github.v3+json");
    http.addHeader("User-Agent", "SmartGarden-ESP32C6");
    
    Serial.print("📡 Sending API request to GitHub...");
    unsigned long startTime = millis();
    int httpCode = http.GET();
    unsigned long elapsed = millis() - startTime;
    
    Serial.println(" Done (" + String(elapsed) + "ms)");
    
    if (httpCode > 0) {
        Serial.println("   HTTP Response Code: " + String(httpCode));
        
        if (httpCode == HTTP_CODE_OK) {
            Serial.println("✅ GitHub API request successful!");
            
            StaticJsonDocument<4096> doc;
            DeserializationError error = deserializeJson(doc, http.getStream());
            http.end();
            
            if (error) {
                Serial.println("❌ Failed to parse JSON: " + String(error.c_str()));
                return;
            }

            String latestVersion = doc["tag_name"].as<String>();
            if (latestVersion.isEmpty() || latestVersion == "null") {
                Serial.println("⚠️ Could not find 'tag_name' in JSON response.");
                return;
            }
            
            Serial.println("📊 Version Comparison:");
            Serial.println("   Current Version: " + String(CURRENT_FIRMWARE_VERSION));
            Serial.println("   Latest Version:  " + latestVersion);

            if (latestVersion != CURRENT_FIRMWARE_VERSION) {
                Serial.println("🚀 NEW FIRMWARE AVAILABLE!");
                Serial.println("   Searching for asset: " + String(FIRMWARE_ASSET_NAME));
                
                String firmwareUrl = "";
                JsonArray assets = doc["assets"].as<JsonArray>();
                int assetCount = 0;

                for (JsonObject asset : assets) {
                    assetCount++;
                    String assetName = asset["name"].as<String>();
                    Serial.println("   Asset " + String(assetCount) + ": " + assetName);

                    if (assetName == String(FIRMWARE_ASSET_NAME)) {
                        String assetId = asset["id"].as<String>();
                        firmwareUrl = "https://api.github.com/repos/" + String(GITHUB_OWNER) + "/" + 
                                      String(GITHUB_REPO) + "/releases/assets/" + assetId;
                        Serial.println("✅ Found matching asset!");
                        break;
                    }
                }
                
                if (assetCount == 0) {
                    Serial.println("⚠️ No assets found in the release.");
                }

                if (firmwareUrl.isEmpty()) {
                    Serial.println("❌ Could not find the specified firmware asset.");
                    Serial.println("   Expected: " + String(FIRMWARE_ASSET_NAME));
                    return;
                }
                
                // Download and apply the update
                Serial.println("🎯 Starting download process...");
                downloadAndApplyFirmware(firmwareUrl);
            } else {
                Serial.println("✅ Device is up to date.");
            }
            
        } else {
            // HTTP error
            String response = http.getString();
            Serial.println("❌ GitHub API error. Response: " + response);
            http.end();
        }
    } else {
        // Connection error
        Serial.println("❌ Connection to GitHub failed!");
        Serial.println("   Error Code: " + String(httpCode));
        Serial.println("   Error: " + String(http.errorToString(httpCode)));
    }
    
    http.end();
    Serial.println("---------------------------------");
}

void handleOTALoop() {
    // Only do periodic checks if you want them
    if (millis() - lastUpdateCheck >= UPDATE_CHECK_INTERVAL) {
        lastUpdateCheck = millis();
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n--- Periodic OTA Update Check ---");
            checkForFirmwareUpdate();
        }
    }
}