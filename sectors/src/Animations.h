// Animations sector: wrapper over animation_engine and mood
#pragma once
#include <Arduino.h>

// Forward declare the global MoodState enum to match signature without including motion.h
enum MoodState : uint8_t;

namespace Sectors { namespace Animations {
  void setMood(MoodState mood);
  MoodState getCurrentMood(); // Access current mood
  void blink();
  void look(const String& direction);
  void eyeSize(const String& adj);
  void playFx(const String& sfx);
}}
