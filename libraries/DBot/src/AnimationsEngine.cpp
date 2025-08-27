#include "AnimationsEngine.h"
#include <algorithm>

void AnimationsEngine::onBlink(BlinkHandler h) { _blink = std::move(h); }
void AnimationsEngine::onLook(LookHandler h)   { _look  = std::move(h); }
void AnimationsEngine::onMood(MoodHandler h)   { _mood  = std::move(h); }

void AnimationsEngine::blink()            { if (_blink) _blink(); }
void AnimationsEngine::look(Direction d)  { if (_look)  _look(d); }
void AnimationsEngine::setMood(Mood m)    { if (_mood)  _mood(m); }
void AnimationsEngine::update()           { /* placeholder for idle animations */ }

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}

void AnimationsEngine::handleCommand(const std::string& cmd) {
    std::string c = toLower(cmd);
    if (c == "blink") blink();
    else if (c == "look_left")  look(Direction::Left);
    else if (c == "look_right") look(Direction::Right);
    else if (c == "look_up")    look(Direction::Up);
    else if (c == "look_down")  look(Direction::Down);
    else if (c == "center")     look(Direction::Center);
    else if (c == "happy")      setMood(Mood::Happy);
    else if (c == "angry")      setMood(Mood::Angry);
    else if (c == "furious")    setMood(Mood::Furious);
    else if (c == "tired" || c == "sleepy") setMood(Mood::Tired);
}

