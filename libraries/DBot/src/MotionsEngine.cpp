// Motion.cpp
#include "Motion.h"

#include <Wire.h>
#include <math.h>
#include "mpu6050.h"   // mpuInit(), mpuRead(), mpuCalibrate()
#include "util.h"      // accelToAngles(), lpf()
#include "config.h"    // IMU_TICK_MS, SHAKE_* thresholds/timings, etc.

// Global flag definition (declared in Motion.h).
bool gMotionDebug = ENABLE_MOTION_DEBUG;

// Debug macros: compiled in always, but cost almost nothing when gMotionDebug=false
#define MD_BEGIN()        do { if (gMotionDebug) { Serial.println("[Motion] begin()"); } } while (0)
#define MD_PRINT(fmt, ...) do { if (gMotionDebug) { Serial.printf("[Motion] " fmt, ##__VA_ARGS__); } } while (0)
#define MD_EVENT(name)    do { if (gMotionDebug) { Serial.println(String("[Motion] EVENT: ") + (name)); } } while (0)

// ---- ctor --------------------------------------------------------------------
Motion::Motion(int sdaPin, int sclPin, uint8_t mpuAddr)
: _sdaPin(sdaPin), _sclPin(sclPin), _addr(mpuAddr),
  _lpfAx(0), _lpfAy(0), _lpfAz(1.0f), _lpfG(0),
  _tiltDeg(20.0f), _shakeAngryDps(200.0f), _shakeFuriousDps(280.0f),
  _tapThresh(0.5f), _stillGThresh(0.06f), _az1gTol(0.12f),
  _shaking(false), _furiousShaking(false),
  _shakeStartMs(0), _furiousStartMs(0), _lastUpdateMs(0), _lastTapMs(0)
{}

// ---- lifecycle ---------------------------------------------------------------
bool Motion::begin() {
  if (gMotionDebug && !Serial) Serial.begin(115200);
  MD_BEGIN();
  MD_PRINT("I2C SDA=%d SCL=%d ADDR=0x%02X\n", _sdaPin, _sclPin, _addr);

  Wire.begin(_sdaPin, _sclPin);
  delay(10);
  mpuInit();
  delay(50);
  mpuCalibrate(200); // flat & still during boot

  float ax, ay, az, gx, gy, gz;
  mpuRead(ax, ay, az, gx, gy, gz);
  _lpfAx = ax; _lpfAy = ay; _lpfAz = az;
  _lpfG  = sqrtf(gx*gx + gy*gy + gz*gz);
  _lastUpdateMs = millis();

  MD_PRINT("Thresholds: tilt=%.1f° angry=%.1f dps furious=%.1f dps tap=%.2f still=%.3f azTol=%.2f\n",
           _tiltDeg, _shakeAngryDps, _shakeFuriousDps, _tapThresh, _stillGThresh, _az1gTol);
  MD_PRINT("Seed LPF: ax=%.3f ay=%.3f az=%.3f g=%.1f\n", _lpfAx, _lpfAy, _lpfAz, _lpfG);
  return true;
}


// ---- thresholds --------------------------------------------------------------
void Motion::setTiltThresholdDeg(float d){ _tiltDeg = d; MD_PRINT("set tilt=%.1f°\n", d); }
void Motion::setShakeAngryThresholdDps(float d){ _shakeAngryDps = d; MD_PRINT("set angry=%.1f dps\n", d); }
void Motion::setShakeFuriousThresholdDps(float d){ _shakeFuriousDps = d; MD_PRINT("set furious=%.1f dps\n", d); }
void Motion::setTapThreshold(float t){ _tapThresh = t; MD_PRINT("set tap=%.2f\n", t); }
void Motion::setStillThreshold(float accelG, float azTol){
  _stillGThresh = accelG; _az1gTol = azTol;
  MD_PRINT("set still=%.3f azTol=%.2f\n", accelG, azTol);
}


// ---- accessors ---------------------------------------------------------------
float Motion::getAccelX() const { return _lpfAx; }
float Motion::getAccelY() const { return _lpfAy; }
float Motion::getAccelZ() const { return _lpfAz; }
float Motion::getGyroMagnitude() const { return _lpfG; }

// ---- callback registration ---------------------------------------------------
void Motion::onEvent(std::function<void(Event)> handler) { _handler = std::move(handler); }

// ---- util --------------------------------------------------------------------
float Motion::lpf(float prev, float cur, float a) {
  // simple 1-pole LPF
  return prev + a * (cur - prev);
}

void Motion::readRaw(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
  // pull calibrated g and dps from helper
  mpuRead(ax, ay, az, gx, gy, gz);
}

// ---- reset -------------------------------------------------------------------
void Motion::reset() {
  _shaking = false;
  _furiousShaking = false;
  _shakeStartMs = _furiousStartMs = _lastTapMs = 0;
  MD_EVENT("RESET");
}


// ---- main update -------------------------------------------------------------
Motion::Event Motion::update() {
  const uint32_t now = millis();
  if (now - _lastUpdateMs < IMU_TICK_MS) {
    return Event::None;
  }
  _lastUpdateMs = now;

  // 1) read & filter
  float ax, ay, az, gx, gy, gz;
  readRaw(ax, ay, az, gx, gy, gz);

  // noise-gated LPF to reduce twitchiness
  static constexpr float ACCEL_NOISE = 0.02f;
  static constexpr float GYRO_NOISE  = 5.0f;

  if (fabsf(ax - _lpfAx) > ACCEL_NOISE) _lpfAx = lpf(_lpfAx, ax);
  if (fabsf(ay - _lpfAy) > ACCEL_NOISE) _lpfAy = lpf(_lpfAy, ay);
  if (fabsf(az - _lpfAz) > ACCEL_NOISE) _lpfAz = lpf(_lpfAz, az);

  const float gmag = sqrtf(gx*gx + gy*gy + gz*gz);
  if (fabsf(gmag - _lpfG) > GYRO_NOISE) _lpfG = lpf(_lpfG, gmag, 0.3f);
  // Throttled raw snapshot every ~500ms for tuning
  static uint32_t _lastSnapshot = 0;
  if (gMotionDebug && now - _lastSnapshot > 500) {
    _lastSnapshot = now;
    MD_PRINT("LPF: ax=%.3f ay=%.3f az=%.3f g=%.1f\n", _lpfAx, _lpfAy, _lpfAz, _lpfG);
  }
  // 2) derive simple features
  // movement heartbeat useful for external power/idle logic
  const bool moving = (fabsf(ax) > _stillGThresh) ||
                      (fabsf(ay) > _stillGThresh) ||
                      (fabsf(az - 1.0f) > _az1gTol) ||
                      (_lpfG > 40.0f);

  // 3) duration-gated shake classification
    // Furious
  if (_lpfG > _shakeFuriousDps) {
    if (!_furiousShaking) { _furiousShaking = true; _furiousStartMs = now; MD_PRINT("furious-start g=%.1f\n", _lpfG); }
    if (now - _furiousStartMs > FURIOUS_MS) {
      MD_EVENT("FURIOUS_SHAKE");
      if (_handler) _handler(Event::FuriousShake);
      return Event::FuriousShake;
    }
  }
  // Angry
  else if (_lpfG > _shakeAngryDps) {
    if (!_shaking) { _shaking = true; _shakeStartMs = now; MD_PRINT("shake-start g=%.1f\n", _lpfG); }
    if (now - _shakeStartMs > SHAKE_MS) {
      MD_EVENT("SHAKE");
      if (_handler) _handler(Event::Shake);
      return Event::Shake;
    }
  } else {
    if (_shaking || _furiousShaking) MD_PRINT("shake-end g=%.1f\n", _lpfG);
    _shaking = false;
    _furiousShaking = false;
  }
  
  // Tap
  const float tapMag = fabsf(ax) + fabsf(ay) + fabsf(az - 1.0f);
  if (tapMag > _tapThresh && (now - _lastTapMs > TAP_COOLDOWN_MS)) {
    _lastTapMs = now;
    MD_EVENT("TAP");
    if (_handler) _handler(Event::Tap);
    return Event::Tap;
  }
  
  // Tilt
  float pitch, roll;
  accelToAngles(_lpfAx, _lpfAy, _lpfAz, pitch, roll);
  if (fabsf(pitch) > _tiltDeg || fabsf(roll) > _tiltDeg) {
    MD_PRINT("tilt pitch=%.1f roll=%.1f (thr=%.1f)\n", pitch, roll, _tiltDeg);
    MD_EVENT("TILT");
    if (_handler) _handler(Event::Tilt);
    return Event::Tilt;
  }
  
  // Still
  if (!moving) {
    MD_EVENT("STILL");
    if (_handler) _handler(Event::Still);
    return Event::Still;
  }


  return Event::None;
}
