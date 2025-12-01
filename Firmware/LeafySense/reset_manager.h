#ifndef RESET_MANAGER_H
#define RESET_MANAGER_H

#include <Arduino.h>
#include "led_controller.h"

enum ResetState {
    RESET_NORMAL,
    RESET_BUTTON_PRESSED,
    RESET_CONFIRMATION_WAIT,
    RESET_CONFIRMED
};

void initResetManager();
void handleResetButton();
bool shouldResetSystem();
void resetSystem();

#endif