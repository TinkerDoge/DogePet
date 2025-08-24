// Motion & emotion engine API
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "config.h"

enum MoodState : uint8_t { MS_DEFAULT, MS_HAPPY, MS_ANGRY, MS_FURIOUS, MS_TIRED };

struct ToastState; // forward declaration

struct MotionState {
  uint32_t lastMoveMs = 0;
  uint32_t moodUntil = 0;
  uint32_t shakeStart = 0;
  uint32_t furiousStart = 0;
  bool     shaking = DEFAULT_SHAKING;
  bool     furiousShaking = DEFAULT_FURIOUS_SHAKING;
  uint32_t furiousJiggleEndMs = 0;
  bool     furiousJiggling = DEFAULT_FURIOUS_JIGGLING;
  uint32_t jiggleNextMs = 0;
  uint32_t jiggleEndMs  = 0;
  bool     jiggling = DEFAULT_JIGGLING;
  float    lpf_ax = 0, lpf_ay = 0, lpf_az = 0, lpf_g = 0;
  MoodState curMood = MS_DEFAULT;
};

void updateEmotionsFromIMU(MotionState& state, ToastState& toast);
void updateLivelinessFromIMU(MotionState& state);
void debugPrintIMUIfChanged();
void triggerFuriousJiggle(MotionState& state);
void scheduleNextJiggle(MotionState& state);
void setEyesMood(MotionState& state, MoodState m);


