#pragma once
#include <Arduino.h>

namespace Sectors { namespace Portal {
  // Initialize resources if needed (SPIFFS etc.). Safe to call multiple times.
  void begin();
  // Run portal server loop when active
  void update();
  // Start the AP + config server
  void start();
  // Stop AP/server and attempt STA connect using saved prefs; returns true on WiFi connected
  bool stopAndConnect(uint32_t timeoutMs);
  // Query
  bool isActive();
}}
