// Power.cpp - Battery monitoring and sleep management
#include "Power.h"
#include "ConfigManager.h"

PowerState Power::state = PowerState::ACTIVE;
uint32_t Power::lastActivityMs = 0;
uint32_t Power::lastMotionMs = 0;
uint32_t Power::lastNoiseMs = 0;

void (*Power::onSleepCallback)() = nullptr;
void (*Power::onWakeCallback)() = nullptr;
void (*Power::onDimCallback)() = nullptr;

void Power::init() {
    pinMode(VBAT_PIN, INPUT);
    analogSetAttenuation(ADC_11db);
    
    lastActivityMs = millis();
    lastMotionMs = millis();
    lastNoiseMs = millis();
    state = PowerState::ACTIVE;
    Serial.println("{\"status\":\"info\",\"msg\":\"Power Monitor Init\"}");
}

float Power::readADC() {
    long sum = 0;
    for(int i=0; i<VBAT_SAMPLES; i++) {
        sum += analogRead(VBAT_PIN);
        delay(2);
    }
    float raw = sum / (float)VBAT_SAMPLES;
    float voltage = (raw / 4095.0f) * 3.3f * 2.0f * VBAT_CAL;
    return voltage;
}

float Power::getVoltage() {
    return readADC();
}

int Power::getPercent() {
    float v = getVoltage();
    int pct = (int)((v - VBAT_MIN_V) / (VBAT_MAX_V - VBAT_MIN_V) * 100.0f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

void Power::logStatus() {
    Serial.printf("{\"status\":\"data\",\"type\":\"power\",\"v\":%.2f,\"pct\":%d,\"state\":%d}\n", 
                  getVoltage(), getPercent(), (int)state);
}

void Power::onActivity() {
    lastActivityMs = millis();
    if (state != PowerState::ACTIVE) {
        wake();
    }
}

void Power::onMotion() {
    lastMotionMs = millis();
    onActivity();
}

void Power::onLoudNoise() {
    lastNoiseMs = millis();
    onActivity();
}

void Power::update() {
    uint32_t now = millis();
    uint32_t idleTime = now - lastActivityMs;
    
    switch (state) {
        case PowerState::ACTIVE:
            if (idleTime >= settings.power.idle_ms) {
                state = PowerState::DIM;
                Serial.println("{\"status\":\"info\",\"msg\":\"Entering DIM mode\"}");
                if (onDimCallback) onDimCallback();
            }
            break;
            
        case PowerState::DIM:
            if (idleTime >= settings.power.sleep_ms) {
                sleep();
            }
            break;
            
        case PowerState::SLEEPING:
            break;
    }
}

PowerState Power::getState() {
    return state;
}

bool Power::isSleeping() {
    return state == PowerState::SLEEPING;
}

void Power::wake() {
    if (state == PowerState::SLEEPING || state == PowerState::DIM) {
        state = PowerState::ACTIVE;
        lastActivityMs = millis();
        Serial.println("{\"status\":\"info\",\"msg\":\"Waking up!\"}");
        if (onWakeCallback) onWakeCallback();
    }
}

void Power::sleep() {
    if (state != PowerState::SLEEPING) {
        state = PowerState::SLEEPING;
        Serial.println("{\"status\":\"info\",\"msg\":\"Entering SLEEP mode\"}");
        if (onSleepCallback) onSleepCallback();
    }
}
