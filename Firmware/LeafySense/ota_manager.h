#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

// Function declarations
void initOTA();
void checkForFirmwareUpdate();
void handleOTALoop();

#endif