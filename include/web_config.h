// DogePet Web Configuration Interface
// Provides WiFi AP and REST API for robot face settings
#pragma once

#include <Arduino.h>

// Forward declaration
class roboEyes;

// Initialize WiFi AP and web server
// Returns true if setup successful
bool setupWebConfig(roboEyes* eyesPtr);

// Call this in loop() to handle web requests
void loopWebConfig();

// WiFi AP Configuration
static constexpr const char* AP_SSID = "DogePet_Config";
static constexpr const char* AP_PASS = "";  // Open network for easy testing
static constexpr uint16_t WEB_PORT = 80;
