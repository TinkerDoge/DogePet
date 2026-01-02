// DogePet Configuration - Hardware parameters for web configuration
#pragma once

#include <Arduino.h>
#include <stdint.h>

// =============================================================================
// HARDWARE PINS - GPIO Configuration
// =============================================================================

// I2C Bus (OLED + MPU6050)
static constexpr int I2C_SDA = 6;
static constexpr int I2C_SCL = 5;

// Button (Active HIGH - HIGH when pressed)
static constexpr int FUNC_BTN = 41;

// I2S Audio Output (MAX98357A)
static constexpr int I2S_BCLK = 17;
static constexpr int I2S_LRC  = 16;
static constexpr int I2S_DO   = 33;

// I2S Microphone Input (INMP441 dual-channel hardware)
static constexpr int I2S_DI = 11;

// Status LED (WS2812)
static constexpr int LED_PIN = 48;

// Battery ADC
static constexpr int VBAT_PIN = 15;

// Vibration Motors (PWM capable)
static constexpr int VIBRO_LEFT  = 4;
static constexpr int VIBRO_RIGHT = 3;

// =============================================================================
// DISPLAY SETTINGS
// =============================================================================
static constexpr int     SCREEN_W    = 128;
static constexpr int     SCREEN_H    = 64;
static constexpr int     OLED_RESET  = -1;
static constexpr uint8_t SCREEN_ADDR = 0x3C;

// =============================================================================
// EYE APPEARANCE (configurable via web)
// =============================================================================
static constexpr int EYE_WIDTH         = 28;
static constexpr int EYE_HEIGHT        = 40;
static constexpr int EYE_BORDER_RADIUS = 8;
static constexpr int EYE_SPACING       = 10;

// =============================================================================
// TIMING
// =============================================================================
static constexpr uint32_t DISPLAY_UPDATE_MS = 30;  // ~33Hz refresh

// =============================================================================
// PERIPHERAL DEFAULTS (for testing via web)
// =============================================================================

// LED
static constexpr uint8_t LED_BRIGHTNESS = 60;

// Audio
static constexpr uint32_t AUDIO_SAMPLE_RATE = 22050;
static constexpr uint8_t  AUDIO_VOLUME      = 200;

// Microphone
static constexpr uint32_t MIC_SAMPLE_RATE = 22050;

// Battery
static constexpr float   VBAT_MIN_V   = 3.2f;
static constexpr float   VBAT_MAX_V   = 4.05f;
static constexpr float   VBAT_CAL     = 1.0518f;
static constexpr uint8_t VBAT_SAMPLES = 12;

// Button debounce
static constexpr uint16_t BTN_DEBOUNCE_MS = 30;

// =============================================================================
// BLE / WIFI (for future web config)
// =============================================================================
static constexpr const char* DEVICE_NAME = "DogePet";


