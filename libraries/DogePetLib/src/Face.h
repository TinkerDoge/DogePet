// Face.h - Display and eye animation management
#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>
#include "FluxGarage_RoboEyes.h"
#include "config.h"

class Face {
public:
    static void init();
    static void showBootScreen(const char* msg);
    static void updateProgressBar(int percent, const char* status);
    static void playLineAnimation();
    static void update();  // Call in loop
    
    // Power state faces
    static void showSleepFace();
    static void showDimFace();
    static void showActiveFace();
    
    // Test helpers
    static void showTestPattern();
    static void clear();
    static void showText(const char* text);
    
    // Accessor for RoboEyes
    static roboEyes* getEyes();
    static Adafruit_SH1106G* getDisplay();

private:
    static Adafruit_SH1106G display;
    static roboEyes eyes;
    static bool isSleeping;
    static uint32_t sleepAnimMs;
};
