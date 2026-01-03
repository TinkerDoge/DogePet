#ifndef TOUCH_H
#define TOUCH_H

#include <Arduino.h>
#include "config.h"

// Touch event types
enum class TouchEvent {
    NONE,
    TAP,            // Quick single tap
    HOLD_START,     // Just started holding
    HOLDING,        // Continuously holding
    HOLD_END        // Just released from hold
};

class Touch {
public:
    static void init();
    static void update();           // Call in loop - handles debounce & state
    
    // Event queries (check after update())
    static TouchEvent getHeadEvent();   // FUNC_BTN on head
    static TouchEvent getChinEvent();   // Optional chin sensor
    
    // Raw state
    static bool isHeadPressed();
    static bool isChinPressed();
    static bool isHeadHeld();
    static bool isChinHeld();
    
    // Timing
    static uint32_t getHeadHoldDuration();
    static uint32_t getChinHoldDuration();

private:
    // Debounce & state tracking - Head (FUNC_BTN)
    static bool headRaw, headState, headLastState;
    static uint32_t headPressMs, headDebounceMs;
    static TouchEvent headEvent;
    static bool headHoldStartSent;  // Track if HOLD_START was already sent
    
    // Debounce & state tracking - Chin (optional)
    static bool chinRaw, chinState, chinLastState;
    static uint32_t chinPressMs, chinDebounceMs;
    static TouchEvent chinEvent;
    static bool chinEnabled;
    static bool chinHoldStartSent;
    
    // Configuration
    static constexpr uint32_t DEBOUNCE_MS = 30;
    static constexpr uint32_t TAP_MAX_MS = 300;    // Max duration for tap
    static constexpr uint32_t HOLD_MIN_MS = 400;   // Min duration to be "hold"
};

#endif
