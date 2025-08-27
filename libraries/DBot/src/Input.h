#pragma once
#include <Arduino.h>
#include <functional>
#include "config.h"

class Input {
public:
    using VoidHandler = std::function<void()>;

    void begin();
    void update();

    void onModeCycle(VoidHandler cb);
    void onFuncShort(VoidHandler cb);
    void onFuncLong(VoidHandler cb);

private:
    VoidHandler _modeCb;
    VoidHandler _shortCb;
    VoidHandler _longCb;

    bool _lastTouch = false;
    bool _lastFunc = false;
    uint32_t _funcPressMs = 0;
};
