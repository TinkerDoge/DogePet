#include "AnimationsEngine.h"
#include "../../../audio.h"

// Ensure delay function is available
#ifndef delay
#define delay(x) ::delay(x)
#endif

// External function wrappers supplied by main application
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

namespace AnimationsEngine {

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
        Eyes_SetPosition(0); // Center position
    }
    else if (animation == "look_right") {
        Eyes_SetPosition(E);
        delay(500);
        Eyes_SetPosition(0);
    }
    else if (animation == "look_up") {
        Eyes_SetPosition(N);
        delay(500);
        Eyes_SetPosition(0);
    }
    else if (animation == "look_down") {
        Eyes_SetPosition(S);
        delay(500);
        Eyes_SetPosition(0);
    }
    else if (animation == "widen") {
        Serial.println("AI Animation: widen (using mood change)");
        Eyes_SetMood(3);
        delay(800);
        Eyes_SetMood(0);
    }
    else if (animation == "narrow") {
        Serial.println("AI Animation: narrow (using mood change)");
        Eyes_SetMood(1);
        delay(800);
        Eyes_SetMood(0);
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
        Eyes_SetPosition(0);
        if (!silentMode) {
            Audio::playCuteQuestion(100);
        }
    }
    else if (animation == "surprise") {
        Serial.println("AI Animation: surprise (using mood change)");
        Eyes_SetMood(3);
        delay(600);
        Eyes_SetMood(0);
    }
    else if (animation == "thinking") {
        Eyes_Blink();
        delay(200);
        Eyes_Blink();
        delay(200);
    }
    else if (animation == "sleepy") {
        Eyes_SetMood(1);
        delay(1000);
        Eyes_SetMood(0);
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
        Eyes_SetPosition(0);
    }
}

// Helper: Adjust eye size
void adjustEyeSize(String adjustment) {
    adjustment.toLowerCase();

    if (adjustment == "widen" || adjustment == "bigger") {
        Eyes_SetMood(3);
    }
    else if (adjustment == "narrow" || adjustment == "smaller") {
        Eyes_SetMood(1);
    }
    else if (adjustment == "normal" || adjustment == "default") {
        Eyes_SetMood(0);
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

} // namespace AnimationsEngine
