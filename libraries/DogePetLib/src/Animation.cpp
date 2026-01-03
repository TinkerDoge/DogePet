// Animation.cpp - High-level animation sequences
#include "Animation.h"
#include "Audio.h"

void Animation::init() {
    // Ready state
}

void Animation::playStartupSequence() {
    roboEyes* eyes = Face::getEyes();
    
    // 1. Blink
    eyes->setMood(0);
    eyes->blink();
    
    // 2. Laugh animation
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
    eyes->setMood(3);
    Face::update();
    
    // Audio feedback
    Audio::playMelody();
    
    delay(500);
    
    // Reset to normal
    eyes->setMood(0);
}

void Animation::tick() {
    // Behavior tree loop - can expand here
}
