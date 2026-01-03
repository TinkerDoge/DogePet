#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include "config.h"

class LED {
public:
    static void init();
    static void setColor(uint8_t r, uint8_t g, uint8_t b);
    static void setBrightness(uint8_t brightness);
    static void on();   // White at current brightness
    static void off();
    
private:
    static uint8_t brightness;
};

#endif
