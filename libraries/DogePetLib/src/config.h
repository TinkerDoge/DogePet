// DogePetLib Configuration
// Centralized configuration for all modules
#pragma once

#include <Arduino.h>
#include <stdint.h>

// =============================================================================
// HARDWARE PINS - GPIO Configuration
// =============================================================================

// I2C Bus (OLED + MPU6050)
static constexpr uint8_t I2C_SDA = 6;
static constexpr uint8_t I2C_SCL = 5;

// Button/Touch (Active HIGH - HIGH when touched)
static constexpr uint8_t FUNC_BTN = 41;

// Optional chin touch (compile-time enable)
#define TOUCH_CHIN_ENABLED
static constexpr uint8_t TOUCH_CHIN = 1;

// I2S Audio Output (MAX98357A)
static constexpr uint8_t I2S_BCLK = 17;
static constexpr uint8_t I2S_LRC  = 16;
static constexpr uint8_t I2S_DO   = 33;

// I2S Microphone Input (INMP441)
static constexpr uint8_t I2S_DI = 11;

// Status LED (WS2812)
static constexpr uint8_t LED_PIN = 48;

// Battery ADC
static constexpr uint8_t VBAT_PIN = 15;

// Vibration Motors (PWM capable)
static constexpr uint8_t VIBRO_LEFT  = 4;
static constexpr uint8_t VIBRO_RIGHT = 3;  // Note: GPIO3 is strapping pin

// =============================================================================
// DISPLAY SETTINGS
// =============================================================================
static constexpr uint8_t  SCREEN_W       = 128;
static constexpr uint8_t  SCREEN_H       = 64;
static constexpr int8_t   OLED_RESET     = -1;
static constexpr uint8_t  SCREEN_ADDR    = 0x3C;
static constexpr uint8_t  DEFAULT_OLED_CONTRAST  = 255;

// =============================================================================
// EYE APPEARANCE
// =============================================================================
static constexpr uint8_t DEFAULT_EYE_WIDTH         = 40;
static constexpr uint8_t DEFAULT_EYE_HEIGHT        = 40;
static constexpr uint8_t DEFAULT_EYE_BORDER_RADIUS = 8;
static constexpr int8_t  DEFAULT_EYE_SPACING       = 10;
static constexpr int8_t  DEFAULT_EYE_OFFSET_X      = 0;
static constexpr int8_t  DEFAULT_EYE_OFFSET_Y      = 0;

// Eye Animation Timing (seconds for RoboEyes)
static constexpr bool    DEFAULT_EYE_AUTO_BLINK  = true;
static constexpr bool    DEFAULT_EYE_IDLE_MODE   = true;
static constexpr bool    DEFAULT_EYE_CURIOUS_MODE   = true;
static constexpr uint8_t DEFAULT_CURIOUS_INTERVAL  = 2;
static constexpr uint8_t DEFAULT_CURIOUS_VARIATION = 2;
static constexpr uint8_t DEFAULT_BLINK_INTERVAL  = 3;
static constexpr uint8_t DEFAULT_BLINK_VARIATION = 1;
static constexpr uint8_t DEFAULT_IDLE_INTERVAL   = 4;
static constexpr uint8_t DEFAULT_IDLE_VARIATION  = 2;

// =============================================================================
// AUDIO SETTINGS
// =============================================================================
static constexpr uint32_t AUDIO_SAMPLE_RATE = 22050;
static constexpr uint8_t  DEFAULT_AUDIO_VOLUME      = 50;  // 0-100

// Microphone
static constexpr uint32_t MIC_SAMPLE_RATE = 22050;
static constexpr int      MIC_CHANGE_THRESHOLD_DB = 5;
static constexpr uint32_t MIN_MIC_LOG_INTERVAL_MS = 500;

// =============================================================================
// HAPTICS SETTINGS
// =============================================================================
static constexpr uint8_t DEFAULT_HAPTIC_INTENSITY = 255;  // 0-255
static constexpr uint8_t HAPTIC_CLICK_MS  = 40;

// =============================================================================
// LED SETTINGS
// =============================================================================
static constexpr uint8_t LED_BRIGHTNESS = 60;  // 0-255
static constexpr uint8_t LED_COLOR_R    = 255;
static constexpr uint8_t LED_COLOR_G    = 100;
static constexpr uint8_t LED_COLOR_B    = 0;

// =============================================================================
// POWER SETTINGS
// =============================================================================
static constexpr float   VBAT_MIN_V      = 3.2f;
static constexpr float   VBAT_MAX_V      = 4.05f;
static constexpr float   VBAT_CAL        = 1.073f;  // Calibrated: multimeter 3.55V = system 3.48V
static constexpr uint8_t VBAT_SAMPLES    = 12;
static constexpr uint8_t LOW_BATT_WARN   = 15;
static constexpr uint8_t CRIT_BATT_WARN  = 5;
static constexpr uint32_t VBAT_LOG_INTERVAL_MS = 30000;  // Log battery status every 30 seconds
static constexpr uint32_t VBAT_READ_INTERVAL_MS = 1000;  // Read battery voltage every 1 second

// =============================================================================
// TIMING & BEHAVIOR
// =============================================================================
static constexpr uint32_t DISPLAY_UPDATE_MS   = 16;   // ~60Hz (improved from 30ms/~33Hz for smoother animations)
static constexpr uint16_t DEBOUNCE_MS         = 30;
static constexpr uint16_t BTN_DEBOUNCE_MS     = 30;
static constexpr uint16_t BTN_LONG_PRESS_MS   = 1000;
static constexpr uint16_t TAP_MAX_MS          = 300;
static constexpr uint16_t HOLD_MIN_MS         = 400;

// Sleep/Power Saving
static constexpr uint32_t DEFAULT_IDLE_TIMEOUT_MS  = 60000;   // 1 min to dim
static constexpr uint32_t DEFAULT_SLEEP_TIMEOUT_MS = 120000;  // 2 min to sleep
static constexpr uint8_t  DIM_BRIGHTNESS   = 30;
// Sleep mode power saving
static constexpr uint32_t VBAT_READ_INTERVAL_SLEEP_MS = 10000;  // Read battery every 10s when sleeping (vs 1s active)
static constexpr uint32_t SLEEP_LOOP_DELAY_MS = 100;            // Delay in main loop when sleeping (reduces CPU usage)

// =============================================================================
// MOTION/IMU SETTINGS
// =============================================================================
static constexpr uint32_t IMU_TICK_MS        = 40;      // ~25Hz polling
static constexpr float    DEFAULT_TILT_THRESHOLD_DEG = 20.0f;   // Tilt detection
static constexpr float    DEFAULT_SHAKE_ANGRY_DPS    = 220.0f;  // Shake detection
static constexpr float    DEFAULT_SHAKE_FURIOUS_DPS  = 250.0f;  // Hard shake
static constexpr uint16_t SHAKE_MS           = 100;     // Sustained shake window
static constexpr uint16_t FURIOUS_MS         = 180;     // Furious shake window
static constexpr float    STILL_G_THRESH     = 0.08f;   // Still detection
static constexpr float    AZ_1G_TOL          = 0.15f;   // az within 1g±tol
static constexpr float    DEFAULT_TAP_SPIKE_DPS      = 120.0f;  // Tap detection threshold
static constexpr uint32_t TAP_COOLDOWN_MS    = 500;     // Min time between taps

// LPF noise thresholds
static constexpr float ACCEL_NOISE = 0.02f;
static constexpr float GYRO_NOISE  = 5.0f;

// =============================================================================
// DEBUG FLAGS
// =============================================================================
static constexpr bool ENABLE_MOTION_DEBUG = false;  // Motion debug prints
static constexpr bool ENABLE_AUDIO_DEBUG  = false;  // Audio debug prints
static constexpr bool DEBUG_SERIAL        = true;   // General serial output
static constexpr bool DEBUG_I2C_SCAN      = true;   // I2C scan at boot

// =============================================================================
// DEVICE INFO
// =============================================================================
static constexpr const char* DEVICE_NAME   = "DogePet";
static constexpr const char* FIRMWARE_VER  = "2.0.0";
static constexpr const char* HARDWARE_REV  = "ESP32-S3-Mini";

// =============================================================================
// CONVENIENCE ALIASES (without DEFAULT_ prefix)
// =============================================================================
#define OLED_CONTRAST       DEFAULT_OLED_CONTRAST
#define EYE_WIDTH           DEFAULT_EYE_WIDTH
#define EYE_HEIGHT          DEFAULT_EYE_HEIGHT
#define EYE_BORDER_RADIUS   DEFAULT_EYE_BORDER_RADIUS
#define EYE_SPACING         DEFAULT_EYE_SPACING
#define EYE_AUTO_BLINK      DEFAULT_EYE_AUTO_BLINK
#define EYE_IDLE_MODE       DEFAULT_EYE_IDLE_MODE
#define BLINK_INTERVAL      DEFAULT_BLINK_INTERVAL
#define BLINK_VARIATION     DEFAULT_BLINK_VARIATION
#define IDLE_INTERVAL       DEFAULT_IDLE_INTERVAL
#define IDLE_VARIATION      DEFAULT_IDLE_VARIATION
#define TILT_THRESHOLD_DEG  DEFAULT_TILT_THRESHOLD_DEG
#define SHAKE_ANGRY_DPS     DEFAULT_SHAKE_ANGRY_DPS
#define SHAKE_FURIOUS_DPS   DEFAULT_SHAKE_FURIOUS_DPS
#define TAP_SPIKE_DPS       DEFAULT_TAP_SPIKE_DPS
#define IDLE_TIMEOUT_MS     DEFAULT_IDLE_TIMEOUT_MS
#define SLEEP_TIMEOUT_MS    DEFAULT_SLEEP_TIMEOUT_MS
