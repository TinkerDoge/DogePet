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
static bool comboConfusedTriggered = false;
static uint32_t confusedStartMs = 0;
static constexpr uint32_t CONFUSED_MAX_MS = 2000;

void Events::init() {
    isPetting = false;
    isChinScratching = false;
    comboConfusedTriggered = false;
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
    
    // Combo: Both head AND chin - Confused expression
    if (isPetting && (chinEvent == TouchEvent::HOLD_START || chinEvent == TouchEvent::HOLDING)) {
        if (!comboConfusedTriggered) {
            // Show confused face
            eyes->confused = true;
            eyes->setHFlicker(true, 3);
            Haptics::doubleClick();
            Audio::surpriseBeep();
            Power::forceWake();  // Force wake from any state
            Serial.println("{\"status\":\"event\",\"type\":\"combo_confused\"}");
            comboConfusedTriggered = true;
        }
    } else {
        if (comboConfusedTriggered) {
            eyes->confused = false;
            eyes->setHFlicker(false);
        }
        comboConfusedTriggered = false;
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
// ============================================================================
// MOTION EVENT HANDLER
// ============================================================================
void Events::onMotionEvent(void* eventPtr) {
    // Motion events trigger behavioral reactions similar to touch
    // Event codes: 0=None, 1=Tilt, 2=Shake, 3=FuriousShake, 4=Tap, 5=Still
    int event = (int)(intptr_t)eventPtr;
    
    roboEyes* eyes = Face::getEyes();
    if (!eyes) return;
    
    switch (event) {
        case 1:  // Tilt - curious/interested reaction
            eyes->curious = true;
            Haptics::click();
            Audio::tapSound();
            Serial.println("{\"status\":\"event\",\"type\":\"motion_tilt\",\"reaction\":\"curious\"}");
            break;
            
        case 2:  // Shake - playful surprise
            eyes->happy = true;
            eyes->blink();
            Haptics::doubleClick();
            Audio::surpriseBeep();
            Serial.println("{\"status\":\"event\",\"type\":\"motion_shake\",\"reaction\":\"playful\"}");
            break;
            
        case 3:  // FuriousShake - startled/jiggling reaction with confused animation
            eyes->angry = true;
            eyes->sweat = true;
            eyes->confused = true;  // Add jiggling confused animation
            Haptics::alarm();
            Audio::yawn();  // Surprised yawn-like sound
            Power::forceWake();  // Force wake from any state
            Serial.println("{\"status\":\"event\",\"type\":\"motion_furious_shake\",\"reaction\":\"startled\"}");
            break;
            
        case 4:  // Motion Tap - quick attention
            eyes->blink();
            Haptics::click();
            Audio::chirp();
            Serial.println("{\"status\":\"event\",\"type\":\"motion_tap\",\"reaction\":\"attention\"}");
            break;
            
        case 5:  // Still - relax, return to normal
            eyes->angry = false;
            eyes->curious = false;
            eyes->sweat = false;
            Serial.println("{\"status\":\"event\",\"type\":\"motion_still\",\"reaction\":\"relax\"}");
            break;
            
        default:
            break;
    }
}