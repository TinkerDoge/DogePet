#include "ui.h"

// Global toast function from existing codebase
extern void showToast(const String& s, uint16_t ms);

void UI::begin() {
    _nextUpdate = 0;
}

void UI::update() {
    if (millis() >= _nextUpdate) {
        _nextUpdate = millis() + DISPLAY_UPDATE_MS;
        // Placeholder: draw operations would happen here
    }
}

void UI::showToast(const String& text, uint16_t ms) {
    ::showToast(text, ms);
}
