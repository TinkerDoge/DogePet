// Haptics.cpp - Vibration motor control
#include "Haptics.h"

// Track if LEDC is set up
static bool ledcSetup = false;

bool Haptics::purring = false;
uint32_t Haptics::purrPhaseMs = 0;
uint8_t Haptics::purrPhase = 0;

void Haptics::init() {
    analogWrite(VIBRO_LEFT, 0);
    analogWrite(VIBRO_RIGHT, 0);
    ledcSetup = true;
    Serial.println("{\"status\":\"info\",\"msg\":\"Haptics Init (LEDC mode)\"}");
}

void Haptics::stop() {
    analogWrite(VIBRO_LEFT, 0);
    analogWrite(VIBRO_RIGHT, 0);
}

void Haptics::setMotor(int pin, int pwm) {
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
    buzz(255, 255, 50);
}

void Haptics::doubleClick() {
    buzz(200, 200, 60); delay(50);
    buzz(255, 255, 100); delay(300);
    buzz(200, 200, 60); delay(50);
    buzz(255, 255, 100);
}

void Haptics::alarm() {
    for (int i = 0; i < 3; i++) {
        buzz(255, 0, 80);
        buzz(0, 255, 80);
    }
    buzz(0, 0, 0);
}

void Haptics::startPurr() {
    purring = true;
    purrPhaseMs = millis();
    purrPhase = 0;
    buzz(180, 180, 0);
}

void Haptics::stopPurr() {
    purring = false;
    purrPhase = 0;
    buzz(0, 0, 0);
}

bool Haptics::isPurring() {
    return purring;
}

void Haptics::purrTick() {
    if (!purring) return;
    
    uint32_t now = millis();
    uint32_t elapsed = now - purrPhaseMs;
    
    switch (purrPhase) {
        case 0:
            if (elapsed >= 150) {
                buzz(255, 255, 0);
                purrPhase = 1;
                purrPhaseMs = now;
            }
            break;
            
        case 1:
            if (elapsed >= 80) {
                buzz(180, 180, 0);
                purrPhase = 2;
                purrPhaseMs = now;
            }
            break;
            
        case 2:
            if (elapsed >= 150) {
                buzz(100, 100, 0);
                purrPhase = 3;
                purrPhaseMs = now;
            }
            break;
            
        case 3:
            if (elapsed >= 120) {
                buzz(180, 180, 0);
                purrPhase = 0;
                purrPhaseMs = now;
            }
            break;
    }
}
