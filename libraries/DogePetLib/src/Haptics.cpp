// Haptics.cpp - Vibration motor control with non-blocking patterns
#include "Haptics.h"
#include "Settings.h"

// Track if LEDC is set up
static bool ledcSetup = false;

// Purr state
bool Haptics::purring = false;
uint32_t Haptics::purrPhaseMs = 0;
uint8_t Haptics::purrPhase = 0;

// Pattern state (for tap, double-tap, alarm)
uint32_t Haptics::patternStartMs = 0;
uint8_t Haptics::patternPhase = 0;
bool Haptics::patternActive = false;

// Helper: Get intensity scaled to 80-100% range for weak motors
// Settings::haptic.intensity (0-255) -> 80-100% PWM (204-255)
static uint8_t getScaledIntensity(uint8_t settingsLevel) {
    // (settingsLevel / 255) * (80% to 100%) 
    // = (settingsLevel * 0.8) / 255 + 0.8 = (settingsLevel / 319) + 204
    // Simplified: map 0-255 to 204-255 range
    return 204 + (settingsLevel / 5);  // 204-255
}

void Haptics::init() {
    analogWrite(VIBRO_LEFT, 0);
    analogWrite(VIBRO_RIGHT, 0);
    ledcSetup = true;
    Serial.println("{\"status\":\"info\",\"msg\":\"Haptics Init (LEDC mode)\"}");
}

void Haptics::stop() {
    analogWrite(VIBRO_LEFT, 0);
    analogWrite(VIBRO_RIGHT, 0);
    patternActive = false;
    purring = false;
}

void Haptics::setMotor(int pin, int pwm) {
    if (pwm < 0) pwm = 0;
    if (pwm > 255) pwm = 255;
    analogWrite(pin, pwm);
}

// DEPRECATED: Only use buzz() for debug or blocking scenarios
// For real-time use, call patternTick() in main loop instead
void Haptics::buzz(uint8_t left, uint8_t right, uint32_t durationMs) {
    analogWrite(VIBRO_LEFT, left);
    analogWrite(VIBRO_RIGHT, right);
    delay(durationMs);
    analogWrite(VIBRO_LEFT, 0);
    analogWrite(VIBRO_RIGHT, 0);
}

// Non-blocking tap pattern: Single quick pulse (50ms)
// Used for: HEAD_TAP, CHIN_TAP
void Haptics::click() {
    patternStartMs = millis();
    patternPhase = 0;
    patternActive = true;
}

// Non-blocking double-tap pattern: lub-DUB heartbeat rhythm
// Used for: Sensor double-tap, Combo events
void Haptics::doubleClick() {
    patternStartMs = millis();
    patternPhase = 10;  // Start at phase 10 (double-tap pattern)
    patternActive = true;
}

// Non-blocking alarm pattern: Urgent alternating pulses
// Used for: Critical alerts, power warnings
void Haptics::alarm() {
    patternStartMs = millis();
    patternPhase = 20;  // Start at phase 20 (alarm pattern)
    patternActive = true;
}

void Haptics::startPurr() {
    purring = true;
    purrPhaseMs = millis();
    purrPhase = 0;
}

void Haptics::stopPurr() {
    purring = false;
    purrPhase = 0;
    analogWrite(VIBRO_LEFT, 0);
    analogWrite(VIBRO_RIGHT, 0);
}

bool Haptics::isPurring() {
    return purring;
}

// Main update function - call from DogePet.ino loop()
// Handles all non-blocking patterns: click, doubleClick, alarm, purr
void Haptics::patternTick() {
    if (!patternActive && !purring) {
        return;
    }

    uint32_t now = millis();
    uint8_t intensity = getScaledIntensity(Settings::haptic.intensity);

    // ============ TAP PATTERN (phases 0-2) ============
    if (patternPhase >= 0 && patternPhase < 3) {
        uint32_t elapsed = now - patternStartMs;
        
        // Simple single pulse with ramp-up for snappy feel (50ms)
        if (elapsed >= 50) {
            analogWrite(VIBRO_LEFT, 0);
            analogWrite(VIBRO_RIGHT, 0);
            patternActive = false;
        } else {
            // Ramp up from 0 to full intensity over 50ms
            uint8_t pwm = (intensity * elapsed) / 50;
            analogWrite(VIBRO_LEFT, pwm);
            analogWrite(VIBRO_RIGHT, pwm);
        }
    }
    
    // ============ DOUBLE-TAP PATTERN (phases 10-15) ============
    // Rhythm: lub (first pulse 60ms) + 50ms gap + DUB (second pulse 90ms)
    else if (patternPhase >= 10 && patternPhase < 16) {
        uint32_t elapsed = now - patternStartMs;
        
        if (patternPhase == 10) {  // First pulse LUB (60ms)
            if (elapsed < 60) {
                uint8_t pwm = (intensity * 95) / 100;  // 95% intensity
                analogWrite(VIBRO_LEFT, pwm);
                analogWrite(VIBRO_RIGHT, pwm);
            } else {
                patternPhase = 11;
                patternStartMs = now;
            }
        } 
        else if (patternPhase == 11) {  // Gap (50ms)
            if (elapsed >= 50) {
                patternPhase = 12;
                patternStartMs = now;
            } else {
                analogWrite(VIBRO_LEFT, 0);
                analogWrite(VIBRO_RIGHT, 0);
            }
        } 
        else if (patternPhase == 12) {  // Second pulse DUB (90ms, stronger)
            if (elapsed < 90) {
                analogWrite(VIBRO_LEFT, intensity);  // 100% intensity for "DUB"
                analogWrite(VIBRO_RIGHT, intensity);
            } else {
                analogWrite(VIBRO_LEFT, 0);
                analogWrite(VIBRO_RIGHT, 0);
                patternActive = false;
            }
        }
    }
    
    // ============ ALARM PATTERN (phases 20-25) ============
    // Alternating left-right, 80ms each, 3 cycles = 480ms total
    else if (patternPhase >= 20 && patternPhase < 26) {
        uint32_t elapsed = now - patternStartMs;
        uint32_t cycleTime = 0;
        
        // Map elapsed time to 80ms pulses (left, right, left, right, left, right)
        uint8_t pulseIndex = elapsed / 80;
        uint32_t pulseElapsed = elapsed % 80;
        
        if (pulseIndex >= 6) {  // 6 pulses = 480ms total
            analogWrite(VIBRO_LEFT, 0);
            analogWrite(VIBRO_RIGHT, 0);
            patternActive = false;
        } else {
            // Odd pulses = left, Even pulses = right
            uint8_t pwm = (intensity * 100) / 100;  // Full intensity for urgency
            if (pulseIndex % 2 == 0) {
                analogWrite(VIBRO_LEFT, pwm);
                analogWrite(VIBRO_RIGHT, 0);
            } else {
                analogWrite(VIBRO_LEFT, 0);
                analogWrite(VIBRO_RIGHT, pwm);
            }
        }
    }

    // ============ PURR PATTERN ============
    if (purring) {
        uint32_t elapsed = now - purrPhaseMs;
        uint8_t baseIntensity = getScaledIntensity(Settings::haptic.intensity);
        
        // Cat-like purr rhythm: variable intensity pattern
        switch (purrPhase) {
            case 0:  // Medium (85%)
                if (elapsed >= 120) {
                    purrPhase = 1;
                    purrPhaseMs = now;
                } else {
                    analogWrite(VIBRO_LEFT, (baseIntensity * 85) / 100);
                    analogWrite(VIBRO_RIGHT, (baseIntensity * 85) / 100);
                }
                break;
                
            case 1:  // Strong (100%)
                if (elapsed >= 100) {
                    purrPhase = 2;
                    purrPhaseMs = now;
                } else {
                    analogWrite(VIBRO_LEFT, baseIntensity);
                    analogWrite(VIBRO_RIGHT, baseIntensity);
                }
                break;
                
            case 2:  // Medium (85%)
                if (elapsed >= 110) {
                    purrPhase = 3;
                    purrPhaseMs = now;
                } else {
                    analogWrite(VIBRO_LEFT, (baseIntensity * 85) / 100);
                    analogWrite(VIBRO_RIGHT, (baseIntensity * 85) / 100);
                }
                break;
                
            case 3:  // Soft (80%)
                if (elapsed >= 90) {
                    purrPhase = 4;
                    purrPhaseMs = now;
                } else {
                    analogWrite(VIBRO_LEFT, (baseIntensity * 80) / 100);
                    analogWrite(VIBRO_RIGHT, (baseIntensity * 80) / 100);
                }
                break;
                
            case 4:  // Medium (85%) - back to start
                if (elapsed >= 130) {
                    purrPhase = 0;
                    purrPhaseMs = now;
                } else {
                    analogWrite(VIBRO_LEFT, (baseIntensity * 85) / 100);
                    analogWrite(VIBRO_RIGHT, (baseIntensity * 85) / 100);
                }
                break;
        }
    }
}

// For compatibility - old purrTick() redirects to patternTick()
void Haptics::purrTick() {
    patternTick();
}
