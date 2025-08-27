#include "UnderlineModules.h"
#include <WiFi.h>

void UnderlineModules::begin() {
    // configure battery pin
    analogReadResolution(12);
}

void UnderlineModules::update() {
    uint32_t now = millis();
    if (now - _lastVBatMs > VBAT_CACHE_MS) {
        float voltage = analogReadMilliVolts(VBAT_PIN) / 1000.0f * VBAT_CAL;
        _batteryPercent = (int)((voltage - VBAT_MIN_V) / (VBAT_MAX_V - VBAT_MIN_V) * 100.0f);
        if (_batteryPercent < 0) _batteryPercent = 0;
        if (_batteryPercent > 100) _batteryPercent = 100;
        _batteryCharging = voltage > VBAT_CHARGE_V;
        _lastVBatMs = now;
    }
}

bool UnderlineModules::wifiConnected() const {
    return WiFi.status() == WL_CONNECTED;
}
