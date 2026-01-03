// Haptics.h - Vibration motor control
#pragma once

#include <Arduino.h>
#include "config.h"

class Haptics {
public:
    static void init();
    static void stop();
    
    // Direct motor control
    static void setMotor(int pin, int pwm);
    static void buzz(uint8_t left, uint8_t right, uint32_t durationMs);
    
    // Haptic patterns
    static void click();
    static void doubleClick();
    static void alarm();
    
    // Non-blocking purr
    static void startPurr();
    static void stopPurr();
    static bool isPurring();
    static void purrTick();  // Call in loop()
    
private:
    static bool purring;
    static uint32_t purrPhaseMs;
    static uint8_t purrPhase;
};
