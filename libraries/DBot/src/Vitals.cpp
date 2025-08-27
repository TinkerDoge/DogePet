#include "Vitals.h"

void Vitals::setBatteryPercent(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    _batteryPercent = percent;
}

int Vitals::batteryPercent() const {
    return _batteryPercent;
}

void Vitals::setCharging(bool charging) { _charging = charging; }
bool Vitals::isCharging() const { return _charging; }

void Vitals::setWifiConnected(bool connected) { _wifi = connected; }
bool Vitals::wifiConnected() const { return _wifi; }

