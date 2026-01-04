#include "LED.h"
#include <Adafruit_NeoPixel.h>
#include "ConfigManager.h"

static Adafruit_NeoPixel strip(1, LED_PIN, NEO_GRB + NEO_KHZ800);

uint8_t LED::brightness = LED_BRIGHTNESS;

void LED::init() {
    strip.begin();
    strip.setBrightness(settings.led.brightness);
    strip.setPixelColor(0, strip.Color(settings.led.r, settings.led.g, settings.led.b));
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
