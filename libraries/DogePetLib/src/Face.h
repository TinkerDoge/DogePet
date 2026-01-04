// Face.h - Display and eye animation management
#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>
#include "FluxGarage_RoboEyes.h"
#include "config.h"

// Forward declaration for Settings
class Settings;

// Display modes - what should be shown on screen
enum class DisplayMode {
    Eyes,       // RoboEyes animation (default)
    Sleep,      // Sleep face with Zzz
    Toast,      // Toast notification overlay
    Custom,     // Custom content (user draws directly)
    Off         // Display off/blank
};

class Face {
public:
    static void init();
    static void update();  // Call in loop - respects current mode
    
    // Display mode control
    static void setMode(DisplayMode mode);
    static DisplayMode getMode();
    
    // Eyes control - quick toggle
    static void enableEyes(bool enable);
    static bool areEyesEnabled();
    
    // Apply settings from Settings module (dynamic, no reboot)
    static void applySettings();
    
    // Boot sequence helpers
    static void showBootScreen(const char* msg);
    static void updateProgressBar(int percent, const char* status);
    static void playLineAnimation();
    
    // Power state faces
    static void showSleepFace();
    static void showDimFace();
    static void showActiveFace();
    
    // Test helpers
    static void testExpression(const char* name);
    static void showTestPattern();
    static void refresh();  // Call display.display()
    
    // Display utilities
    static void clear();
    static void showText(const char* text);
    
    // Accessor for RoboEyes and Display
    static roboEyes* getEyes();
    static Adafruit_SH1106G* getDisplay();

private:
    static Adafruit_SH1106G display;
    static roboEyes eyes;
    static DisplayMode currentMode;
    static bool eyesEnabled;
    static uint32_t sleepAnimMs;
    
    // Internal draw methods
    static void drawSleepFace();
};
