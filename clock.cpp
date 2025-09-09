// Clock Face Module - Implementation
#include "clock.h"
#include "config.h"
#include <stdint.h>
// Old OLED drawClock implementation removed.
// If needed, a new LVGL-based clock will be implemented later.

// Frame constants (from main)
static const int FRAME_X = 0;
static const int FRAME_Y = 0;  
static const int FRAME_W = SCREEN_W - 0;
static const int FRAME_H = 64;
static const uint8_t FRAME_RADIUS = 8;
static const uint8_t FRAME_THICKNESS = 2;

// UI helper functions
// drawSharedFrame removed

// measureAdvance_Pixeboy removed

// headerBaselineY removed

// headerBottomY removed

// drawHeaderDateBattery removed

// Battery reading function
float readVBATVolts() {
  // Average a few samples
  uint32_t mvAcc = 0;
  for (uint8_t i=0;i<VBAT_SAMPLES;i++) mvAcc += analogReadMilliVolts(VBAT_PIN);
  float mvAvg = (float)mvAcc / (float)VBAT_SAMPLES;
  float v_pin = mvAvg / 1000.0f; // calibrated millivolts -> volts
  // Divider is 1:2 → battery voltage is 2x pin voltage
  return v_pin * 2.0f * VBAT_CAL;
}

int voltsToPercent(float v) {
  if (v <= VBAT_MIN_V) return 0;
  if (v >= VBAT_MAX_V) return 100;
  float pct = (v - VBAT_MIN_V) / (VBAT_MAX_V - VBAT_MIN_V) * 100.0f;
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  return (int)(pct + 0.5f);
}

// Main clock drawing function removed; to be reimplemented using LVGL later
