#include "include/Animation.h"
#include "include/Audio.h"

void Animation::init() {
    // Ready state
}

void Animation::playStartupSequence() {
    roboEyes* eyes = Face::getEyes();
    
    // 1. Blink
    eyes->setMood(0); // Normal
    eyes->blink(); // Non-blocking trigger, but we want sequence?
    // RoboEyes blink is non-blocking state change.
    
    // 2. Laugh Animation
    // Manually animate mood/position
    for(int i=0; i<3; i++) {
        eyes->setPosition(1); // N
        Face::update();
        delay(100);
        eyes->setPosition(5); // S
        Face::update();
        delay(100);
    }
    eyes->setPosition(0); // Center
    
    // Happy mood
    eyes->setMood(3); // Happy
    Face::update();
    
    // Audio feedback
    Audio::playMelody();
    
    // Wait a bit
    delay(500);
    
    // Reset to normal
    eyes->setMood(0);
}

void Animation::tick() {
    // Main behavior tree loop
    // Currently relying on RoboEyes internal auto-blink/idle
    // Can expand here for complex logic
}
