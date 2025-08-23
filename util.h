// Utility helpers for smoothing and angle conversion
#pragma once

#include <Arduino.h>
#include <math.h>

// Low pass filter function for smoothing sensor readings
inline float lpf(float prev, float x, float a = 0.2f) {
  return prev * (1.0f - a) + x * a;
}

// Convert accelerometer readings to pitch and roll angles (degrees)
inline void accelToAngles(float ax, float ay, float az, float& pitchDeg, float& rollDeg) {
  pitchDeg = atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.2958f;   // pitch around X
  rollDeg  = atan2f( ay, sqrtf(ax * ax + az * az)) * 57.2958f;   // roll around Y
}


