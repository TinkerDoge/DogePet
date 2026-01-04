#pragma once

#include <Arduino.h>
#include <Preferences.h>

struct DogeSettings {
    char bot_name[32];
    
    struct {
        uint8_t width;
        uint8_t height;
        uint8_t radius;
        uint8_t spacing;
        bool auto_blink;
        bool idle_mode;
        bool sweat;
        bool cyclops;
        uint8_t blink_int;
        uint8_t idle_int;
    } eye;
    
    struct {
        float tilt;
        float shake;
        float furious;
        float tap;
    } motion;
    
    struct {
        uint32_t idle_ms;
        uint32_t sleep_ms;
    } power;
    
    struct {
        uint8_t vol;
        uint8_t mic_thresh;
    } audio;
    
    struct {
        uint8_t brightness;
        uint8_t r, g, b;
    } led;
    
    struct {
        uint8_t contrast;
        bool flip;
    } screen;
    
    uint8_t haptic_int;
};

extern DogeSettings settings;

class ConfigManager {
public:
    static void begin();
    static void load();
    static void save();
    static void resetToDefaults();
    
    static String getJson();
    static bool applyJson(String json);
    
private:
    static Preferences prefs;
};
