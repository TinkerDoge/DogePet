// Motion.h
#pragma once

#include <Arduino.h>
#include <functional>
// Motion debugging (Serial prints). Off by default.
static constexpr bool ENABLE_MOTION_DEBUG = false;
// ---- Global Motion debug flag ----------------------------------------------
// Runtime on/off without recompiling other modules. Defaults from config.h.
extern bool gMotionDebug;

// Optional convenience to toggle at runtime.
inline void MotionSetDebug(bool on) { gMotionDebug = on; }


/**
 * @brief High‑level motion interpreter for the MPU6050.
 *
 * Encapsulates raw I2C communication and filters sensor data to detect
 * gestures such as tilt, shake, furious shake, tap and inactivity.
 * Thresholds are configurable at runtime.
 */
class Motion {
public:
    // Discrete motion events reported to listeners
    enum class Event {
        None,
        Tilt,          ///< Gentle tilt beyond tiltThresholdDeg
        Shake,         ///< Brief shake beyond shakeAngryDps
        FuriousShake,  ///< Sustained shake beyond shakeFuriousDps
        Tap,           ///< Single tap detected by accelerometer spike
        Still          ///< Device returned to a stationary/flat position
    };

    // Constructor: pass in I2C pins and optional MPU address
    Motion(int sdaPin, int sclPin, uint8_t mpuAddr = 0x68);

    /**
     * @brief Initialize the sensor.  Should be called in setup().
     * Returns true on success.
     */
    bool begin();

    /**
     * @brief Poll the sensor and update internal filters.
     * Call this regularly (e.g. every 20–50 ms).  Returns the latest event.
     */
    Event update();

    // Accessors for filtered accelerometer/gyro values (g and dps)
    float getAccelX() const;
    float getAccelY() const;
    float getAccelZ() const;
    float getGyroMagnitude() const;

    // Threshold configuration
    void setTiltThresholdDeg(float degrees);        // default: 20.0
    void setShakeAngryThresholdDps(float dps);      // default: 200.0
    void setShakeFuriousThresholdDps(float dps);    // default: 280.0
    void setTapThreshold(float sum);                // default: 0.5
    void setStillThreshold(float accelG, float azTol); // default: 0.06, 0.12

    // Callback registration: called when the corresponding event occurs
    void onEvent(std::function<void(Event)> handler);

    // Reset motion state (e.g. after changing thresholds)
    void reset();

private:
    // Internal helpers: low‑pass filter, raw read, etc.
    void readRaw(float &ax, float &ay, float &az, float &gx, float &gy, float &gz);
    float lpf(float prev, float current, float alpha = 0.5f);

    // Pin and address
    int _sdaPin, _sclPin;
    uint8_t _addr;

    // Filtered sensor values
    float _lpfAx, _lpfAy, _lpfAz, _lpfG;

    // Thresholds
    float _tiltDeg;
    float _shakeAngryDps;
    float _shakeFuriousDps;
    float _tapThresh;
    float _stillGThresh;
    float _az1gTol;

    // Event state
    bool _shaking;
    bool _furiousShaking;
    uint32_t _shakeStartMs;
    uint32_t _furiousStartMs;
    uint32_t _lastUpdateMs;
    uint32_t _lastTapMs;

    // Callback
    std::function<void(Event)> _handler;
};


//// Implementation details below
/*
Motion motion(I2C_SDA, I2C_SCL);

void onMotion(Motion::Event e) {
  switch (e) {
    case Motion::Event::FuriousShake: Status.set(Mood::Furious); break;
    case Motion::Event::Shake:        Status.set(Mood::Angry);   break;
    case Motion::Event::Tilt:         Status.bump(Mood::Happy, MOOD_HOLD_MS); break;
    case Motion::Event::Tap:          Status.signalTap();        break; // e.g., blink or wake
    case Motion::Event::Still:        Status.maybeSleep();       break;
    default: break;
  }
}

void setup() {
  motion.begin();
  motion.onEvent(onMotion);
// optional runtime override
  MotionSetDebug(true);   // or false to silence
}

In your loop():

motion.update();      // emits events → StatusManager
Status.update();      // applies timers, transitions, pushes params to modules
Animations.update();  // reads current status params
UI.update();          // draws

*/