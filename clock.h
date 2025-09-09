// Clock Face Module - Header
// Handles the clock display functionality
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <ChronosESP32.h>
#include "config.h"

// Function to draw the clock face (OLED removed – to be reimplemented in LVGL later)
// void drawClock(Adafruit_SH1106G& display, ChronosESP32& chrono);

// Battery reading functions used by clock
float readVBATVolts();
int voltsToPercent(float v);

// External variables needed by clock (will be accessed from main)
extern uint32_t lastVbatReadMs;
extern float vbatVolts;
extern int vbatPercent;
extern bool batteryCharging;
extern uint8_t vbatChargeCount;
extern bool silentMode;
