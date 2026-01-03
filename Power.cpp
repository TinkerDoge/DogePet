#include "include/Power.h"

// State variables
PowerState Power::state = PowerState::ACTIVE;
uint32_t Power::lastActivityMs = 0;
uint32_t Power::lastMotionMs = 0;
uint32_t Power::lastNoiseMs = 0;

// Callbacks (set by main sketch)
void (*Power::onSleepCallback)() = nullptr;
void (*Power::onWakeCallback)() = nullptr;
void (*Power::onDimCallback)() = nullptr;

void Power::init() {
    // ADC pin setup (if needed, usually analogRead handles it)
    pinMode(VBAT_PIN, INPUT);
    lastActivityMs = millis();
    lastMotionMs = millis();
    lastNoiseMs = millis();
    state = PowerState::ACTIVE;
    Serial.println("{\"status\":\"info\",\"msg\":\"Power Monitor Init\"}");
}

float Power::readADC() {
    // Supersampling
    long sum = 0;
    for(int i=0; i<VBAT_SAMPLES; i++) {
        sum += analogRead(VBAT_PIN);
        delay(2);
    }
    float raw = sum / (float)VBAT_SAMPLES;
    
    // ESP32 ADC is 12-bit (0-4095)
    // Ref voltage usually ~3.3V (but can vary)
    // Circuit: Divider (10k/10k) = V_bat / 2 -> Pin
    // V_bat = Pin_V * 2
    // Pin_V = (raw / 4095) * 3.3
    
    float voltage = (raw / 4095.0f) * 3.3f * 2.0f * VBAT_CAL;
    return voltage;
}

float Power::getVoltage() {
    return readADC();
}

int Power::getPercent() {
    float v = getVoltage();
    // Simple linear approx for LiPo 3.2v - 4.2v
    int pct = (int)((v - VBAT_MIN_V) / (VBAT_MAX_V - VBAT_MIN_V) * 100.0f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

void Power::logStatus() {
    Serial.printf("{\"status\":\"data\",\"type\":\"power\",\"v\":%.2f,\"pct\":%d,\"state\":%d}\n", 
                  getVoltage(), getPercent(), (int)state);
}

// ============================================================================
// Activity Tracking
// ============================================================================

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

// ============================================================================
// Sleep Mode Management
// ============================================================================

void Power::update() {
    uint32_t now = millis();
    uint32_t idleTime = now - lastActivityMs;
    
    switch (state) {
        case PowerState::ACTIVE:
            // Check if we should dim
            if (idleTime >= IDLE_TIMEOUT_MS) {
                state = PowerState::DIM;
                Serial.println("{\"status\":\"info\",\"msg\":\"Entering DIM mode\"}");
                if (onDimCallback) onDimCallback();
            }
            break;
            
        case PowerState::DIM:
            // Check if we should sleep
            if (idleTime >= SLEEP_TIMEOUT_MS) {
                sleep();
            }
            break;
            
        case PowerState::SLEEPING:
            // In sleep mode - update() still called but minimal work
            // Wake events handled by onActivity/onMotion/onLoudNoise
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
