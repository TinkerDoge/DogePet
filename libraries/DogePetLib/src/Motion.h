// Motion.h - High-level motion interpreter for MPU6050
#pragma once

#include <Arduino.h>
#include <functional>

// Runtime debug flag (can be toggled without recompiling)
extern bool gMotionDebug;
inline void MotionSetDebug(bool on) { gMotionDebug = on; }

/**
 * @brief High-level motion interpreter for the MPU6050.
 * 
 * Encapsulates raw I2C communication and filters sensor data to detect
 * gestures such as tilt, shake, furious shake, tap and inactivity.
 * Thresholds are configurable at runtime.
 * 
 * Can be used as an instance or via static methods for compatibility.
 */
class Motion {
public:
    // Discrete motion events reported to listeners
    enum class Event {
        None,
        Tilt,           // Gentle tilt beyond threshold
        Shake,          // Brief shake
        FuriousShake,   // Sustained hard shake
        Tap,            // Single tap detected
        Still           // Device returned to stationary position
    };

    // =========================================================================
    // STATIC API (for compatibility with old code)
    // =========================================================================
    static void init();             // Initialize global instance
    static Event update();          // Update and return event
    static bool isReady();          // Check if initialized
    static void calibrate();        // Recalibrate sensor
    static float getGyroMag();      // Get gyro magnitude (dps)

    // =========================================================================
    // INSTANCE API
    // =========================================================================
    
    // Constructor: uses I2C pins from config.h
    Motion();
    
    // Constructor with custom pins
    Motion(int sdaPin, int sclPin, uint8_t mpuAddr = 0x68);

    /**
     * @brief Initialize the sensor. Call in setup().
     * @return true on success
     */
    bool begin();

    /**
     * @brief Poll the sensor and update internal filters (instance method).
     * Call this regularly (every 20-50ms). Returns the latest event.
     */
    Event updateInstance();

    // Accessors for filtered values (g and dps)
    float getAccelX() const { return _lpfAx; }
    float getAccelY() const { return _lpfAy; }
    float getAccelZ() const { return _lpfAz; }
    float getGyroMagnitude() const { return _lpfG; }
    
    // Check if device is currently moving
    bool isMoving() const { return _moving; }
    bool isShaking() const { return _shaking; }
    bool isFuriousShaking() const { return _furiousShaking; }

    // Threshold configuration (defaults from config.h)
    void setTiltThresholdDeg(float degrees);
    void setShakeAngryThresholdDps(float dps);
    void setShakeFuriousThresholdDps(float dps);
    void setTapThreshold(float dps);
    void setStillThreshold(float accelG, float azTol);

    // Callback registration
    void onEvent(std::function<void(Event)> handler);

    // Reset motion state
    void reset();
    
    // Recalibrate (device must be still) - instance method
    void calibrateInstance();

private:
    void readRaw(float &ax, float &ay, float &az, float &gx, float &gy, float &gz);
    float lpf(float prev, float current, float alpha = 0.5f);

    // Pin and address
    int _sdaPin, _sclPin;
    uint8_t _addr;
    bool _initialized;

    // Filtered sensor values
    float _lpfAx, _lpfAy, _lpfAz, _lpfG;

    // Thresholds
    float _tiltDeg;
    float _shakeAngryDps;
    float _shakeFuriousDps;
    float _tapThresh;
    float _stillGThresh;
    float _az1gTol;

    // State
    bool _moving;
    bool _shaking;
    bool _furiousShaking;
    uint32_t _shakeStartMs;
    uint32_t _furiousStartMs;
    uint32_t _lastUpdateMs;
    uint32_t _lastTapMs;
    bool _wasStill;

    // Callback
    std::function<void(Event)> _handler;
    
    // Static global instance
    static Motion _instance;
};
