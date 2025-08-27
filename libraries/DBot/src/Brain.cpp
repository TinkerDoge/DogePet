#include "Brain.h"
#include "../../../ai_companion.h"

bool Brain::begin(const char* apiKey) {
    AICompanion::initialize();
    return AICompanion::begin(apiKey);
}

void Brain::update() {
    AICompanion::handleBackgroundChatter();

    String latest = String(AICompanion::aiResponse);
    if (latest.length() && latest != _lastResponse) {
        _lastResponse = latest;
        if (_thoughtCb) _thoughtCb(latest);

        String anim = AICompanion::extractJsonValue(latest, "animation");
        if (anim.length() && _animCb) _animCb(anim);

        String sfx = AICompanion::extractJsonValue(latest, "sound");
        if (sfx.length() && _soundCb) _soundCb(sfx);
    }
}

void Brain::sendUserMessage(const String& msg) {
    AICompanion::handleMessage(msg.c_str());
}

String Brain::getLatestResponse() const {
    return String(AICompanion::aiResponse);
}

void Brain::onAnimationCommand(StringHandler cb) { _animCb = cb; }
void Brain::onSoundFxCommand(StringHandler cb) { _soundCb = cb; }
void Brain::onThought(StringHandler cb) { _thoughtCb = cb; }
