// Shared configuration constants for motion/emotion thresholds and timings
#pragma once

#include <stdint.h>

// === Emotion & Motion Detection Thresholds ===
static constexpr float     TILT_HAPPY_DEG     = 10.0f;   // gentle tilt ⇒ happy
static constexpr float     SHAKE_ANGRY_DPS    = 200.0f;  // quick gyro spike ⇒ angry
static constexpr float     SHAKE_FURIOUS_DPS  = 280.0f;  // higher gyro spike ⇒ furious
static constexpr uint16_t  SHAKE_MS           = 120;     // sustained spike window
static constexpr uint16_t  FURIOUS_MS         = 200;     // longer sustained spike for furious
static constexpr float     STILL_G_THRESH     = 0.06f;   // |ax|,|ay| below this and az≈+1g → flat
static constexpr float     AZ_1G_TOL          = 0.12f;   // az within 1g±tol

// === Timing Constants ===
static constexpr uint32_t  MOOD_HOLD_MS       = 2500;    // how long to keep happy/angry
static constexpr uint32_t  FURIOUS_HOLD_MS    = 4000;    // longer duration for furious

// === Tap Detection Constants ===
static constexpr float     TAP_SPIKE_DPS      = 140.0f;  // quick rotation spike => blink
static constexpr uint32_t  TAP_MIN_GAP_MS     = 600;     // min time between blinks
static constexpr uint32_t  TAP_COOLDOWN_MS    = 200;     // cooldown after tap detection

// === Debug System Constants ===
static constexpr float     ACCEL_DELTA        = 0.05f;   // g-units
static constexpr float     GYRO_DELTA         = 5.0f;    // deg/s


