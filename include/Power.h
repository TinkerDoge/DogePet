#ifndef POWER_H
#define POWER_H

#include <Arduino.h>
#include "config.h"

enum class PowerState {
    ACTIVE,     // Normal operation
    DIM,        // Screen dimmed, still responsive
    SLEEPING    // Low power mode, waiting for wake event
};

class Power {
public:
    static void init();
    static void update();           // Call in loop - handles sleep/wake logic
    
    // Battery
    static float getVoltage();
    static int getPercent();
    static void logStatus();
    
    // Activity tracking - call these when events occur
    static void onActivity();       // Generic activity (touch, button, etc.)
    static void onMotion();         // IMU motion detected
    static void onLoudNoise();      // Microphone triggered
    
    // Sleep mode
    static PowerState getState();
    static bool isSleeping();
    static void wake();             // Force wake from sleep
    static void sleep();            // Force sleep
    
    // Callbacks - set by main sketch
    static void (*onSleepCallback)();
    static void (*onWakeCallback)();
    static void (*onDimCallback)();

private:
    static float readADC();
    static PowerState state;
    static uint32_t lastActivityMs;
    static uint32_t lastMotionMs;
    static uint32_t lastNoiseMs;
};

#endif
