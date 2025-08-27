#include "Brain.h"

bool Brain::begin(const std::string&) { return true; }

void Brain::sendUserMessage(const std::string&) {
    // User message handling would be implemented here
}

void Brain::receiveAIResponse(const std::string& json) {
    _pending = json;
}

const std::string& Brain::latestResponse() const { return _last; }

void Brain::update() {
    if (_pending.empty()) return;
    _last = _pending;
    _pending.clear();
    if (_thoughtCb) _thoughtCb(_last);
    const std::string anim = extract(_last, "animation");
    if (!anim.empty() && _animCb) _animCb(anim);
    const std::string sfx = extract(_last, "sound");
    if (!sfx.empty() && _soundCb) _soundCb(sfx);
}

std::string Brain::extract(const std::string& json, const std::string& key) {
    const std::string pat = "\"" + key + "\"";
    size_t pos = json.find(pat);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos);
    if (pos == std::string::npos) return {};
    pos = json.find('"', pos);
    if (pos == std::string::npos) return {};
    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return {};
    return json.substr(pos + 1, end - pos - 1);
}

void Brain::onAnimationCommand(Handler cb) { _animCb = std::move(cb); }
void Brain::onSoundFxCommand(Handler cb)  { _soundCb = std::move(cb); }
void Brain::onThought(Handler cb)         { _thoughtCb = std::move(cb); }

