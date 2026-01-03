// LED.h - WS2812 NeoPixel status LED
#pragma once

#include <Arduino.h>
#include "config.h"

class LED {
public:
    static void init();
    static void setColor(uint8_t r, uint8_t g, uint8_t b);
    static void setBrightness(uint8_t val);
    static void on();
    static void off();
    
private:
    static uint8_t brightness;
};
