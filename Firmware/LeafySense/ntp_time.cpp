#include "ntp_time.h"
#include "config.h"
#include <Arduino.h>

bool timeSynced = false;

void initNTP() {
  Serial.println("⏰ Initializing NTP time synchronization...");
  
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  delay(1000);
  
  if (syncNTPTime()) {
    Serial.println("✅ NTP time synchronized!");
  } else {
    Serial.println("❌ NTP time synchronization failed!");
  }
}

bool syncNTPTime() {
  Serial.print("🕒 Synchronizing time with NTP server...");
  
  int attempts = 0;
  while (!time(nullptr) && attempts < 20) {
    Serial.print(".");
    delay(500);
    attempts++;
  }
  
  if (attempts >= 20) {
    Serial.println(" FAILED");
    timeSynced = false;
    return false;
  }
  
  Serial.println(" SUCCESS");
  timeSynced = true;
  
  DateTime currentTime = getCurrentTime();
  Serial.println("   Current time: " + currentTime.timestamp);
  
  return true;
}

DateTime getCurrentTime() {
  DateTime dt;
  
  if (!timeSynced) {
    dt.timestamp = "Time not synchronized";
    return dt;
  }
  
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  
  dt.year = timeinfo->tm_year + 1900;
  dt.month = timeinfo->tm_mon + 1;
  dt.day = timeinfo->tm_mday;
  dt.hour = timeinfo->tm_hour;
  dt.minute = timeinfo->tm_min;
  dt.second = timeinfo->tm_sec;
  
  // Create formatted timestamp
  char timestamp[20];
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
           dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
  dt.timestamp = String(timestamp);
  
  return dt;
}

String getTimestamp() {
  return getCurrentTime().timestamp;
}

void printCurrentTime() {
  DateTime current = getCurrentTime();
  Serial.println("⏰ Current Time: " + current.timestamp);
}