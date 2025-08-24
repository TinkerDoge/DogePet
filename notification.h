// Notification Face Module - Header
// Handles the notification display functionality
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <Adafruit_SH110X.h>
#include "config.h"

// Function to draw the notification face
void drawNotif(Adafruit_SH1106G& display);

// Toast overlay function
void drawToastIfAny(Adafruit_SH1106G& display);

// External variables needed by notification (will be accessed from main)
extern char lastNotifTitle[64];
extern char lastNotifBody[128];
extern char toastText[200];
extern char toastFullText[250];
extern uint32_t toastUntil;
extern bool toastVisible;
extern uint32_t lastToastDrawMs;
extern bool toastTypewriter;
extern uint8_t toastTypePos;
extern uint32_t toastTypeSpeed;
extern uint32_t lastToastTypeMs;
// Binary scatter overlay flags
extern bool toastScatter;     // when true, draw each char at random positions
extern bool toastNoFrame;     // when true, skip banner frame/box

// Utility function to check if there are notifications
bool hasNotif();
