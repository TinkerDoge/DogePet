#pragma once
#include <string>
#include <functional>
#include <chrono>

class UI {
public:
    using ToastHandler = std::function<void(const std::string&, uint16_t)>;

    void onToast(ToastHandler h);
    void update();
    void showToast(const std::string& text, uint16_t ms);

private:
    ToastHandler _toast;
    std::chrono::steady_clock::time_point _nextUpdate{};
    static constexpr std::chrono::milliseconds updateInterval{50};
};

