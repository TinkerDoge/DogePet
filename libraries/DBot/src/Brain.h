#pragma once
#include <Arduino.h>
#include <functional>
#include "config.h"

// Brain module wrapping AICompanion
class Brain {
public:
    using StringHandler = std::function<void(const String&)>;

    bool begin(const char* apiKey);
    void update();

    void sendUserMessage(const String& msg);
    String getLatestResponse() const;

    void onAnimationCommand(StringHandler cb);
    void onSoundFxCommand(StringHandler cb);
    void onThought(StringHandler cb);

private:
    String _lastResponse;
    StringHandler _animCb;
    StringHandler _soundCb;
    StringHandler _thoughtCb;
};
