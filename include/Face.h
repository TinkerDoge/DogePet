#ifndef FACE_H
#define FACE_H

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>
#include "FluxGarage_RoboEyes.h"
#include "config.h"

// Singleton or static class for Face management
class Face {
public:
    static void init();
    static void showBootScreen(const char* msg);
    static void updateProgressBar(int percent, const char* status);
    static void playLineAnimation();
    static void update(); // Call in loop
    
    // Power state faces
    static void showSleepFace();    // Closed eyes with Zzz
    static void showDimFace();      // Tired/droopy eyes
    static void showActiveFace();   // Normal eyes, re-enable animations
    
    // Test helpers (for companion app)
    static void showTestPattern();
    static void clear();
    static void showText(const char* text);
    
    // Accessor for RoboEyes (for serial sync)
    static roboEyes* getEyes();
    static Adafruit_SH1106G* getDisplay();

private:
    static Adafruit_SH1106G display;
    static roboEyes eyes;
    static bool isSleeping;
    static uint32_t sleepAnimMs;
};

#endif
