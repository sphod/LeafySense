#include "reset_manager.h"
#include "config.h"
#include <Arduino.h>
#include <Preferences.h>

ResetState resetState = RESET_NORMAL;
unsigned long buttonPressStart = 0;
unsigned long confirmationStart = 0;
bool lastButtonState = HIGH;

void initResetManager() {
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
    Serial.println("🔘 Reset Manager Initialized (GPIO " + String(RESET_BUTTON_PIN) + ")");
}

void handleResetButton() {
    bool currentButtonState = digitalRead(RESET_BUTTON_PIN);
    
    switch (resetState) {
        case RESET_NORMAL:
            if (currentButtonState == LOW && lastButtonState == HIGH) {
                // Button just pressed
                buttonPressStart = millis();
                resetState = RESET_BUTTON_PRESSED;
                Serial.println("🔘 Reset button pressed, waiting for 5 seconds...");
            }
            break;
            
        case RESET_BUTTON_PRESSED:
            if (currentButtonState == HIGH) {
                // Button released before 5 seconds
                resetState = RESET_NORMAL;
                Serial.println("🔘 Reset button released early");
            } else if (millis() - buttonPressStart >= RESET_HOLD_TIME) {
                // Button held for 5 seconds - enter confirmation mode
                resetState = RESET_CONFIRMATION_WAIT;
                confirmationStart = millis();
                Serial.println("🚨 RESET MODE: Red LED blinking for 10 seconds");
                Serial.println("   Press button again to confirm reset, or wait to cancel");
            }
            break;
            
        case RESET_CONFIRMATION_WAIT:
            // Blink red LED during confirmation period
            if ((millis() / 500) % 2 == 0) {
                setLEDColor(LED_RED, 100);
            } else {
                setLEDColor(LED_OFF);
            }
            
            // Check for confirmation button press
            if (currentButtonState == LOW && lastButtonState == HIGH) {
                resetState = RESET_CONFIRMED;
                Serial.println("✅ Reset confirmed! Restarting system...");
            }
            
            // Check if confirmation period expired
            if (millis() - confirmationStart >= RESET_CONFIRM_TIME) {
                resetState = RESET_NORMAL;
                Serial.println("⏰ Reset period expired, returning to normal operation");
                // Restore normal LED status
                // This will be set by the main loop based on system status
            }
            break;
            
        case RESET_CONFIRMED:
            break;
    }
    
    lastButtonState = currentButtonState;
}

bool shouldResetSystem() {
    return resetState == RESET_CONFIRMED;
}

void resetSystem() {
    Serial.println("🔄 Performing factory reset...");
    
    // Clear all saved preferences
    Preferences preferences;
    preferences.begin("wifi-config", false);
    preferences.clear();
    preferences.end();
    
    // Restart ESP
    Serial.println("💫 Restarting ESP32...");
    delay(1000);
    ESP.restart();
}