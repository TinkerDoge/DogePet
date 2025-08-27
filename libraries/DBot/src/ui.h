#pragma once
#include <Arduino.h>
#include "config.h"

class UI {
public:
    void begin();
    void update();
    void showToast(const String& text, uint16_t ms);
private:
    uint32_t _nextUpdate = 0;
};
