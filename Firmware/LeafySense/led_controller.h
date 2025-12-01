#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <Adafruit_NeoPixel.h>

// LED Color Definitions
enum LEDColor {
  LED_OFF = 0,
  LED_WHITE,
  LED_RED,
  LED_GREEN,
  LED_BLUE,
  LED_YELLOW,
  LED_CYAN,
  LED_MAGENTA,
  LED_ORANGE
};

// Function declarations
void initLED();
void setLEDColor(LEDColor color, uint8_t brightness = 50);
void blinkLED(LEDColor color, uint8_t blinks = 3, uint16_t delay_ms = 200);
void setLEDStatus(bool wifiConnected, bool mqttConnected, bool sensorsWorking);

#endif