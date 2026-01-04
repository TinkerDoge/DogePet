// Events.cpp - Central event handler for touch → behavior reactions
#include "Events.h"
#include "Face.h"
#include "Touch.h"
#include "Haptics.h"
#include "Audio.h"
#include "Power.h"
#include "Motion.h"
#include "config.h"

static bool isPetting = false;
static bool isChinScratching = false;
static bool comboConfusedTriggered = false;
static bool furiousShakeTriggered = false;
static bool shakeTriggered = false;
static bool tiltTriggered = false;
static bool stillTriggered = false;  // Debounce Still events
static uint32_t confusedStartMs = 0;
static uint32_t tiltLastEventMs = 0;
static uint32_t shakeStartMs = 0;  // Track shake animation start
static uint32_t stillLastEventMs = 0;  // Debounce Still
static constexpr uint32_t CONFUSED_MAX_MS = 2000;
static constexpr uint32_t FURIOUS_SHAKE_RESET_MS = 500;  // Reset confused animation after 500ms
static constexpr uint32_t SHAKE_RESET_MS = 800;  // Reset happy animation after 800ms
static constexpr uint32_t TILT_DEBOUNCE_MS = 2000;  // Tilt events debounced to prevent spam
static constexpr uint32_t STILL_DEBOUNCE_MS = 500;  // Still events debounced to prevent spam

// Priority-based event handling: Higher priority events block lower ones
enum EventPriority {
    PRIORITY_FURIOUS_SHAKE = 3,  // Highest - forces wake, most dramatic
    PRIORITY_COMBO = 2,           // High - combo touch, forces wake
    PRIORITY_SHAKE_TAP = 1,       // Medium - regular shake/tap interactions
    PRIORITY_TILT = 0             // Lowest - subtle tilt, easily triggered
};

// Track current active priority (prevents overlapping behaviors)
static EventPriority activeEventPriority = (EventPriority)-1;

// ============================================================================
// HELPER: Clear all animation state from lower-priority events
// Call this when a higher-priority event needs to preempt lower-priority ones
// ============================================================================
static void clearLowPriorityAnimation(roboEyes* eyes, EventPriority minPriorityToClear) {
    if (!eyes) return;
    
    // Only clear if we're overriding a lower or equal priority event
    if (activeEventPriority <= minPriorityToClear) {
        // Clear ALL animation flags and flicker states
        eyes->happy = false;
        eyes->curious = false;
        eyes->sweat = false;
        eyes->angry = false;
        eyes->tired = false;
        
        // CRITICAL: Clear ALL flicker settings (this was the missing piece!)
        eyes->setVFlicker(0, 0);  // Stop vertical flicker from shake animations
        eyes->setHFlicker(0, 0);  // Stop horizontal flicker from confused animations
        
        // Reset all triggered flags for lower-priority events
        shakeTriggered = false;
        tiltTriggered = false;
        shakeStartMs = 0;
    }
}

// Touch state tracking for reduced logging (only log on state change)
static bool lastHeadPressedState = false;
static bool lastChinPressedState = false;

void Events::init() {
    isPetting = false;
    isChinScratching = false;
    comboConfusedTriggered = false;
    furiousShakeTriggered = false;
    shakeTriggered = false;
    tiltTriggered = false;
    stillTriggered = false;
    confusedStartMs = 0;
    tiltLastEventMs = 0;
    shakeStartMs = 0;
    stillLastEventMs = 0;
    activeEventPriority = (EventPriority)-1;
    lastHeadPressedState = false;
    lastChinPressedState = false;
}

void Events::update() {
    roboEyes* eyes = Face::getEyes();
    if (!eyes) return;

    // Skip all touch reactions if sleeping (except combo/furious shake for wake)
    if (Power::isSleeping()) {
        return;
    }

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
    bool headPressed = (headEvent != TouchEvent::NONE);
    
    // Log head touch state change
    if (headPressed != lastHeadPressedState) {
        lastHeadPressedState = headPressed;
        if (headPressed) {
            Serial.println("{\"status\":\"data\",\"type\":\"touch\",\"sensor\":\"head\",\"state\":\"pressed\"}");
        } else {
            Serial.println("{\"status\":\"data\",\"type\":\"touch\",\"sensor\":\"head\",\"state\":\"released\"}");
        }
    }
    
    switch (headEvent) {
        case TouchEvent::TAP:
            eyes->blink();
            Haptics::click();
            Audio::tapSound(); // Keep simple blip for tap
            Power::onActivity();
            Serial.println("{\"status\":\"event\",\"type\":\"tap\",\"sensor\":\"head\"}");
            break;
            
        case TouchEvent::HOLD_START:
            isPetting = true;
            eyes->happy = true;
            eyes->tired = false;
            Haptics::startPurr();
            Audio::speak(Audio::MOOD_HAPPY); // "Happy droid chatter"
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
            Audio::contentSound(); // Keep distinct "end" sound
            Serial.println("{\"status\":\"event\",\"type\":\"petting_end\"}");
            break;
            
        default:
            break;
    }
    
    // ========== CHIN SENSOR (OPTIONAL) ==========
    #ifdef TOUCH_CHIN_ENABLED
    TouchEvent chinEvent = Touch::getChinEvent();
    bool chinPressed = (chinEvent != TouchEvent::NONE);
    
    // Log chin touch state change
    if (chinPressed != lastChinPressedState) {
        lastChinPressedState = chinPressed;
        if (chinPressed) {
            Serial.println("{\"status\":\"data\",\"type\":\"touch\",\"sensor\":\"chin\",\"state\":\"pressed\"}");
        } else {
            Serial.println("{\"status\":\"data\",\"type\":\"touch\",\"sensor\":\"chin\",\"state\":\"released\"}");
        }
    }
    
    // Combo: Both head AND chin - Confused expression
    if (isPetting && (chinEvent == TouchEvent::HOLD_START || chinEvent == TouchEvent::HOLDING)) {
        if (!comboConfusedTriggered) {
            // High priority - clear lower priority animations
            clearLowPriorityAnimation(eyes, (EventPriority)(PRIORITY_COMBO - 1));
            
            // Show confused face
            eyes->confused = true;
            eyes->setHFlicker(true, 3);
            Haptics::doubleClick();
            Audio::speak(Audio::MOOD_CURIOUS); // "Confused whistle?"
            // Check if sleeping - if so, force wake; if awake, just mark activity
            if (Power::isSleeping()) {
                Power::forceWake();  // Force wake from sleep
            } else {
                Power::onActivity();  // Just register activity if already awake
            }
            Serial.println("{\"status\":\"event\",\"type\":\"combo_confused\"}");
            comboConfusedTriggered = true;
            confusedStartMs = millis();  // Track when confused animation started
            activeEventPriority = PRIORITY_COMBO;
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
                // Smoother eye blinking cycle: longer duration for better animation with new 60Hz framerate
                // Open eyes: holds for 100ms every 4 seconds
                if (holdTime % 4000 < 100) {
                    eyes->open();
                    eyes->setVFlicker(0, 0);
                // Close eyes: happens at middle of cycle (smooth timing)
                } else if (holdTime % 4000 > 400 && holdTime % 4000 < 500) {
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
    uint32_t now = millis();
    
    roboEyes* eyes = Face::getEyes();
    if (!eyes) return;

    // Auto-reset shake animation after timeout
    if (shakeTriggered && shakeStartMs > 0) {
        if (now - shakeStartMs > SHAKE_RESET_MS) {
            eyes->happy = false;
            eyes->setVFlicker(0, 0);  // CRITICAL: Clear flicker when animation ends!
            eyes->blink();  // One final blink before reset
            shakeTriggered = false;
            shakeStartMs = 0;
            if (activeEventPriority == PRIORITY_SHAKE_TAP) {
                activeEventPriority = (EventPriority)-1;  // Release priority only if still active
            }
        }
    }

    // Reset furious shake animation after timeout
    if (furiousShakeTriggered && confusedStartMs > 0) {
        if (now - confusedStartMs > FURIOUS_SHAKE_RESET_MS) {
            eyes->angry = false;
            eyes->sweat = false;
            eyes->confused = false;
            eyes->setHFlicker(0, 0);  // CRITICAL: Clear flicker when animation ends!
            eyes->setVFlicker(0, 0);  // Also clear vertical flicker if set
            furiousShakeTriggered = false;
            shakeTriggered = false;  // Clear shake flag too when animation ends
            confusedStartMs = 0;
            activeEventPriority = (EventPriority)-1;  // Release event priority
        }
    }

    // Skip motion reactions if sleeping (except furious shake for wake)
    if (Power::isSleeping() && event != 3) {  // 3 = FuriousShake
        return;
    }
    
    switch (event) {
        case 1:  // Tilt - curious/interested reaction (LOWEST PRIORITY)
            // Debounce tilt events to prevent spam (only trigger once per TILT_DEBOUNCE_MS)
            if (!tiltTriggered && (now - tiltLastEventMs >= TILT_DEBOUNCE_MS)) {
                // Only trigger if no higher priority event is active
                if (activeEventPriority < PRIORITY_TILT) {
                    eyes->curious = true;
                    Haptics::click();
                    Audio::speak(Audio::MOOD_CURIOUS); // "Curious whistle"
                    Power::onMotion();
                    // Include all 6 axes in tilt event (accel X,Y,Z + gyro X,Y,Z)
                    Serial.printf("{\"status\":\"event\",\"type\":\"motion_tilt\",\"reaction\":\"curious\",\"accel\":{\"x\":%.3f,\"y\":%.3f,\"z\":%.3f},\"gyro\":{\"x\":%.1f,\"y\":%.1f,\"z\":%.1f}}\n",
                        Motion::getLastAccelX(), Motion::getLastAccelY(), Motion::getLastAccelZ(),
                        Motion::getLastGyroX(), Motion::getLastGyroY(), Motion::getLastGyroZ());
                    tiltTriggered = true;
                    tiltLastEventMs = now;
                    activeEventPriority = PRIORITY_TILT;
                }
            }
            break;
            
        case 2:  // Shake - playful surprise (MEDIUM PRIORITY)
            // Only trigger if no higher priority event is active
            if (!shakeTriggered && activeEventPriority < PRIORITY_SHAKE_TAP) {
                // Medium priority - clear lower priority (tilt) animations
                clearLowPriorityAnimation(eyes, (EventPriority)(PRIORITY_SHAKE_TAP - 1));
                
                eyes->happy = true;
                eyes->blink();
                Haptics::doubleClick();
                Audio::speak(Audio::MOOD_HAPPY); // "Happy chatter"
                Power::onMotion();
                Serial.println("{\"status\":\"event\",\"type\":\"motion_shake\",\"reaction\":\"playful\"}");
                shakeTriggered = true;
                shakeStartMs = now;  // Track when shake animation started
                activeEventPriority = PRIORITY_SHAKE_TAP;
            }
            break;
            
        case 3:  // FuriousShake - startled/jiggling reaction (HIGHEST PRIORITY)
            // Always trigger, overrides everything else
            if (!furiousShakeTriggered) {
                // Highest priority - clear ALL lower priority animations
                clearLowPriorityAnimation(eyes, (EventPriority)(PRIORITY_FURIOUS_SHAKE - 1));
                
                eyes->angry = true;
                eyes->sweat = true;
                eyes->confused = true;  // Add jiggling confused animation
                Haptics::alarm();
                Audio::speak(Audio::MOOD_ANGRY);  // "Angry/Startled noise"
                // Check if sleeping - if so, force wake; if awake, just mark activity
                if (Power::isSleeping()) {
                    Power::forceWake();  // Force wake from sleep
                } else {
                    Power::onMotion();  // Just register motion if already awake
                }
                Serial.println("{\"status\":\"event\",\"type\":\"motion_furious_shake\",\"reaction\":\"startled\"}");
                furiousShakeTriggered = true;
                shakeTriggered = false;  // Cancel any pending shake
                tiltTriggered = false;   // Cancel any pending tilt
                confusedStartMs = now;  // Track when animation started
                activeEventPriority = PRIORITY_FURIOUS_SHAKE;  // Highest priority
            }
            break;
            
        case 4:  // Motion Tap - quick attention (MEDIUM PRIORITY)
            // Only trigger if no higher priority event is active
            if (activeEventPriority < PRIORITY_SHAKE_TAP) {
                eyes->blink();
                Haptics::click();
                Audio::tapSound(); // Simple tap
                Power::onMotion();
                Serial.println("{\"status\":\"event\",\"type\":\"motion_tap\",\"reaction\":\"attention\"}");
                activeEventPriority = PRIORITY_SHAKE_TAP;
            }
            break;
            
        case 5:  // Still - relax, return to normal (ALWAYS RESETS)
            // Debounce still events to prevent spam
            if (!stillTriggered && (now - stillLastEventMs >= STILL_DEBOUNCE_MS)) {
                // Clear ALL animation flags and flicker (complete reset)
                eyes->angry = false;
                eyes->happy = false;  // IMPORTANT: Reset happy flag from shake
                eyes->curious = false;
                eyes->sweat = false;
                eyes->confused = false;
                eyes->tired = false;
                
                // Clear ALL flicker settings
                eyes->setVFlicker(0, 0);
                eyes->setHFlicker(0, 0);
                
                // Reset ALL triggered flags
                furiousShakeTriggered = false;
                shakeTriggered = false;
                tiltTriggered = false;
                confusedStartMs = 0;
                shakeStartMs = 0;
                activeEventPriority = (EventPriority)-1;  // Release priority
                stillTriggered = true;
                stillLastEventMs = now;
                Serial.println("{\"status\":\"event\",\"type\":\"motion_still\",\"reaction\":\"relax\"}");
            }
            break;
            
        default:
            break;
    }
}