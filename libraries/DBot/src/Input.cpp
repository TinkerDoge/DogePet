#include "Input.h"

using Clock = std::chrono::steady_clock;

void Input::update(bool touch, bool func) {
    if (touch && !_lastTouch && _modeCb) {
        _modeCb();
    }
    _lastTouch = touch;

    auto now = Clock::now();
    if (func && !_lastFunc) {
        _pressTime = now;
    } else if (!func && _lastFunc) {
        auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(now - _pressTime);
        if (dur >= holdTime) {
            if (_longCb) _longCb();
        } else {
            if (_shortCb) _shortCb();
        }
    }
    _lastFunc = func;
}

void Input::onModeCycle(VoidHandler cb) { _modeCb = std::move(cb); }
void Input::onFuncShort(VoidHandler cb) { _shortCb = std::move(cb); }
void Input::onFuncLong(VoidHandler cb)  { _longCb = std::move(cb); }

