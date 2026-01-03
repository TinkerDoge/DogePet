// LED.cpp - WS2812 NeoPixel status LED
#include "LED.h"
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel strip(1, LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t LED::brightness = LED_BRIGHTNESS;

void LED::init() {
    strip.begin();
    strip.setBrightness(brightness);
    strip.clear();
    strip.show();
    Serial.println("{\"status\":\"info\",\"msg\":\"LED Init\"}");
}

void LED::setColor(uint8_t r, uint8_t g, uint8_t b) {
    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}

void LED::setBrightness(uint8_t val) {
    brightness = val;
    strip.setBrightness(brightness);
    strip.show();
}

void LED::on() {
    setColor(255, 255, 255);
}

void LED::off() {
    strip.clear();
    strip.show();
}
