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
    
    enum WaveType {
        WAVE_SQUARE,
        WAVE_NOISE
    };

    enum Mood {
        MOOD_HAPPY,
        MOOD_SAD,
        MOOD_CURIOUS,
        MOOD_ANGRY,
        MOOD_NEUTRAL
    };

    enum WaveEffect {
        EFFECT_NONE,
        EFFECT_MIX,     // Additive (Chord)
        EFFECT_RING     // Multiplicative (Ring Mod/Metallic)
    };

    // Tone generation
    // modFreq: Frequency of second oscillator (0 = disabled)
    // effect: How to combine oscillator 1 and 2
    static void playTone(uint32_t freq, uint32_t durationMs, WaveType type = WAVE_SQUARE, 
                        uint32_t modFreq = 0, WaveEffect effect = EFFECT_NONE);
    
    static void playMelody();  // Startup jingle
    
    // Procedural Speech
    static void speak(Mood mood);

    // Sound effects
    static void chirp();
    static void purrrSound();
    static void surpriseBeep();
    static void yawn();
    
    // Touch interaction sounds
    static void tapSound();      // Gentle tap/beep for head tap
    static void happySound();     // Content/happy sound for petting start
    static void contentSound();   // Satisfied sound for petting end
    static void satisfiedSound(); // Gentle satisfied sound for chin scratch end
    
    // Special FX
    static void playSizzle(); // White noise burst
    
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
