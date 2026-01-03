// DogePet Serial Command Interface
// Handles JSON commands from PC companion app
#pragma once

#include <Arduino.h>
#include "FluxGarage_RoboEyes.h"

// Command string definitions (match WebApp_Protocol.md)
#define CMD_CONNECT       "connect"
#define CMD_SET_EYES      "set_eyes"
#define CMD_GET_EYES      "get_eyes"      // primary per protocol
#define CMD_GET_SETTINGS  "get_settings"  // compatibility alias
#define CMD_ACTION        "action"
#define CMD_SET_PINOUT    "set_pinout"
#define CMD_GET_PINOUT    "get_pinout"
#define CMD_GET_SENSORS   "get_sensors"

// Hardware Configuration
struct HardwareConfig {
    int i2c_sda;
    int i2c_scl;
    int func_btn;
    int led_pin;
    int vibro_left;
    int vibro_right;
    int vbat_pin;
    // Audio / I2S
    int i2s_do;
    int i2s_bclk;
    int i2s_lrc;
    int mic_sd;
    int mic_ws;
    int mic_sck;
};

// Public API
void setupSerialCmd(roboEyes* eyesPtr);
void processSerialCmd();
void loadHardwareConfig();
HardwareConfig* getHardwareConfig();
