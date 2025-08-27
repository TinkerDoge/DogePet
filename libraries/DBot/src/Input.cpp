#include "Input.h"

void Input::begin() {
    pinMode(TOUCH_PIN, INPUT_PULLUP);
    pinMode(FUNC_BTN, INPUT_PULLUP);
}

void Input::update() {
    bool touch = digitalRead(TOUCH_PIN) == LOW;
    if (touch && !_lastTouch && _modeCb) {
        _modeCb();
    }
    _lastTouch = touch;

    bool func = digitalRead(FUNC_BTN) == LOW;
    uint32_t now = millis();
    if (func && !_lastFunc) {
        _funcPressMs = now;
    } else if (!func && _lastFunc) {
        uint32_t dur = now - _funcPressMs;
        if (dur >= HOLD_TIME_MS) {
            if (_longCb) _longCb();
        } else {
            if (_shortCb) _shortCb();
        }
    }
    _lastFunc = func;
}

void Input::onModeCycle(VoidHandler cb) { _modeCb = cb; }
void Input::onFuncShort(VoidHandler cb) { _shortCb = cb; }
void Input::onFuncLong(VoidHandler cb) { _longCb = cb; }
