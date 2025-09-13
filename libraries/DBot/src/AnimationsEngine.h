#pragma once
#include <string>
#include <functional>

class AnimationsEngine {
public:
    enum class Mood { Default, Happy, Angry, Furious, Tired };
    enum class Direction { Center, Left, Right, Up, Down };

    using BlinkHandler = std::function<void()>;
    using LookHandler  = std::function<void(Direction)>;
    using MoodHandler  = std::function<void(Mood)>;

    void onBlink(BlinkHandler h);
    void onLook(LookHandler h);
    void onMood(MoodHandler h);

    void blink();
    void look(Direction d);
    void setMood(Mood m);
    void update();

    // Interpret simple textual commands (e.g., from AI)
    void handleCommand(const std::string& cmd);

private:
    BlinkHandler _blink;
    LookHandler  _look;
    MoodHandler  _mood;
};

