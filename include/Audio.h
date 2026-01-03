#ifndef AUDIO_H
#define AUDIO_H

#include <Arduino.h>
#include "config.h"

class Audio {
public:
    static void init();
    static void update();          // Call in loop - monitors mic, logs on change
    static void setMicLogEnabled(bool enabled);
    
    // Microphone
    static int readMicDB();
    static void testMicLog(); // Logs raw data to Serial (deprecated, use update())
    
    // Amplifier
    static void playTone(uint32_t freq, uint32_t durationMs);
    static void playMelody(); // Cute startup melody
    static void stop();
    
    // Cute Sound Effects
    static void chirp();        // Quick happy chirp (chin tap)
    static void purrrSound();   // Descending content sound (chin scratch)
    static void surpriseBeep(); // Ascending surprised beep (combo touch)
    static void yawn();         // Descending sleepy yawn
    
    // Volume control
    static void setVolume(uint8_t vol);  // 0-100
    static uint8_t getVolume();

private:
    static void initI2S();
    static bool isI2SReady;
    static uint8_t volume;
    
    // Change detection for mic logging
    static bool micLogEnabled;
    static int lastMicDB;
    static uint32_t lastMicLogMs;
    
    static constexpr int MIC_CHANGE_THRESHOLD_DB = 3;    // Minimum dB change to log
    static constexpr uint32_t MIN_MIC_LOG_INTERVAL_MS = 100;  // Rate limit
};

#endif
