// Face.h - Display and eye animation management
#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>
#include "FluxGarage_RoboEyes.h"
#include "config.h"

// Display modes for controlling what's shown
enum class DisplayMode {
    Eyes,       // Normal RoboEyes animation
    Sleep,      // Sleep face with Zzz
    Custom,     // User-controlled (eyes paused)
    Toast,      // Temporary message overlay
    Off         // Display cleared/off
};

class Face {
public:
    static void init();
    static void showBootScreen(const char* msg);
    static void updateProgressBar(int percent, const char* status);
    static void playLineAnimation();
    static void update();  // Call in loop
    
    // Display mode control
    static void setMode(DisplayMode mode);
    static DisplayMode getMode();
    static void enableEyes(bool enable);  // Convenience toggle
    static bool areEyesEnabled();
    
    // Power state faces
    static void showSleepFace();
    static void showDimFace();
    static void showActiveFace();
    
    // Toast system (temporary overlay)
    static void showToast(const char* text, uint32_t durationMs = 2000);
    static bool isToastActive();
    
    // Test helpers
    static void showTestPattern();
    static void clear();
    static void showText(const char* text);
    
    // Accessor for RoboEyes and Display
    static roboEyes* getEyes();
    static Adafruit_SH1106G* getDisplay();

private:
    static Adafruit_SH1106G display;
    static roboEyes eyes;
    static DisplayMode currentMode;
    static DisplayMode previousMode;  // For restoring after toast
    static uint32_t sleepAnimMs;
    static char toastText[64];
    static uint32_t toastEndMs;
    
    static void drawSleepFace();
    static void drawToast();
};
