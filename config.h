// Shared configuration constants for motion/emotion thresholds and timings
#pragma once

#include <Arduino.h>
#include <stdint.h>

// Ensure stdint types are available
#if !defined(__STDINT_H) && !defined(_STDINT_H_)
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long int32_t;
typedef short int16_t;
#endif

// === Hardware pins & display (centralized) ===
static constexpr int      I2C_SDA        = 9;
static constexpr int      I2C_SCL        = 8;
static constexpr int      TOUCH_PIN      = 13;
static constexpr int      FUNC_BTN       = 1;
static constexpr int      I2S_LRC        = 10;  // WS/LRCLK
static constexpr int      I2S_BCLK       = 11;  // BCLK
static constexpr int      I2S_DO         = 12;  // I2S data out
static constexpr int      I2S_DI         = 2;   // (MIC)
static constexpr int      LED_PIN        = 48;  // WS2812
static constexpr uint8_t  LED_BRIGHTNESS = 60;
static constexpr int      VBAT_PIN       = 7;

// Display constants
static constexpr int      SCREEN_W       = 128;
static constexpr int      SCREEN_H       = 64;
static constexpr int      OLED_RESET     = -1;
static constexpr uint8_t  SCREEN_ADDR    = 0x3C;
// BLE/device
static constexpr const char* BLE_DEVICE_NAME = "PetBot";

// === Bot talkativeness and feature flags ===
// 0 = silent, 1 = rare, 2 = occasional, 3 = chatty, 4 = chatty+, 5 = chattyPro
static constexpr uint8_t BOT_TALKATIVE_LEVEL = 3;
// Enable/disable categories
static constexpr bool ENABLE_IDLE_CHATTER = true;
static constexpr bool ENABLE_NOTIFY_SFX   = true;
static constexpr bool ENABLE_MODE_SFX     = true;
static constexpr bool ENABLE_STARTUP_SFX  = true;
static constexpr bool ENABLE_LED_STATUS   = true;
static constexpr bool ENABLE_MOOD_SFX     = true;  // play mood-change SFX
static constexpr bool ENABLE_BLE_SFX      = true;  // BLE UI/connect/disconnect SFX
static constexpr bool ENABLE_TAP_SFX      = true;  // blink/tap tick
static constexpr bool ENABLE_BINARY_CHATTER = true;  // show binary sequences when chattering
// Play idle procedural audio during local (non-AI) chatter
static constexpr bool ENABLE_IDLE_AUDIO_CHATTER = false; // sound only when AI chatter/response
// Motion/emotion behavior toggles
static constexpr bool ENABLE_TILT_HAPPY    = true;  // tilt -> happy
static constexpr bool ENABLE_SHAKE_ANGRY   = true;  // shake -> angry
static constexpr bool ENABLE_SHAKE_FURIOUS = true;  // sustained hard shake -> furious
static constexpr bool ENABLE_RANDOM_JIGGLE = true;  // periodic micro-jiggle

// === Emotion & Motion Detection Thresholds ===
static constexpr float     TILT_HAPPY_DEG     = 20.0f;   // gentle tilt ⇒ happy
static constexpr float     SHAKE_ANGRY_DPS    = 200.0f;  // quick gyro spike ⇒ angry
static constexpr float     SHAKE_FURIOUS_DPS  = 280.0f;  // higher gyro spike ⇒ furious
static constexpr uint16_t  SHAKE_MS           = 120;     // sustained spike window
static constexpr uint16_t  FURIOUS_MS         = 200;     // longer sustained spike for furious
static constexpr float     STILL_G_THRESH     = 0.06f;   // |ax|,|ay| below this and az≈+1g → flat
static constexpr float     AZ_1G_TOL          = 0.12f;   // az within 1g±tol

// === Timing Constants ===
static constexpr uint32_t  MOOD_HOLD_MS       = 2500;    // how long to keep happy/angry
static constexpr uint32_t  FURIOUS_HOLD_MS    = 4000;    // longer duration for furious
static constexpr uint32_t  MOOD_SFX_GAP_MS    = 600;     // debounce for mood SFX
// UI/system timings
static constexpr uint32_t  HOLD_TIME_MS       = 2000;    // FUNC_BTN hold for BLE toggle
static constexpr uint32_t  NOTIF_POPUP_MS     = 4000;    // popup visible duration
static constexpr uint32_t  NOTIF_SFX_GAP_MS   = 500;     // min gap between notif sfx
static constexpr uint32_t  NOTIF_SCROLL_DT_MS = 90;      // scroll tick
static constexpr uint32_t  VBAT_CACHE_MS      = 3000;    // cache VBAT for UI
static constexpr uint32_t  VBAT_LOG_MS        = 3000;    // log interval
static constexpr uint32_t  STARTUP_SFX_DELAY  = 200;     // ms after boot to play startup
static constexpr uint32_t  EMO_TICK_MS        = 40;      // ~25Hz emotion engine
static constexpr uint32_t  IMU_TICK_MS        = 40;      // ~25Hz liveliness
static constexpr uint32_t  NON_EYES_DELAY_MS  = 12;      // faster polling so triple-tap is reliable on all faces
// Display update throttling for power savings
static constexpr uint32_t  DISPLAY_UPDATE_MS  = 50;      // ~20Hz display updates
static constexpr uint32_t  ROBOEYES_UPDATE_MS = 30;      // ~33Hz RoboEyes updates
// Power management
static constexpr uint32_t  SLEEP_AFTER_MS      = 30000;   // enter sleep after 30s of no movement
static constexpr uint32_t  DIM_AFTER_MS        = 10000;   // dim display after 10s of no movement
static constexpr uint8_t   SLEEP_BRIGHTNESS    = 10;      // dimmed brightness (0-255)
static constexpr uint8_t   NORMAL_BRIGHTNESS   = 60;      // normal brightness
static constexpr uint32_t  TOAST_UPDATE_MS     = 2000;     // minimum time between toast redraws (anti-flicker)

// === Tap Detection Constants ===
static constexpr float     TAP_SPIKE_DPS      = 140.0f;  // quick rotation spike => blink
static constexpr uint32_t  TAP_MIN_GAP_MS     = 600;     // min time between blinks
static constexpr uint32_t  TAP_COOLDOWN_MS    = 200;     // cooldown after tap detection

// === Debug System Constants ===
static constexpr float     ACCEL_DELTA        = 0.05f;   // g-units
static constexpr float     GYRO_DELTA         = 5.0f;    // deg/s

// === Battery config (moved from sketch) ===
static constexpr float     VBAT_MIN_V   = 3.2f;
static constexpr float     VBAT_MAX_V   = 4.05f;
static constexpr uint8_t   VBAT_SAMPLES = 12;
static constexpr float     VBAT_CAL     = 1.0518f; // scale ADC to multimeter
// Charging detection
static constexpr float     VBAT_CHARGE_V        = 4.04f;  // align at/just below MAX so it can trip
static constexpr float     VBAT_CHARGE_HYST_V   = 0.04f;  // drop below (VBAT_CHARGE_V - this) to clear
static constexpr uint8_t   VBAT_CHARGE_MIN_COUNT = 2;     // consecutive reads to confirm

// === Audio defaults ===
static constexpr uint32_t  AUDIO_SAMPLE_RATE   = 22050;
static constexpr uint8_t   AUDIO_DEFAULT_VOLUME = 50;  // 0..255

// === Microphone (I2S Input) Configuration ===
// Note: Microphone sensitivity is boosted with 64x gain in audio.cpp
static constexpr bool      ENABLE_MIC_LISTENING = true;  // Enable microphone feature
static constexpr uint32_t  MIC_SAMPLE_RATE      = 16000; // 16kHz for voice/noise detection
static constexpr uint16_t  MIC_BUFFER_SIZE      = 1024;  // Buffer size for audio samples (larger = better averaging)
static constexpr float     MIC_NOISE_THRESHOLD  = 0.5f; // RMS threshold for noise detection (lower = more sensitive)
static constexpr uint32_t  MIC_CHECK_INTERVAL   = 100;   // Check every 100ms
static constexpr uint32_t  MIC_COOLDOWN_MS      = 2000;  // Minimum time between reactions
static constexpr uint32_t  MIC_FEEDBACK_COOLDOWN_MS = 300; // Cooldown after audio playback
// Initial motion/emotion state defaults
static constexpr bool      DEFAULT_SHAKING          = true;
static constexpr bool      DEFAULT_FURIOUS_SHAKING  = true;
static constexpr bool      DEFAULT_FURIOUS_JIGGLING = true;
static constexpr bool      DEFAULT_JIGGLING         = true;

// === Gemini AI Configuration ===
static constexpr bool      ENABLE_GEMINI_AI         = true;  // Enable AI features
// Debug logging for AI (set to true to see detailed logs)
static constexpr bool      ENABLE_AI_DEBUG_LOGS     = false;
static constexpr const char* GEMINI_API_KEY         = "AIzaSyBUI39g_byb1KUm_iNkvQF4y86mxCV8A28";    // Set your API key here
static constexpr const char* GEMINI_MODEL           = "gemini-2.5-flash";    // AI model to use

static constexpr uint32_t  GEMINI_COOLDOWN_MS      = 45000; // Longer cooldown to save tokens
// Randomized cooldown range (used instead of fixed, unless both are equal)
static constexpr uint32_t  GEMINI_COOLDOWN_MIN_MS  = 30000; // 30 seconds
static constexpr uint32_t  GEMINI_COOLDOWN_MAX_MS  = 90000; // 90 seconds
static constexpr bool      ENABLE_AI_CHATTER        = true;  // Enable background AI conversations
static constexpr uint32_t  AI_CHATTER_INTERVAL_MS   = 60000; // 60 seconds between background chatter (token saving)

// === WiFi Configuration ===
static constexpr bool      ENABLE_WIFI              = true;  // Enable WiFi for AI features
static constexpr const char* WIFI_SSID              = "NOVA";    // Set your WiFi network name
static constexpr const char* WIFI_PASSWORD          = "22122024";    // Set your WiFi password
static constexpr uint32_t  WIFI_CONNECT_TIMEOUT_MS = 30000; // WiFi connection timeout


