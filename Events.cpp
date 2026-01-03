#include "include/Events.h"
#include "include/Face.h"
#include "include/Touch.h"
#include "include/Haptics.h"
#include "include/Audio.h"
#include "include/Power.h"
#include "include/config.h"

// State variables (moved from DogePet.ino)
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

    // ========== SAFEGUARD: Clear stuck animations ==========
    // If confused has been on too long, force clear it
    if (eyes->confused && confusedStartMs > 0) {
        if (millis() - confusedStartMs > CONFUSED_MAX_MS) {
            eyes->confused = false;
            eyes->setHFlicker(0, 0);  // Force stop horizontal shaking
            confusedStartMs = 0;
            Serial.println("{\"status\":\"debug\",\"msg\":\"Confused timeout safeguard triggered\"}");
        }
    }
    
    // ========== HEAD TOUCH (FUNC_BTN - Petting area) ==========
    TouchEvent headEvent = Touch::getHeadEvent();
    
    switch (headEvent) {
        case TouchEvent::TAP:
            // Single tap on head = blink
            eyes->blink();
            Power::onActivity();
            Serial.println("{\"status\":\"event\",\"type\":\"tap\",\"sensor\":\"head\"}");
            break;
            
        case TouchEvent::HOLD_START:
            // Start petting - begin happy mode
            isPetting = true;
            eyes->happy = true;
            eyes->tired = false;
            Haptics::startPurr();
            Power::onActivity();
            Serial.println("{\"status\":\"event\",\"type\":\"petting_start\"}");
            break;
            
        case TouchEvent::HOLDING:
            // Continue petting - maintain happy state
            // Occasional blink while being pet (every ~2 seconds)
            if (Touch::getHeadHoldDuration() % 2000 < 50) {
                eyes->blink();
            }
            break;
            
        case TouchEvent::HOLD_END:
            // Stop petting - return to normal
            isPetting = false;
            eyes->happy = false;
            Haptics::stopPurr();
            Serial.println("{\"status\":\"event\",\"type\":\"petting_end\"}");
            break;
            
        default:
            break;
    }
    
    // ========== CHIN SENSOR (Optional - only if TOUCH_CHIN_ENABLED) ==========
    #ifdef TOUCH_CHIN_ENABLED
    TouchEvent chinEvent = Touch::getChinEvent();
    
    // Special combo: Both head AND chin touched = overwhelming love!
    if (isPetting && (chinEvent == TouchEvent::HOLD_START || chinEvent == TouchEvent::HOLDING)) {
        if (!comboLoveTriggered) {
            // Overwhelmed with affection! Confused happy reaction
            eyes->confused = true;
            confusedStartMs = millis();  // Track when confusion started
            eyes->happy = true;
            Haptics::doubleClick();
            Audio::surpriseBeep();
            Serial.println("{\"status\":\"event\",\"type\":\"combo_love\"}");
            comboLoveTriggered = true;
        }
        Power::onActivity();
    } else {
        if (comboLoveTriggered) {
            // Exiting combo - clean up
            eyes->confused = false;
            eyes->setHFlicker(0, 0);  // Stop shaking
            confusedStartMs = 0;
        }
        comboLoveTriggered = false;  // Reset when not in combo
    }
    
    switch (chinEvent) {
        case TouchEvent::TAP:
            // Tap on chin = playful chirp + wink
            eyes->blink(true, false);  // Wink left eye
            Haptics::click();
            Audio::chirp();
            Power::onActivity();
            Serial.println("{\"status\":\"event\",\"type\":\"tap\",\"sensor\":\"chin\"}");
            break;
            
        case TouchEvent::HOLD_START:
            // Start chin scratches - blissful closed eyes with gentle tremble!
            if (!isPetting) {
                isChinScratching = true;
                eyes->close();  // Blissfully close eyes
                eyes->setVFlicker(1, 2);  // Gentle vertical trembling (amplitude 2)
                Haptics::startPurr();
                Audio::purrrSound();
                Serial.println("{\"status\":\"event\",\"type\":\"chin_scratch_start\"}");
            }
            Power::onActivity();
            break;
            
        case TouchEvent::HOLDING:
            // Continue chin scratches - eyes stay closed with gentle shake
            if (isChinScratching && !isPetting) {
                // Every ~3 seconds, briefly open eyes to "peek" then close again
                uint32_t holdTime = Touch::getChinHoldDuration();
                if (holdTime % 3000 < 50) {
                    eyes->open();  // Quick peek
                    eyes->setVFlicker(0, 0);  // Stop shake while peeking
                } else if (holdTime % 3000 > 200 && holdTime % 3000 < 250) {
                    eyes->close();  // Back to bliss
                    eyes->setVFlicker(1, 2);  // Resume gentle shake
                }
            }
            break;
            
        case TouchEvent::HOLD_END:
            // Stop chin scratches - open eyes and stop shaking
            if (isChinScratching) {
                isChinScratching = false;
                eyes->open();
                eyes->setVFlicker(0, 0);  // Stop trembling
                eyes->confused = false;
                if (!isPetting) {
                    Haptics::stopPurr();
                }
                Serial.println("{\"status\":\"event\",\"type\":\"chin_scratch_end\"}");
            }
            break;
            
        default:
            break;
    }
    #endif
}
