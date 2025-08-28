#include <Arduino.h>
#include <stdint.h>
#include "motion.h"
#include "util.h"
#include "mpu6050.h"
#include "audio.h"
#include "config.h"

// Avoid including RoboEyes here to prevent duplicate globals from its header.
// Instead, call small wrapper functions provided by the main sketch.
extern void Eyes_SetMood(uint8_t mood);
extern void Eyes_SetHFlicker(bool on, int level);
extern void Eyes_SetVFlicker(bool on, int level);
extern void Eyes_Blink();

// Mood codes from RoboEyes
static constexpr uint8_t ROBO_DEFAULT = 0;
static constexpr uint8_t ROBO_TIRED   = 1;
static constexpr uint8_t ROBO_ANGRY   = 2;
static constexpr uint8_t ROBO_HAPPY   = 3;

// SFX debounce for mood changes
static uint32_t lastMoodSfxMs = 0;

// Thresholds and timings centralized in config.h

// Global variables (from main sketch)
MoodState curMood = MS_DEFAULT;
extern uint32_t lastMoveMs;
extern uint32_t moodUntil;
extern uint32_t shakeStart;
extern uint32_t furiousStart;
extern bool     shaking;
extern bool     furiousShaking;
extern uint32_t furiousJiggleEndMs;
extern bool     furiousJiggling;
extern uint32_t jiggleNextMs;
extern uint32_t jiggleEndMs;
extern bool     jiggling;
extern float    lpf_ax, lpf_ay, lpf_az, lpf_g;
static float    last_ax=0, last_ay=0, last_az=0;
static float    last_gx=0, last_gy=0, last_gz=0;
static uint32_t lastBlinkMs=0;
static uint32_t lastTapCheckMs=0;

// Toast & helpers
extern void showToast(const String& s, uint16_t ms);

void setEyesMood(MoodState m) {
  if (m == curMood) {
    return; // avoid retriggering same mood repeatedly
  }
  if (curMood == MS_FURIOUS && m != MS_FURIOUS && furiousJiggling) {
    furiousJiggling = false;
    Eyes_SetHFlicker(false, 0);
    Eyes_SetVFlicker(false, 0);
  }
  curMood = m;
  uint32_t nowMs = millis();
  bool allowSfx = ENABLE_MOOD_SFX && ((nowMs - lastMoodSfxMs) >= MOOD_SFX_GAP_MS);
  switch (m) {
    case MS_DEFAULT: Eyes_SetMood(ROBO_DEFAULT); break;
    case MS_HAPPY:   Eyes_SetMood(ROBO_HAPPY);   if (allowSfx) Audio::playCuteHello(); break;
    case MS_ANGRY:   Eyes_SetMood(ROBO_ANGRY);   if (allowSfx) Audio::playCuteNo(); break;
    case MS_FURIOUS: Eyes_SetMood(ROBO_ANGRY);   triggerFuriousJiggle(); Audio::playCuteFurious(); allowSfx = true; break; // always play furious
    case MS_TIRED:   Eyes_SetMood(ROBO_TIRED);   break;
  }
  if (allowSfx) lastMoodSfxMs = nowMs;
}

void triggerFuriousJiggle() {
  furiousJiggling = true;
  furiousJiggleEndMs = millis() + 500 + (uint32_t)random(0, 300);
  Eyes_SetHFlicker(true, 2);
  if (random(0,2)==0) Eyes_SetVFlicker(true, 2);
}

void updateEmotionsFromIMU() {
  uint32_t currentMs = millis();
  static uint32_t lastMpuReadMs = 0;

  // Only read MPU at IMU tick frequency to reduce I2C traffic
  if (currentMs - lastMpuReadMs < IMU_TICK_MS) return;
  lastMpuReadMs = currentMs;

  float ax, ay, az, gx, gy, gz;
  mpuRead(ax, ay, az, gx, gy, gz);

  float gmag = sqrtf(gx*gx + gy*gy + gz*gz);

  // Only update filters if there's significant change (noise reduction)
  const float ACCEL_NOISE_THRESHOLD = 0.02f;
  const float GYRO_NOISE_THRESHOLD = 5.0f;

  if (fabsf(ax - lpf_ax) > ACCEL_NOISE_THRESHOLD) lpf_ax = lpf(lpf_ax, ax);
  if (fabsf(ay - lpf_ay) > ACCEL_NOISE_THRESHOLD) lpf_ay = lpf(lpf_ay, ay);
  if (fabsf(az - lpf_az) > ACCEL_NOISE_THRESHOLD) lpf_az = lpf(lpf_az, az);
  if (fabsf(gmag - lpf_g) > GYRO_NOISE_THRESHOLD) lpf_g = lpf(lpf_g, gmag, 0.3f);

  bool moving = (fabsf(ax) > STILL_G_THRESH) || (fabsf(ay) > STILL_G_THRESH) || (fabsf(az-1.0f) > AZ_1G_TOL) || (lpf_g > 40.0f);
  if (moving) lastMoveMs = currentMs;

  if (ENABLE_SHAKE_FURIOUS && lpf_g > SHAKE_FURIOUS_DPS) {
    if (!furiousShaking) { furiousShaking = true; furiousStart = currentMs; }
    if (currentMs - furiousStart > FURIOUS_MS) {
      setEyesMood(MS_FURIOUS);
      moodUntil = currentMs + FURIOUS_HOLD_MS;
      // force furious jiggle and keep it on for the entire hold
      furiousJiggling = true;
      furiousJiggleEndMs = moodUntil;
      Eyes_SetHFlicker(true, 2);
      Eyes_SetVFlicker(true, 2);
      // strong alert sound immediately
      if (ENABLE_MOOD_SFX) Audio::sfxFurious();
      showToast("GRRRR! 😠", 1200);
    }
  } else if (ENABLE_SHAKE_ANGRY && lpf_g > SHAKE_ANGRY_DPS) {
    if (!shaking) { shaking = true; shakeStart = currentMs; }
    if (currentMs - shakeStart > SHAKE_MS) {
      if (curMood != MS_FURIOUS) {
        setEyesMood(MS_ANGRY);
        moodUntil = currentMs + MOOD_HOLD_MS;
        showToast("grrr!", 900);
      }
    }
  } else {
    shaking = false;
    furiousShaking = false;
  }

  float tap_mag = fabsf(ax) + fabsf(ay) + fabsf(az - 1.0f);
  if (tap_mag > 0.5f && curMood != MS_ANGRY && curMood != MS_FURIOUS && curMood != MS_HAPPY &&
      (currentMs - lastTapCheckMs > TAP_COOLDOWN_MS)) {
    Eyes_Blink();
    if (ENABLE_TAP_SFX) Audio::sfxBlink();
    lastTapCheckMs = currentMs;
  }

  float pitch, roll;
  accelToAngles(lpf_ax, lpf_ay, lpf_az, pitch, roll);
  if (ENABLE_TILT_HAPPY && curMood != MS_ANGRY && curMood != MS_FURIOUS) {
    if (fabsf(pitch) > TILT_HAPPY_DEG || fabsf(roll) > TILT_HAPPY_DEG) {
      setEyesMood(MS_HAPPY);
      moodUntil = currentMs + MOOD_HOLD_MS;
    }
  }

  if (curMood != MS_TIRED && (curMood == MS_HAPPY || curMood == MS_ANGRY || curMood == MS_FURIOUS)) {
    if (currentMs > moodUntil) setEyesMood(MS_DEFAULT);
  }
}

void debugPrintIMUIfChanged() {
  float ax,ay,az,gx,gy,gz;
  mpuRead(ax,ay,az,gx,gy,gz);
  bool changed = false;
  if (fabs(ax-last_ax) > ACCEL_DELTA || fabs(ay-last_ay) > ACCEL_DELTA || fabs(az-last_az) > ACCEL_DELTA ||
      fabs(gx-last_gx) > GYRO_DELTA  || fabs(gy-last_gy) > GYRO_DELTA  || fabs(gz-last_gz) > GYRO_DELTA) {
    changed = true;
  }
  if (changed) {
    Serial.printf("MPU ax=%.2f ay=%.2f az=%.2f gx=%.1f gy=%.1f gz=%.1f\n", ax, ay, az, gx, gy, gz);
    last_ax=ax; last_ay=ay; last_az=az; last_gx=gx; last_gy=gy; last_gz=gz;
  }
}

void scheduleNextJiggle() {
  uint32_t currentMs = millis();
  jiggleNextMs = currentMs + 800 + (uint32_t)random(0, 2200);
}

void updateLivelinessFromIMU() {
  uint32_t currentMs = millis();
  static uint32_t lastMpuReadMs = 0;

  // Use the same MPU reading optimization as emotion system
  if (currentMs - lastMpuReadMs < IMU_TICK_MS) return;
  lastMpuReadMs = currentMs;

  float ax,ay,az,gx,gy,gz; mpuRead(ax,ay,az,gx,gy,gz);

  // Apply the same noise reduction as emotion system
  const float ACCEL_NOISE_THRESHOLD = 0.02f;
  const float GYRO_NOISE_THRESHOLD = 5.0f;

  if (fabsf(ax - lpf_ax) > ACCEL_NOISE_THRESHOLD) lpf_ax = lpf(lpf_ax, ax);
  if (fabsf(ay - lpf_ay) > ACCEL_NOISE_THRESHOLD) lpf_ay = lpf(lpf_ay, ay);
  if (fabsf(az - lpf_az) > ACCEL_NOISE_THRESHOLD) lpf_az = lpf(lpf_az, az);

  float gmag = sqrtf(gx*gx + gy*gy + gz*gz);
  if (fabsf(gmag - lpf_g) > GYRO_NOISE_THRESHOLD) lpf_g = lpf(lpf_g, gmag, 0.3f);

  if (lpf_g > TAP_SPIKE_DPS && (currentMs - lastBlinkMs > TAP_MIN_GAP_MS) &&
      (currentMs - lastTapCheckMs > TAP_COOLDOWN_MS) &&
      curMood != MS_ANGRY && curMood != MS_FURIOUS && curMood != MS_HAPPY) {
    Eyes_Blink(); lastBlinkMs = currentMs; lastTapCheckMs = currentMs;
  }

  if (ENABLE_RANDOM_JIGGLE && !jiggling && currentMs >= jiggleNextMs && curMood != MS_FURIOUS) {
    jiggling = true; jiggleEndMs = currentMs + 120 + (uint32_t)random(0, 120);
    Eyes_SetHFlicker(true, 1); if (random(0,3)==0) Eyes_SetVFlicker(true, 1);
  }
  if (jiggling && currentMs >= jiggleEndMs) {
    jiggling = false; Eyes_SetHFlicker(false, 0); Eyes_SetVFlicker(false, 0); scheduleNextJiggle();
  }

  if (furiousJiggling && currentMs >= furiousJiggleEndMs) {
    furiousJiggling = false; Eyes_SetHFlicker(false, 0); Eyes_SetVFlicker(false, 0);
  }
}


