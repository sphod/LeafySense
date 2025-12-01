#include "led_controller.h"
#include "config.h"
#include <Arduino.h>

Adafruit_NeoPixel pixel(NUM_LEDS, WS2812B_PIN, NEO_GRB + NEO_KHZ800);

void initLED() {
  Serial.println("💡 Initializing WS2812B LED...");
  pixel.begin();
  pixel.setBrightness(50);
  setLEDColor(LED_BLUE);  // Initialization color
  Serial.println("✅ WS2812B LED initialized!");
}

void setLEDColor(LEDColor color, uint8_t brightness) {
  pixel.setBrightness(brightness);
  
  switch(color) {
    case LED_OFF:
      pixel.setPixelColor(0, 0, 0, 0);
      break;
    case LED_WHITE:
      pixel.setPixelColor(0, 255, 255, 255);
      break;
    case LED_RED:
      pixel.setPixelColor(0, 255, 0, 0);
      break;
    case LED_GREEN:
      pixel.setPixelColor(0, 0, 255, 0);
      break;
    case LED_BLUE:
      pixel.setPixelColor(0, 0, 0, 255);
      break;
    case LED_YELLOW:
      pixel.setPixelColor(0, 255, 255, 0);
      break;
    case LED_CYAN:
      pixel.setPixelColor(0, 0, 255, 255);
      break;
    case LED_MAGENTA:
      pixel.setPixelColor(0, 255, 0, 255);
      break;
    case LED_ORANGE:
      pixel.setPixelColor(0, 255, 165, 0);
      break;
  }
  pixel.show();
}

void blinkLED(LEDColor color, uint8_t blinks, uint16_t delay_ms) {
  for(int i = 0; i < blinks; i++) {
    setLEDColor(color);
    delay(delay_ms);
    setLEDColor(LED_OFF);
    if(i < blinks - 1) delay(delay_ms);
  }
}

void setLEDStatus(bool wifiConnected, bool mqttConnected, bool sensorsWorking) {
  if (!wifiConnected) {
    setLEDColor(LED_RED);  // Red: No WiFi
  } else if (!mqttConnected) {
    setLEDColor(LED_YELLOW);  // Yellow: WiFi OK, no MQTT
  } else if (!sensorsWorking) {
    setLEDColor(LED_ORANGE);  // Orange: WiFi & MQTT OK, sensor issues
  } else {
    setLEDColor(LED_GREEN);   // Green: All systems go!
  }
}