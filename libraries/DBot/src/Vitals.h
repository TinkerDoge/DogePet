#pragma once
#include <string>

// Simple container for runtime vitals like battery and connectivity
class Vitals {
public:
    void setBatteryPercent(int percent);
    int batteryPercent() const;

    void setCharging(bool charging);
    bool isCharging() const;

    void setWifiConnected(bool connected);
    bool wifiConnected() const;

private:
    int  _batteryPercent = 0;
    bool _charging = false;
    bool _wifi = false;
};

