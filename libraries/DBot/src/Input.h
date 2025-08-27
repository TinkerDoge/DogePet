#pragma once
#include <functional>
#include <chrono>

class Input {
public:
    using VoidHandler = std::function<void()>;

    // In this refactored module the hardware layer feeds the button states
    void update(bool touchPressed, bool funcPressed);

    void onModeCycle(VoidHandler cb);
    void onFuncShort(VoidHandler cb);
    void onFuncLong(VoidHandler cb);

private:
    VoidHandler _modeCb;
    VoidHandler _shortCb;
    VoidHandler _longCb;

    bool _lastTouch = false;
    bool _lastFunc = false;
    std::chrono::steady_clock::time_point _pressTime{};
    static constexpr std::chrono::milliseconds holdTime{2000};
};

