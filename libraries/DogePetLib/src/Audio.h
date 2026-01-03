// Audio.h - I2S audio for mic input and speaker output
#pragma once

#include <Arduino.h>
#include "config.h"

class Audio {
public:
    static void init();
    static void update();  // Check mic for changes
    
    // Mic reading
    static int readMicDB();
    static void testMicLog();
    static void setMicLogEnabled(bool enabled);
    
    // Tone generation
    static void playTone(uint32_t freq, uint32_t durationMs);
    static void playMelody();  // Startup jingle
    
    // Sound effects
    static void chirp();
    static void purrrSound();
    static void surpriseBeep();
    static void yawn();
    
    // Volume control
    static void setVolume(uint8_t vol);  // 0-100
    static uint8_t getVolume();
    static void stop();
    
private:
    static void initI2S();
    static bool isI2SReady;
    static uint8_t volume;
    
    // Mic change detection
    static bool micLogEnabled;
    static int lastMicDB;
    static uint32_t lastMicLogMs;
};
