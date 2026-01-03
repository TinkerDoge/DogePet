// DogePet Configuration - All configurable parameters
// These serve as DEFAULT values - runtime values are loaded from NVS
#pragma once

#include <Arduino.h>
#include <stdint.h>

// =============================================================================
// HARDWARE PINS - GPIO Configuration (NVS: dogepet_hw)
// =============================================================================

// I2C Bus (OLED + MPU6050)
static constexpr uint8_t I2C_SDA = 6;
static constexpr uint8_t I2C_SCL = 5;

// Button (Active HIGH - HIGH when pressed)
static constexpr uint8_t FUNC_BTN = 41;

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

// Touch/Button Input
// Primary touch is FUNC_BTN on head (Active LOW with pull-up)
// Optional second touch for chin area (uncomment when added)
# define TOUCH_CHIN_ENABLED
static constexpr uint8_t TOUCH_CHIN = 1;  // Optional chin touch sensor (future)

// =============================================================================
// DISPLAY SETTINGS (NVS: dogepet_disp)
// =============================================================================
static constexpr uint8_t  SCREEN_W    = 128;
static constexpr uint8_t  SCREEN_H    = 64;
static constexpr int8_t   OLED_RESET  = -1;
static constexpr uint8_t  SCREEN_ADDR = 0x3C;
static constexpr uint8_t  OLED_CONTRAST = 255;      // 0-255

// =============================================================================
// EYE APPEARANCE (NVS: dogepet_eyes)
// =============================================================================
static constexpr uint8_t EYE_WIDTH         = 28;    // 10-60
static constexpr uint8_t EYE_HEIGHT        = 40;    // 10-64
static constexpr uint8_t EYE_BORDER_RADIUS = 8;     // 0-20
static constexpr int8_t  EYE_SPACING       = 10;    // -20 to 50
static constexpr int8_t  EYE_OFFSET_X      = 0;     // Horizontal offset (-30 to 30)
static constexpr int8_t  EYE_OFFSET_Y      = 0;     // Vertical offset (-20 to 20)

// Eye Animation Timing (seconds for RoboEyes)
static constexpr uint8_t BLINK_INTERVAL    = 3;     // Seconds between blinks
static constexpr uint8_t BLINK_VARIATION   = 1;     // Random variation (+/- seconds)
static constexpr uint8_t IDLE_INTERVAL     = 4;     // Seconds between idle movements
static constexpr uint8_t IDLE_VARIATION    = 2;     // Random variation (+/- seconds)

// Eye Behavior Defaults
static constexpr bool    EYE_AUTO_BLINK    = true;
static constexpr bool    EYE_IDLE_MODE     = true;
static constexpr bool    EYE_CURIOSITY     = false;
static constexpr bool    EYE_CYCLOPS       = false;

// =============================================================================
// AUDIO SETTINGS (NVS: dogepet_audio)
// =============================================================================
static constexpr uint32_t AUDIO_SAMPLE_RATE = 22050;  // Hz
static constexpr uint8_t  AUDIO_VOLUME      = 10;     // 0-100
static constexpr uint8_t  AUDIO_BASS_BOOST  = 0;      // 0-10 (future)

// Startup Melody Notes (Hz) - C5 E5 G5 C6
static constexpr uint16_t MELODY_NOTE_1 = 523;
static constexpr uint16_t MELODY_NOTE_2 = 659;
static constexpr uint16_t MELODY_NOTE_3 = 784;
static constexpr uint16_t MELODY_NOTE_4 = 1047;
static constexpr uint8_t  MELODY_NOTE_DURATION = 100; // ms per note
static constexpr uint8_t  MELODY_NOTE_GAP = 50;       // ms between notes

// Microphone
static constexpr uint32_t MIC_SAMPLE_RATE = 22050;
static constexpr int8_t   MIC_GAIN_DB     = 0;        // dB offset for calibration

// =============================================================================
// HAPTICS SETTINGS (NVS: dogepet_haptics)
// =============================================================================
static constexpr uint8_t  HAPTIC_INTENSITY    = 200;  // 0-255 default motor power
static constexpr uint8_t  HAPTIC_CLICK_MS     = 40;   // Click duration
static constexpr uint8_t  HAPTIC_DOUBLE_GAP   = 80;   // Gap between double-click

// =============================================================================
// LED SETTINGS (NVS: dogepet_led)
// =============================================================================
static constexpr uint8_t LED_BRIGHTNESS = 60;         // 0-255
static constexpr uint8_t LED_COLOR_R    = 255;        // Default color
static constexpr uint8_t LED_COLOR_G    = 100;
static constexpr uint8_t LED_COLOR_B    = 0;

// =============================================================================
// POWER SETTINGS (NVS: dogepet_pwr)
// =============================================================================
static constexpr float   VBAT_MIN_V      = 3.2f;      // Empty battery voltage
static constexpr float   VBAT_MAX_V      = 4.05f;     // Full battery voltage
static constexpr float   VBAT_CAL        = 1.0518f;   // ADC calibration multiplier
static constexpr uint8_t VBAT_SAMPLES    = 12;        // ADC oversampling count
static constexpr uint8_t LOW_BATT_WARN   = 15;        // Low battery warning (%)
static constexpr uint8_t CRIT_BATT_WARN  = 5;         // Critical battery (%)

// =============================================================================
// TIMING & BEHAVIOR (NVS: dogepet_cfg)
// =============================================================================
static constexpr uint32_t DISPLAY_UPDATE_MS   = 30;   // ~33Hz refresh
static constexpr uint16_t BTN_DEBOUNCE_MS     = 30;   // Button debounce
static constexpr uint16_t BTN_LONG_PRESS_MS   = 1000; // Long press threshold
static constexpr uint16_t BTN_DOUBLE_TAP_MS   = 300;  // Double-tap window

// Sleep/Power Saving
static constexpr uint32_t IDLE_TIMEOUT_MS     = 60000;  // 1 min to dim
static constexpr uint32_t SLEEP_TIMEOUT_MS    = 120000; // 2 min to sleep
static constexpr uint8_t  DIM_BRIGHTNESS      = 30;     // Dimmed OLED brightness

// =============================================================================
// MOTION/IMU SETTINGS (NVS: dogepet_imu)
// =============================================================================
static constexpr uint8_t  IMU_UPDATE_HZ       = 50;     // IMU polling rate
static constexpr float    TILT_THRESHOLD_DEG  = 15.0f;  // Tilt detection threshold
static constexpr float    SHAKE_THRESHOLD_DPS = 200.0f; // Shake detection (deg/sec)
static constexpr uint16_t MOTION_DEBOUNCE_MS  = 200;    // Motion event debounce

// Calibration offsets (set by calibration routine)
static constexpr int16_t  IMU_OFFSET_AX = 0;
static constexpr int16_t  IMU_OFFSET_AY = 0;
static constexpr int16_t  IMU_OFFSET_AZ = 0;
static constexpr int16_t  IMU_OFFSET_GX = 0;
static constexpr int16_t  IMU_OFFSET_GY = 0;
static constexpr int16_t  IMU_OFFSET_GZ = 0;

// =============================================================================
// DEVICE INFO (NVS: dogepet_dev)
// =============================================================================
static constexpr const char* DEVICE_NAME    = "DogePet";
static constexpr const char* FIRMWARE_VER   = "2.0.0";
static constexpr const char* HARDWARE_REV   = "ESP32-S3-Mini";

// =============================================================================
// DEBUG FLAGS
// =============================================================================
static constexpr bool DEBUG_SERIAL    = true;   // Enable serial debug output
static constexpr bool DEBUG_I2C_SCAN  = true;   // Run I2C scan at boot
static constexpr bool DEBUG_BOOT_BEEP = true;   // Play sound during boot

// =============================================================================
// NVS NAMESPACE DEFINITIONS
// =============================================================================
#define NVS_NS_HARDWARE   "dogepet_hw"
#define NVS_NS_EYES       "dogepet_eyes"
#define NVS_NS_AUDIO      "dogepet_audio"
#define NVS_NS_HAPTICS    "dogepet_hap"
#define NVS_NS_LED        "dogepet_led"
#define NVS_NS_POWER      "dogepet_pwr"
#define NVS_NS_CONFIG     "dogepet_cfg"
#define NVS_NS_IMU        "dogepet_imu"
#define NVS_NS_DEVICE     "dogepet_dev"


