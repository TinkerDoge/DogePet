// Touch.cpp - Debounced touch input with tap/hold detection
#include "Touch.h"

// Static member initialization - Head (FUNC_BTN)
bool Touch::headRaw = false, Touch::headState = false, Touch::headLastState = false;
uint32_t Touch::headPressMs = 0, Touch::headDebounceMs = 0;
TouchEvent Touch::headEvent = TouchEvent::NONE;
bool Touch::headHoldStartSent = false;

// Static member initialization - Chin (optional)
bool Touch::chinRaw = false, Touch::chinState = false, Touch::chinLastState = false;
uint32_t Touch::chinPressMs = 0, Touch::chinDebounceMs = 0;
TouchEvent Touch::chinEvent = TouchEvent::NONE;
bool Touch::chinEnabled = false;
bool Touch::chinHoldStartSent = false;

void Touch::init() {
    pinMode(FUNC_BTN, INPUT_PULLDOWN);
    
    #ifdef TOUCH_CHIN_ENABLED
    chinEnabled = true;
    pinMode(TOUCH_CHIN, INPUT_PULLDOWN);
    Serial.println("{\"status\":\"info\",\"msg\":\"Touch Init (Head + Chin)\"}");
    #else
    chinEnabled = false;
    Serial.println("{\"status\":\"info\",\"msg\":\"Touch Init (Head only)\"}");
    #endif
}

void Touch::update() {
    uint32_t now = millis();
    
    headRaw = digitalRead(FUNC_BTN) == HIGH;
    headEvent = TouchEvent::NONE;
    
    // ========== HEAD SENSOR DEBOUNCE & STATE ==========
    if (headRaw != headState) {
        if (now - headDebounceMs >= DEBOUNCE_MS) {
            headLastState = headState;
            headState = headRaw;
            headDebounceMs = now;
            
            if (headState) {
                headPressMs = now;
                headHoldStartSent = false;
            } else {
                uint32_t holdTime = now - headPressMs;
                if (holdTime < TAP_MAX_MS) {
                    headEvent = TouchEvent::TAP;
                } else {
                    headEvent = TouchEvent::HOLD_END;
                }
                headHoldStartSent = false;
            }
        }
    } else {
        headDebounceMs = now;
        
        if (headState) {
            uint32_t holdTime = now - headPressMs;
            if (holdTime >= HOLD_MIN_MS) {
                if (!headHoldStartSent) {
                    headEvent = TouchEvent::HOLD_START;
                    headHoldStartSent = true;
                } else {
                    headEvent = TouchEvent::HOLDING;
                }
            }
        }
    }
    
    // ========== CHIN SENSOR (OPTIONAL) ==========
    #ifdef TOUCH_CHIN_ENABLED
    if (chinEnabled) {
        chinRaw = digitalRead(TOUCH_CHIN) == HIGH;
        
        if (chinRaw != chinState) {
            if (now - chinDebounceMs >= DEBOUNCE_MS) {
                chinLastState = chinState;
                chinState = chinRaw;
                chinDebounceMs = now;
                
                if (chinState) {
                    chinPressMs = now;
                    chinEvent = TouchEvent::HOLD_START;
                } else {
                    uint32_t holdTime = now - chinPressMs;
                    if (holdTime < TAP_MAX_MS) {
                        chinEvent = TouchEvent::TAP;
                    } else {
                        chinEvent = TouchEvent::HOLD_END;
                    }
                }
            }
        } else {
            chinDebounceMs = now;
            
            if (chinState && (now - chinPressMs >= HOLD_MIN_MS)) {
                chinEvent = TouchEvent::HOLDING;
            } else if (!chinState) {
                chinEvent = TouchEvent::NONE;
            }
        }
    }
    #endif
}

TouchEvent Touch::getHeadEvent() { return headEvent; }
TouchEvent Touch::getChinEvent() { 
    #ifdef TOUCH_CHIN_ENABLED
    return chinEvent; 
    #else
    return TouchEvent::NONE;
    #endif
}

bool Touch::isHeadPressed() { return headState; }
bool Touch::isChinPressed() { 
    #ifdef TOUCH_CHIN_ENABLED
    return chinState; 
    #else
    return false;
    #endif
}
bool Touch::isHeadHeld() { return headState && (millis() - headPressMs >= HOLD_MIN_MS); }
bool Touch::isChinHeld() { 
    #ifdef TOUCH_CHIN_ENABLED
    return chinState && (millis() - chinPressMs >= HOLD_MIN_MS); 
    #else
    return false;
    #endif
}

uint32_t Touch::getHeadHoldDuration() {
    return headState ? (millis() - headPressMs) : 0;
}

uint32_t Touch::getChinHoldDuration() {
    #ifdef TOUCH_CHIN_ENABLED
    return chinState ? (millis() - chinPressMs) : 0;
    #else
    return 0;
    #endif
}
