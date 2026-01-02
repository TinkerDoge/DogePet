// DogePet Serial Command Interface
// Handles JSON commands from PC companion app
#pragma once

#include <Arduino.h>

// Forward declaration
class roboEyes;

// Initialize Serial command handler
void setupSerialCmd(roboEyes* eyesPtr);

// Process incoming Serial commands - call in loop()
void processSerialCmd();

// Command types
#define CMD_SET_EYES    "set_eyes"
#define CMD_GET_SETTINGS "get_settings"
#define CMD_ACTION      "action"
