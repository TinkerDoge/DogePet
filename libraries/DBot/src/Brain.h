#pragma once
#include <string>
#include <functional>

// Lightweight wrapper around an external AI backend.
class Brain {
public:
    using Handler = std::function<void(const std::string&)>;

    bool begin(const std::string& apiKey);
    void update();

    void sendUserMessage(const std::string& msg);
    const std::string& latestResponse() const;

    // Feed an AI response (JSON string) into the brain
    void receiveAIResponse(const std::string& json);

    void onAnimationCommand(Handler cb);
    void onSoundFxCommand(Handler cb);
    void onThought(Handler cb);

private:
    static std::string extract(const std::string& json, const std::string& key);

    std::string _pending;
    std::string _last;
    Handler _animCb;
    Handler _soundCb;
    Handler _thoughtCb;
};

