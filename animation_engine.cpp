// Animation Engine Module - Implementation
// Handles eye animations and sound effects for the robot
#include "animation_engine.h"
#include "audio.h"
#include "motion.h"

// Ensure stdint types are available
#ifndef uint32_t
typedef unsigned long uint32_t;
#endif

// Ensure delay function is available
#ifndef delay
#define delay(x) ::delay(x)
#endif

// Forward declarations - we'll access roboEyes through external functions
// to avoid including the header that requires global display object

// External function wrappers (defined in main DogePet.ino)
extern void Eyes_Blink();
extern void Eyes_SetPosition(unsigned char position);
extern void Eyes_SetMood(unsigned char mood);
extern void Eyes_Open();
extern void Eyes_SetEyeSize(int leftWidth, int rightWidth, int leftHeight, int rightHeight);

// RoboEyes position constants (from FluxGarage_RoboEyes.h)
#define N  1
#define NE 2
#define E  3
#define SE 4
#define S  5
#define SW 6
#define W  7
#define NW 8

namespace AnimationEngine {
    
    // Execute eye animation commands
    void executeEyeAnimation(String animation) {
        animation.toLowerCase();
        animation.trim();

        Serial.printf("AI Animation: %s\n", animation.c_str());

        if (animation == "blink") {
            Eyes_Blink();
            if (ENABLE_TAP_SFX && !silentMode) {
                Audio::sfxBlink();
            }
        }
        else if (animation == "look_left") {
            Eyes_SetPosition(W);
            delay(500);
            Eyes_SetPosition(0); // Center position (0 triggers default case)
        }
        else if (animation == "look_right") {
            Eyes_SetPosition(E);
            delay(500);
            Eyes_SetPosition(0); // Center position (0 triggers default case)
        }
        else if (animation == "look_up") {
            Eyes_SetPosition(N);
            delay(500);
            Eyes_SetPosition(0); // Center position (0 triggers default case)
        }
        else if (animation == "look_down") {
            Eyes_SetPosition(S);
            delay(500);
            Eyes_SetPosition(0); // Center position (0 triggers default case)
        }
        else if (animation == "widen") {
            // Note: Eye size adjustment would need external wrapper functions
            // For now, we'll use a simpler approach with mood changes
            Serial.println("AI Animation: widen (using mood change)");
            Eyes_SetMood(3); // Happy mood makes eyes appear wider
            delay(800);
            Eyes_SetMood(0); // Return to default
        }
        else if (animation == "narrow") {
            // Note: Eye size adjustment would need external wrapper functions
            // For now, we'll use a simpler approach with mood changes
            Serial.println("AI Animation: narrow (using mood change)");
            Eyes_SetMood(1); // Tired mood makes eyes appear narrower
            delay(800);
            Eyes_SetMood(0); // Return to default
        }
        else if (animation == "happy") {
            setEyesMood(MS_HAPPY);
            moodUntil = millis() + 3000;
        }
        else if (animation == "sad" || animation == "tired") {
            setEyesMood(MS_TIRED);
            moodUntil = millis() + 3000;
        }
        else if (animation == "angry") {
            setEyesMood(MS_ANGRY);
            moodUntil = millis() + 2000;
        }
        else if (animation == "curious" || animation == "question") {
            Eyes_SetPosition(NE);
            delay(300);
            Eyes_SetPosition(NW);
            delay(300);
            Eyes_SetPosition(0); // Center position (0 triggers default case)
            if (!silentMode) {
                Audio::playCuteQuestion(100);
            }
        }
        else if (animation == "surprise") {
            // Use mood change for surprise effect
            Serial.println("AI Animation: surprise (using mood change)");
            Eyes_SetMood(3); // Happy mood for wide eyes
            delay(600);
            Eyes_SetMood(0); // Return to default
        }
        else if (animation == "thinking") {
            // Slow blink pattern
            Eyes_Blink();
            delay(200);
            Eyes_Blink();
            delay(200);
        }
        else if (animation == "sleepy") {
            Eyes_SetMood(1); // Tired mood
            delay(1000);
            Eyes_SetMood(0); // Reset to default
        }
    }

    // Execute sound effect commands
    void executeSoundFX(String soundFx) {
        soundFx.toLowerCase();
        soundFx.trim();

        Serial.printf("AI Sound FX: %s\n", soundFx.c_str());

        if (soundFx == "curious" || soundFx == "question") {
            Audio::playCuteQuestion(150);
        }
        else if (soundFx == "happy" || soundFx == "joy") {
            Audio::playCuteHello(150);
        }
        else if (soundFx == "sad" || soundFx == "tired") {
            Audio::playCuteSleep(200);
        }
        else if (soundFx == "angry" || soundFx == "grumpy") {
            Audio::playCuteNo(150);
        }
        else if (soundFx == "surprise" || soundFx == "shock") {
            Audio::sfxCurious();
        }
        else if (soundFx == "thinking" || soundFx == "hmm") {
            Audio::sfxCurious();
        }
        else if (soundFx == "chatter" || soundFx == "talk") {
            Audio::playCuteChatterFree(800, 1200, 100);
        }
        else if (soundFx == "yes" || soundFx == "confirm") {
            Audio::playCuteYes(150);
        }
        else if (soundFx == "no" || soundFx == "deny") {
            Audio::playCuteNo(150);
        }
        else if (soundFx == "notify" || soundFx == "attention") {
            Audio::sfxNotify();
        }
        else if (soundFx == "blink") {
            Audio::sfxBlink();
        }
    }

    // Helper: Blink with sound
    void blinkWithSound() {
        Eyes_Blink();
        if (ENABLE_TAP_SFX && !silentMode) {
            Audio::sfxBlink();
        }
    }

    // Helper: Look in a direction
    void lookDirection(String direction) {
        direction.toLowerCase();
        
        if (direction == "left") {
            Eyes_SetPosition(W);
        }
        else if (direction == "right") {
            Eyes_SetPosition(E);
        }
        else if (direction == "up") {
            Eyes_SetPosition(N);
        }
        else if (direction == "down") {
            Eyes_SetPosition(S);
        }
        else if (direction == "center" || direction == "middle") {
            Eyes_SetPosition(0); // Center position
        }
    }

    // Helper: Adjust eye size
    void adjustEyeSize(String adjustment) {
        adjustment.toLowerCase();
        
        if (adjustment == "widen" || adjustment == "bigger") {
            // Use mood for size changes (simplified approach)
            Eyes_SetMood(3); // Happy mood
        }
        else if (adjustment == "narrow" || adjustment == "smaller") {
            // Use mood for size changes (simplified approach)
            Eyes_SetMood(1); // Tired mood
        }
        else if (adjustment == "normal" || adjustment == "default") {
            // Return to default mood
            Eyes_SetMood(0); // Default mood
        }
    }

    // Helper: Set eye mood
    void setEyeMood(String mood) {
        mood.toLowerCase();
        
        if (mood == "happy") {
            setEyesMood(MS_HAPPY);
            moodUntil = millis() + 3000;
        }
        else if (mood == "angry") {
            setEyesMood(MS_ANGRY);
            moodUntil = millis() + 2000;
        }
        else if (mood == "tired" || mood == "sad") {
            setEyesMood(MS_TIRED);
            moodUntil = millis() + 3000;
        }
        else if (mood == "furious") {
            setEyesMood(MS_FURIOUS);
            moodUntil = millis() + 4000;
        }
        else {
            setEyesMood(MS_DEFAULT);
        }
    }
}
