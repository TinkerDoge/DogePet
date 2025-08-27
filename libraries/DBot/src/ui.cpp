#include "ui.h"

using Clock = std::chrono::steady_clock;

void UI::onToast(ToastHandler h) { _toast = std::move(h); }

void UI::update() {
    auto now = Clock::now();
    if (_nextUpdate.time_since_epoch().count() == 0 || now >= _nextUpdate) {
        _nextUpdate = now + updateInterval;
        // Screen refresh work would be performed here
    }
}

void UI::showToast(const std::string& text, uint16_t ms) {
    if (_toast) {
        _toast(text, ms);
    }
}

