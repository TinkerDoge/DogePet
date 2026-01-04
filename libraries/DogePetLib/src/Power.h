// Power.h - Battery monitoring and sleep management
#pragma once

#include <Arduino.h>
#include "config.h"

enum class PowerState {
    ACTIVE,
    DIM,
    SLEEPING
};

class Power {
public:
    static void init();
    static void update();  // Call in loop()
    
    // Battery reading
    static float getVoltage();
    static int getPercent();
    static void logStatus();
    
    // Activity tracking (call these to keep awake)
    static void onActivity();      // Wakes from DIM, not from SLEEP
    static void onMotion();         // Wakes from DIM, not from SLEEP
    static void onLoudNoise();      // Wakes from DIM, not from SLEEP
    static void forceWake();        // Wakes from any state (for combo touch and furious shake)
    
    // State control
    static PowerState getState();
    static bool isSleeping();
    static void wake();
    static void sleep();
    
    // Callbacks for state changes
    static void (*onSleepCallback)();
    static void (*onWakeCallback)();
    static void (*onDimCallback)();
    
private:
    static float readADC();
    static void updateBatteryReading();
    static int calculatePercent(float voltage);
    
    static PowerState state;
    static uint32_t lastActivityMs;
    static uint32_t lastMotionMs;
    static uint32_t lastNoiseMs;
    
    // Battery monitoring
    static float cachedVoltage;
    static int cachedPercent;
    static uint32_t lastVbatReadMs;
    static uint32_t lastVbatLogMs;
    static bool batteryInitialized;
};
