#pragma once
#include <Arduino.h>
#include "config.h"

class UnderlineModules {
public:
    void begin();
    void update();

    int batteryPercent() const { return _batteryPercent; }
    bool isCharging() const { return _batteryCharging; }
    bool wifiConnected() const;

private:
    uint32_t _lastVBatMs = 0;
    int _batteryPercent = 0;
    bool _batteryCharging = false;
};
