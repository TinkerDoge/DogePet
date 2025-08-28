// Vitals sector: battery, wifi, ble, clock, and notification wrappers
#pragma once
#include <Arduino.h>

// Forward declarations to avoid heavy includes
class Adafruit_SH1106G;
class ChronosESP32;

namespace Sectors { namespace Vitals {
  // Battery functions
  float readVBATVolts();
  int voltsToPercent(float v);
  
  // Clock functions
  void drawClock(Adafruit_SH1106G& display, ChronosESP32& chrono);
  
  // Notification functions  
  void drawNotification(Adafruit_SH1106G& display);
  bool hasNotification();
}}
