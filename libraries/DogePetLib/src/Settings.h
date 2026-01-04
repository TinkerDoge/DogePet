// Settings.h - Runtime and NVS persistent settings
// Dynamic settings: Apply immediately (Face/Display)
// Persistent settings: Saved to NVS, require reboot (Pins, Hardware)
#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// =============================================================================
// DYNAMIC SETTINGS - Apply immediately at runtime
// =============================================================================
struct FaceSettings {
    uint8_t width;
    uint8_t height;
    uint8_t radius;
    int8_t  spacing;
    bool    autoBlink;
    bool    idleMode;
    uint8_t blinkInterval;
    uint8_t idleInterval;
    uint8_t contrast;
    bool    curious;       // Show curious expression
    bool    sweat;         // Show sweat drop
};

struct AudioSettings {
    uint8_t volume;         // 0-100
    bool    micLogEnabled;
};

struct HapticSettings {
    uint8_t intensity;      // 0-255
};

struct LEDSettings {
    uint8_t brightness;     // 0-255
    uint8_t r, g, b;
};

// =============================================================================
// PERSISTENT SETTINGS - Require reboot after change
// =============================================================================
struct MotionSettings {
    float tiltDeg;
    float shakeAngryDps;
    float shakeFuriousDps;
    float tapSpikeDps;
};

struct PowerSettings {
    uint32_t idleTimeoutMs;
    uint32_t sleepTimeoutMs;
};

struct PinSettings {
    uint8_t i2cSda;
    uint8_t i2cScl;
    uint8_t funcBtn;
    uint8_t touchChin;
    uint8_t i2sBclk;
    uint8_t i2sLrc;
    uint8_t i2sDo;
    uint8_t i2sDi;
    uint8_t ledPin;
    uint8_t vbatPin;
    uint8_t vibroLeft;
    uint8_t vibroRight;
};

// =============================================================================
// SETTINGS CLASS
// =============================================================================
class Settings {
public:
    // Dynamic settings (live update)
    static FaceSettings   face;
    static AudioSettings  audio;
    static HapticSettings haptic;
    static LEDSettings    led;
    
    // Persistent settings (reboot required)
    static MotionSettings motion;
    static PowerSettings  power;
    static PinSettings    pins;
    
    // Device info
    static char botName[32];
    
    // Flags
    static bool pendingReboot;  // True if persistent settings changed
    
    // Lifecycle
    static void begin();        // Load from NVS
    static void save();         // Save to NVS
    static void resetDefaults();// Reset all to defaults
    
    // Dynamic settings helpers
    static void applyFaceSettings();   // Push to Face module
    static void applyAudioSettings();  // Push to Audio module
    static void applyHapticSettings(); // Push to Haptics module
    static void applyLEDSettings();    // Push to LED module
    
    // JSON interface for companion app
    static String toJson();                     // All settings as JSON
    static String toDynamicJson();              // Only dynamic settings
    static String toPersistentJson();           // Only persistent settings
    static bool fromJson(const String& json);   // Parse and apply
    
private:
    static Preferences prefs;
    static void loadDefaults();
    static void loadFromNVS();
    static void saveToNVS();
};
