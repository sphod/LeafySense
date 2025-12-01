#ifndef NTP_TIME_H
#define NTP_TIME_H

#include <WiFi.h>
#include <time.h>

struct DateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  String timestamp;
};

// Function declarations
void initNTP();
bool syncNTPTime();
DateTime getCurrentTime();
String getTimestamp();
void printCurrentTime();

#endif