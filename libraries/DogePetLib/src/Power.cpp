// Power.cpp - Battery monitoring and sleep management
#include "Power.h"

PowerState Power::state = PowerState::ACTIVE;
uint32_t Power::lastActivityMs = 0;
uint32_t Power::lastMotionMs = 0;
uint32_t Power::lastNoiseMs = 0;

// Battery monitoring
float Power::cachedVoltage = 0.0f;
int Power::cachedPercent = -1;
uint32_t Power::lastVbatReadMs = 0;
uint32_t Power::lastVbatLogMs = 0;
bool Power::batteryInitialized = false;

void (*Power::onSleepCallback)() = nullptr;
void (*Power::onWakeCallback)() = nullptr;
void (*Power::onDimCallback)() = nullptr;

void Power::init() {
    pinMode(VBAT_PIN, INPUT);
    analogSetAttenuation(ADC_11db);
    analogReadResolution(12);  // 12-bit ADC resolution
    
    lastActivityMs = millis();
    lastMotionMs = millis();
    lastNoiseMs = millis();
    lastVbatReadMs = 0;
    lastVbatLogMs = 0;
    cachedVoltage = 0.0f;
    cachedPercent = -1;
    batteryInitialized = false;
    state = PowerState::ACTIVE;
    
    // Read battery once at init
    updateBatteryReading();
    
    Serial.println("{\"status\":\"info\",\"msg\":\"Power Monitor Init\"}");
}

float Power::readADC() {
    long sum = 0;
    for(int i=0; i<VBAT_SAMPLES; i++) {
        sum += analogRead(VBAT_PIN);
        delay(2);
    }
    float raw = sum / (float)VBAT_SAMPLES;
    // Calculate voltage: ADC reading -> voltage with divider and calibration
    // Formula: V_battery = (ADC_raw / 4095) * 3.3V * divider_ratio * calibration
    float voltage = (raw / 4095.0f) * 3.3f * 2.0f * VBAT_CAL;
    return voltage;
}

void Power::updateBatteryReading() {
    uint32_t now = millis();
    
    // Use longer interval when sleeping to save power
    uint32_t readInterval = (state == PowerState::SLEEPING) ? 
                            VBAT_READ_INTERVAL_SLEEP_MS : 
                            VBAT_READ_INTERVAL_MS;
    
    // Only read battery at configured interval to avoid excessive ADC reads
    if (now - lastVbatReadMs >= readInterval) {
        cachedVoltage = readADC();
        cachedPercent = calculatePercent(cachedVoltage);
        lastVbatReadMs = now;
        batteryInitialized = true;
    }
}

int Power::calculatePercent(float voltage) {
    // Clamp voltage to valid range
    if (voltage < VBAT_MIN_V) voltage = VBAT_MIN_V;
    if (voltage > VBAT_MAX_V) voltage = VBAT_MAX_V;
    
    // Linear interpolation: percent = (V - V_min) / (V_max - V_min) * 100
    int pct = (int)((voltage - VBAT_MIN_V) / (VBAT_MAX_V - VBAT_MIN_V) * 100.0f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

float Power::getVoltage() {
    // Update reading if needed, then return cached value
    updateBatteryReading();
    return cachedVoltage;
}

int Power::getPercent() {
    // Update reading if needed, then return cached value
    updateBatteryReading();
    return cachedPercent;
}

void Power::logStatus() {
    // Ensure we have fresh readings
    updateBatteryReading();
    
    // Log battery status in JSON format for monitoring
    Serial.printf("{\"status\":\"data\",\"type\":\"power\",\"voltage\":%.2f,\"percent\":%d,\"state\":%d}\n", 
                  cachedVoltage, cachedPercent, (int)state);
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
    
    // Update battery reading periodically (uses longer interval when sleeping)
    updateBatteryReading();
    
    // Log battery status at configured interval (longer when sleeping)
    if (batteryInitialized) {
        uint32_t logInterval = (state == PowerState::SLEEPING) ? 
                               VBAT_LOG_INTERVAL_SLEEP_MS : 
                               VBAT_LOG_INTERVAL_MS;
        if (now - lastVbatLogMs >= logInterval) {
            logStatus();
            lastVbatLogMs = now;
        }
    }
    
    // Power state management
    switch (state) {
        case PowerState::ACTIVE:
            if (idleTime >= IDLE_TIMEOUT_MS) {
                state = PowerState::DIM;
                Serial.println("{\"status\":\"info\",\"msg\":\"Entering DIM mode\"}");
                if (onDimCallback) onDimCallback();
            }
            break;
            
        case PowerState::DIM:
            if (idleTime >= SLEEP_TIMEOUT_MS) {
                sleep();
            }
            break;
            
        case PowerState::SLEEPING:
            // In sleep mode, minimal processing - just check for wake conditions
            // Battery reading and logging already use longer intervals above
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
        Serial.println("{\"status\":\"info\",\"msg\":\"Entering SLEEP mode - power saving active\"}");
        if (onSleepCallback) onSleepCallback();
    }
}
