// Motion.cpp - High-level motion interpreter implementation
#include "Motion.h"
#include "mpu6050.h"
#include "config.h"
#include "ConfigManager.h"
#include <Wire.h>
#include <math.h>

// Global calibration offsets (used by mpu6050.h inline functions)
int32_t mpu_accel_off_x = 0, mpu_accel_off_y = 0, mpu_accel_off_z = 0;
int32_t mpu_gyro_off_x = 0, mpu_gyro_off_y = 0, mpu_gyro_off_z = 0;

// Global debug flag
bool gMotionDebug = ENABLE_MOTION_DEBUG;

// Debug macros
#define MD_PRINT(fmt, ...) do { if (gMotionDebug) { Serial.printf("[Motion] " fmt, ##__VA_ARGS__); } } while (0)
#define MD_EVENT(name) do { if (gMotionDebug) { Serial.println(String("[Motion] EVENT: ") + (name)); } } while (0)

// Static instance
Motion Motion::_instance;

// ---- Static API ----
void Motion::init() {
    _instance.begin();
}

Motion::Event Motion::update() {
    return _instance.updateInstance();
}

bool Motion::isReady() {
    return _instance._initialized;
}

void Motion::calibrate() {
    _instance.calibrateInstance();
}

float Motion::getGyroMag() {
    return _instance._lpfG;
}

// ---- Constructors ----
Motion::Motion()
    : Motion(I2C_SDA, I2C_SCL, MPU6050_ADDR) {}

Motion::Motion(int sdaPin, int sclPin, uint8_t mpuAddr)
    : _sdaPin(sdaPin), _sclPin(sclPin), _addr(mpuAddr),
      _initialized(false),
      _lpfAx(0), _lpfAy(0), _lpfAz(1.0f), _lpfG(0),
      _tiltDeg(DEFAULT_TILT_THRESHOLD_DEG), 
      _shakeAngryDps(DEFAULT_SHAKE_ANGRY_DPS),
      _shakeFuriousDps(DEFAULT_SHAKE_FURIOUS_DPS),
      _tapThresh(DEFAULT_TAP_SPIKE_DPS), 
      _stillGThresh(STILL_G_THRESH), 
      _az1gTol(AZ_1G_TOL),
      _moving(false), _shaking(false), _furiousShaking(false),
      _shakeStartMs(0), _furiousStartMs(0), _lastUpdateMs(0), _lastTapMs(0),
      _wasStill(true) {}

// ---- Lifecycle ----
bool Motion::begin() {
    MD_PRINT("begin() SDA=%d SCL=%d ADDR=0x%02X\n", _sdaPin, _sclPin, _addr);

    // Calibrate and set thresholds
    calibrate();
    
    // Load runtime settings
    _tiltDeg = settings.motion.tilt;
    _shakeAngryDps = settings.motion.shake;
    _shakeFuriousDps = settings.motion.furious;
    _tapThresh = settings.motion.tap;
    
    _initialized = true;
    
    // Initialize I2C (if not already done)
    Wire.begin(_sdaPin, _sclPin);
    Wire.setClock(400000);
    delay(10);
    
    // Initialize MPU6050
    if (!mpuInit()) {
        Serial.println("{\"status\":\"error\",\"msg\":\"MPU6050 init failed\"}");
        _initialized = false;
        return false;
    }
    delay(50);
    
    // Calibrate (device should be flat and still)
    Serial.println("{\"status\":\"info\",\"msg\":\"Calibrating IMU... keep still\"}");
    mpuCalibrate(200);
    
    // Print calibration offsets
    Serial.printf("{\"status\":\"info\",\"msg\":\"IMU Offsets: AX=%ld AY=%ld AZ=%ld GX=%ld GY=%ld GZ=%ld\"}\n",
                  mpu_accel_off_x, mpu_accel_off_y, mpu_accel_off_z,
                  mpu_gyro_off_x, mpu_gyro_off_y, mpu_gyro_off_z);
    
    // Seed LPF with initial reading
    float ax, ay, az, gx, gy, gz;
    mpuRead(ax, ay, az, gx, gy, gz);
    _lpfAx = ax; _lpfAy = ay; _lpfAz = az;
    _lpfG = sqrtf(gx*gx + gy*gy + gz*gz);
    _lastUpdateMs = millis();
    
    MD_PRINT("Thresholds: tilt=%.1f° shake=%.1f dps furious=%.1f dps tap=%.1f dps\n",
             _tiltDeg, _shakeAngryDps, _shakeFuriousDps, _tapThresh);
    MD_PRINT("Seed LPF: ax=%.3f ay=%.3f az=%.3f g=%.1f\n", _lpfAx, _lpfAy, _lpfAz, _lpfG);
    
    _initialized = true;
    Serial.println("{\"status\":\"ok\",\"msg\":\"Motion sensor ready\"}");
    return true;
}

// ---- Threshold setters ----
void Motion::setTiltThresholdDeg(float d) { _tiltDeg = d; MD_PRINT("set tilt=%.1f°\n", d); }
void Motion::setShakeAngryThresholdDps(float d) { _shakeAngryDps = d; MD_PRINT("set shake=%.1f dps\n", d); }
void Motion::setShakeFuriousThresholdDps(float d) { _shakeFuriousDps = d; MD_PRINT("set furious=%.1f dps\n", d); }
void Motion::setTapThreshold(float t) { _tapThresh = t; MD_PRINT("set tap=%.1f dps\n", t); }
void Motion::setStillThreshold(float accelG, float azTol) {
    _stillGThresh = accelG; _az1gTol = azTol;
    MD_PRINT("set still=%.3f azTol=%.2f\n", accelG, azTol);
}

// ---- Callbacks ----
void Motion::onEvent(std::function<void(Event)> handler) { _handler = std::move(handler); }

// ---- Helpers ----
float Motion::lpf(float prev, float cur, float a) {
    return prev + a * (cur - prev);
}

void Motion::readRaw(float &ax, float &ay, float &az, float &gx, float &gy, float &gz) {
    mpuRead(ax, ay, az, gx, gy, gz);
}

void Motion::reset() {
    _moving = false;
    _shaking = false;
    _furiousShaking = false;
    _shakeStartMs = _furiousStartMs = _lastTapMs = 0;
    _wasStill = true;
    MD_EVENT("RESET");
}

void Motion::calibrateInstance() {
    Serial.println("{\"status\":\"info\",\"msg\":\"Re-calibrating IMU...\"}");
    mpuCalibrate(200);
    Serial.println("{\"status\":\"ok\",\"msg\":\"Calibration complete\"}");
}

// ---- Main update (instance method) ----
Motion::Event Motion::updateInstance() {
    const uint32_t now = millis();
    
    // Rate limiting
    if (now - _lastUpdateMs < IMU_TICK_MS) {
        return Event::None;
    }
    _lastUpdateMs = now;
    
    // 1) Read & filter
    float ax, ay, az, gx, gy, gz;
    readRaw(ax, ay, az, gx, gy, gz);
    
    // Noise-gated LPF to reduce twitchiness
    if (fabsf(ax - _lpfAx) > ACCEL_NOISE) _lpfAx = lpf(_lpfAx, ax);
    if (fabsf(ay - _lpfAy) > ACCEL_NOISE) _lpfAy = lpf(_lpfAy, ay);
    if (fabsf(az - _lpfAz) > ACCEL_NOISE) _lpfAz = lpf(_lpfAz, az);
    
    const float gmag = sqrtf(gx*gx + gy*gy + gz*gz);
    if (fabsf(gmag - _lpfG) > GYRO_NOISE) _lpfG = lpf(_lpfG, gmag, 0.3f);
    
    // Debug output (throttled)
    static uint32_t lastSnapshot = 0;
    if (gMotionDebug && now - lastSnapshot > 500) {
        lastSnapshot = now;
        MD_PRINT("LPF: ax=%.3f ay=%.3f az=%.3f g=%.1f\n", _lpfAx, _lpfAy, _lpfAz, _lpfG);
    }
    
    // 2) Derive features
    _moving = (fabsf(_lpfAx) > _stillGThresh) ||
              (fabsf(_lpfAy) > _stillGThresh) ||
              (fabsf(_lpfAz - 1.0f) > _az1gTol) ||
              (_lpfG > 40.0f);
    
    // Check for still (was moving, now not)
    bool isStillNow = !_moving;
    if (!_wasStill && isStillNow) {
        _wasStill = true;
        MD_EVENT("STILL");
        if (_handler) _handler(Event::Still);
        return Event::Still;
    }
    _wasStill = isStillNow;
    
    // 3) Duration-gated shake classification
    
    // Furious shake (highest priority)
    if (_lpfG > _shakeFuriousDps) {
        if (!_furiousShaking) {
            _furiousShaking = true;
            _furiousStartMs = now;
            MD_PRINT("furious-start g=%.1f\n", _lpfG);
        }
        if (now - _furiousStartMs > FURIOUS_MS) {
            MD_EVENT("FURIOUS_SHAKE");
            if (_handler) _handler(Event::FuriousShake);
            return Event::FuriousShake;
        }
    }
    // Normal shake
    else if (_lpfG > _shakeAngryDps) {
        if (!_shaking) {
            _shaking = true;
            _shakeStartMs = now;
            MD_PRINT("shake-start g=%.1f\n", _lpfG);
        }
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
    
    // 4) Tap detection (quick gyro spike)
    if (_lpfG > _tapThresh && now - _lastTapMs > TAP_COOLDOWN_MS) {
        _lastTapMs = now;
        MD_EVENT("TAP");
        if (_handler) _handler(Event::Tap);
        return Event::Tap;
    }
    
    // 5) Tilt detection (accelerometer angles)
    float tiltX = atan2f(_lpfAy, _lpfAz) * 180.0f / M_PI;
    float tiltY = atan2f(_lpfAx, _lpfAz) * 180.0f / M_PI;
    
    if (fabsf(tiltX) > _tiltDeg || fabsf(tiltY) > _tiltDeg) {
        // Only report tilt if not already shaking
        if (!_shaking && !_furiousShaking) {
            MD_EVENT("TILT");
            if (_handler) _handler(Event::Tilt);
            return Event::Tilt;
        }
    }
    
    return Event::None;
}
