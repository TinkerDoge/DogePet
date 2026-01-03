#include "include/Haptics.h"

// Track if LEDC is set up for these pins
static bool ledcSetup = false;

void Haptics::init() {
    // Use analogWrite to configure LEDC from the start
    // This avoids issues with switching between GPIO and LEDC modes
    analogWrite(VIBRO_LEFT, 0);
    analogWrite(VIBRO_RIGHT, 0);
    ledcSetup = true;
    
    Serial.println("{\"status\":\"info\",\"msg\":\"Haptics Init (LEDC mode)\"}");
}

void Haptics::stop() {
    // Just set PWM to 0, don't detach - keeps LEDC configured
    analogWrite(VIBRO_LEFT, 0);
    analogWrite(VIBRO_RIGHT, 0);
}

void Haptics::setMotor(int pin, int pwm) {
    // Always use analogWrite - it handles 0 properly
    if (pwm < 0) pwm = 0;
    if (pwm > 255) pwm = 255;
    analogWrite(pin, pwm);
}

void Haptics::buzz(uint8_t left, uint8_t right, uint32_t durationMs) {
    analogWrite(VIBRO_LEFT, left);
    analogWrite(VIBRO_RIGHT, right);
    if (durationMs > 0) {
        delay(durationMs);
        analogWrite(VIBRO_LEFT, 0);
        analogWrite(VIBRO_RIGHT, 0);
    }
}

void Haptics::click() {
    buzz(255, 255, 50);  // Strong short click
}

void Haptics::doubleClick() {
    // Heartbeat pattern: lub-DUB ... lub-DUB
    // First beat (lub)
    buzz(200, 200, 60);
    delay(50);
    // Second beat (DUB - stronger)
    buzz(255, 255, 100);
    delay(300);
    // Repeat
    buzz(200, 200, 60);
    delay(50);
    buzz(255, 255, 100);
}

void Haptics::alarm() {
    // Rapid alternating pattern - urgent feel
    for (int i = 0; i < 3; i++) {
        buzz(255, 0, 80);   // Left motor
        buzz(0, 255, 80);   // Right motor
    }
    buzz(0, 0, 0);  // Ensure off at end
}

// ============================================================================
// Purr - Non-blocking cat-like rhythmic vibration
// ============================================================================

bool Haptics::purring = false;
uint32_t Haptics::purrPhaseMs = 0;
uint8_t Haptics::purrPhase = 0;

void Haptics::startPurr() {
    purring = true;
    purrPhaseMs = millis();
    purrPhase = 0;
    buzz(180, 180, 0);  // Start immediately
}

void Haptics::stopPurr() {
    purring = false;
    purrPhase = 0;
    buzz(0, 0, 0);  // Turn off
}

bool Haptics::isPurring() {
    return purring;
}

void Haptics::purrTick() {
    if (!purring) return;
    
    uint32_t now = millis();
    uint32_t elapsed = now - purrPhaseMs;
    
    // Purr pattern: rhythmic vibration like cat purring
    // Uses buzz() which we know works reliably
    
    switch (purrPhase) {
        case 0: // Medium rumble
            if (elapsed >= 150) {
                buzz(255, 255, 0);  // Pulse up
                purrPhase = 1;
                purrPhaseMs = now;
            }
            break;
            
        case 1: // Strong pulse
            if (elapsed >= 80) {
                buzz(180, 180, 0);  // Back down
                purrPhase = 2;
                purrPhaseMs = now;
            }
            break;
            
        case 2: // Medium
            if (elapsed >= 150) {
                buzz(100, 100, 0);  // Quiet moment
                purrPhase = 3;
                purrPhaseMs = now;
            }
            break;
            
        case 3: // Soft pause
            if (elapsed >= 120) {
                buzz(180, 180, 0);  // Start next cycle
                purrPhase = 0;
                purrPhaseMs = now;
            }
            break;
    }
}
