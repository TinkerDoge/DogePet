// Motion & emotion engine API
#pragma once

#include <Arduino.h>
#include <stdint.h>

enum MoodState : uint8_t { MS_DEFAULT, MS_HAPPY, MS_ANGRY, MS_FURIOUS, MS_TIRED };

void updateEmotionsFromIMU();
void updateLivelinessFromIMU();
void debugPrintIMUIfChanged();
void triggerFuriousJiggle();
void scheduleNextJiggle();
void setEyesMood(MoodState m);


