#ifndef HAPTICS_H
#define HAPTICS_H

#include <Arduino.h>
#include "config.h"

class Haptics {
public:
    static void init();
    static void stop(); // Force all motors OFF
    static void buzz(uint8_t leftIntensity, uint8_t rightIntensity, uint32_t durationMs);
    static void click(); // Short tick
    static void doubleClick(); // Two ticks
    static void alarm(); // Urgent alarm sequence
    
    // Purr - cat-like rhythmic vibration (non-blocking)
    static void startPurr();      // Begin purring
    static void stopPurr();       // Stop purring
    static void purrTick();       // Call in loop to update purr pattern
    static bool isPurring();
    
private:
    static void setMotor(int pin, int pwm);
    static bool purring;
    static uint32_t purrPhaseMs;
    static uint8_t purrPhase;
};

#endif
