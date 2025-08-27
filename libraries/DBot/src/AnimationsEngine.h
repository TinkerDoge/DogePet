#pragma once
#include <Arduino.h>
#include <stdint.h>
#include "config.h"

// Mood states used by animation commands
enum MoodState : uint8_t { MS_DEFAULT, MS_HAPPY, MS_ANGRY, MS_FURIOUS, MS_TIRED };

// Animation engine functionality
namespace AnimationsEngine {
    void executeEyeAnimation(String animation);
    void executeSoundFX(String soundFx);

    // Helpers exposed for other modules
    void blinkWithSound();
    void lookDirection(String direction);
    void adjustEyeSize(String adjustment);
    void setEyeMood(String mood);
}

// External dependencies (provided by main application)
extern bool silentMode;
extern uint32_t moodUntil;
extern void setEyesMood(MoodState mood);
