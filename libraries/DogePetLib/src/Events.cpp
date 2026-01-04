// Events.cpp - Central event handler for touch → behavior reactions
#include "Events.h"
#include "Face.h"
#include "Touch.h"
#include "Haptics.h"
#include "Audio.h"
#include "Power.h"
#include "config.h"

static bool isPetting = false;
static bool isChinScratching = false;
static bool comboLoveTriggered = false;
static uint32_t confusedStartMs = 0;
static constexpr uint32_t CONFUSED_MAX_MS = 2000;

void Events::init() {
    isPetting = false;
    isChinScratching = false;
    comboLoveTriggered = false;
    confusedStartMs = 0;
}

void Events::update() {
    roboEyes* eyes = Face::getEyes();
    if (!eyes) return;

    // Safeguard: Clear stuck animations
    if (eyes->confused && confusedStartMs > 0) {
        if (millis() - confusedStartMs > CONFUSED_MAX_MS) {
            eyes->confused = false;
            eyes->setHFlicker(0, 0);
            confusedStartMs = 0;
        }
    }
    
    // ========== HEAD TOUCH ==========
    TouchEvent headEvent = Touch::getHeadEvent();
    
    switch (headEvent) {
        case TouchEvent::TAP:
            eyes->blink();
            Haptics::click();
            Audio::tapSound();
            Power::onActivity();
            Serial.println("{\"status\":\"event\",\"type\":\"tap\",\"sensor\":\"head\"}");
            break;
            
        case TouchEvent::HOLD_START:
            isPetting = true;
            eyes->happy = true;
            eyes->tired = false;
            Haptics::startPurr();
            Audio::happySound();
            Power::onActivity();
            Serial.println("{\"status\":\"event\",\"type\":\"petting_start\"}");
            break;
            
        case TouchEvent::HOLDING:
            if (Touch::getHeadHoldDuration() % 2000 < 50) {
                eyes->blink();
            }
            break;
            
        case TouchEvent::HOLD_END:
            isPetting = false;
            eyes->happy = false;
            Haptics::stopPurr();
            Audio::contentSound();
            Serial.println("{\"status\":\"event\",\"type\":\"petting_end\"}");
            break;
            
        default:
            break;
    }
    
    // ========== CHIN SENSOR (OPTIONAL) ==========
    #ifdef TOUCH_CHIN_ENABLED
    TouchEvent chinEvent = Touch::getChinEvent();
    
    // Combo: Both head AND chin
    if (isPetting && (chinEvent == TouchEvent::HOLD_START || chinEvent == TouchEvent::HOLDING)) {
        if (!comboLoveTriggered) {
            eyes->confused = true;
            confusedStartMs = millis();
            eyes->happy = true;
            Haptics::doubleClick();
            Audio::surpriseBeep();
            Serial.println("{\"status\":\"event\",\"type\":\"combo_love\"}");
            comboLoveTriggered = true;
        }
        Power::onActivity();
    } else {
        if (comboLoveTriggered) {
            eyes->confused = false;
            eyes->setHFlicker(0, 0);
            confusedStartMs = 0;
        }
        comboLoveTriggered = false;
    }
    
    switch (chinEvent) {
        case TouchEvent::TAP:
            eyes->blink(true, false);
            Haptics::click();
            Audio::chirp();
            Power::onActivity();
            Serial.println("{\"status\":\"event\",\"type\":\"tap\",\"sensor\":\"chin\"}");
            break;
            
        case TouchEvent::HOLD_START:
            if (!isPetting) {
                isChinScratching = true;
                eyes->close();
                eyes->setVFlicker(1, 2);
                Haptics::startPurr();
                Audio::purrrSound();
                Serial.println("{\"status\":\"event\",\"type\":\"chin_scratch_start\"}");
            }
            Power::onActivity();
            break;
            
        case TouchEvent::HOLDING:
            if (isChinScratching && !isPetting) {
                uint32_t holdTime = Touch::getChinHoldDuration();
                if (holdTime % 3000 < 50) {
                    eyes->open();
                    eyes->setVFlicker(0, 0);
                } else if (holdTime % 3000 > 200 && holdTime % 3000 < 250) {
                    eyes->close();
                    eyes->setVFlicker(1, 2);
                }
            }
            break;
            
        case TouchEvent::HOLD_END:
            if (isChinScratching) {
                isChinScratching = false;
                eyes->open();
                eyes->setVFlicker(0, 0);
                eyes->confused = false;
                if (!isPetting) {
                    Haptics::stopPurr();
                    Audio::satisfiedSound();
                }
                Serial.println("{\"status\":\"event\",\"type\":\"chin_scratch_end\"}");
            }
            break;
            
        default:
            break;
    }
    #endif
}
