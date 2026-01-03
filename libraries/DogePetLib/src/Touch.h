// Touch.h - Debounced touch input with tap/hold detection
#pragma once

#include <Arduino.h>
#include "config.h"

enum class TouchEvent {
    NONE = 0,
    TAP,
    HOLD_START,
    HOLDING,
    HOLD_END
};

class Touch {
public:
    static void init();
    static void update();  // Call in loop()
    
    // Event getters (one-shot per loop)
    static TouchEvent getHeadEvent();
    static TouchEvent getChinEvent();
    
    // Raw state getters
    static bool isHeadPressed();
    static bool isChinPressed();
    static bool isHeadHeld();
    static bool isChinHeld();
    
    // Hold duration (ms)
    static uint32_t getHeadHoldDuration();
    static uint32_t getChinHoldDuration();
    
private:
    // Head sensor (FUNC_BTN)
    static bool headRaw, headState, headLastState;
    static uint32_t headPressMs, headDebounceMs;
    static TouchEvent headEvent;
    static bool headHoldStartSent;
    
    // Chin sensor (optional)
    static bool chinRaw, chinState, chinLastState;
    static uint32_t chinPressMs, chinDebounceMs;
    static TouchEvent chinEvent;
    static bool chinEnabled;
    static bool chinHoldStartSent;
};
