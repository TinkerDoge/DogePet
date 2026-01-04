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
    static void buzz(uint8_t left, uint8_t right, uint32_t durationMs);  // Blocking version
    
    // Haptic patterns (non-blocking)
    static void click();           // Single tap feedback (50ms pulse)
    static void doubleClick();     // Double tap with heartbeat rhythm (lub-DUB)
    static void alarm();           // Urgent alternating pattern (3 cycles, 80ms each)
    
    // Non-blocking pattern system (call in main loop)
    static void patternTick();     // Updates all active patterns
    
    // Non-blocking purr
    static void startPurr();
    static void stopPurr();
    static bool isPurring();
    static void purrTick();  // Call in loop()
    
protected:
    // Pattern timing state
    static uint32_t patternStartMs;
    static uint8_t patternPhase;
    static bool patternActive;
    
private:
    static bool purring;
    static uint32_t purrPhaseMs;
    static uint8_t purrPhase;
};
