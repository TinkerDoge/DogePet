// Animation Engine Module - Header  
// Handles eye animations and sound effects for the robot
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "config.h"
#include "motion.h"

// Ensure stdint types are available
#ifndef uint32_t
typedef unsigned long uint32_t;
#endif
#ifndef uint8_t  
typedef unsigned char uint8_t;
#endif

// Forward declarations - using external wrapper functions instead of direct access

// Animation engine functionality
namespace AnimationEngine {
    // Eye animation commands
    void executeEyeAnimation(String animation);
    
    // Sound effect commands
    void executeSoundFX(String soundFx);
    
    // Animation helpers
    void blinkWithSound();
    void lookDirection(String direction);
    void adjustEyeSize(String adjustment);
    void setEyeMood(String mood);
}

// External dependencies (declared in main)
extern bool silentMode;
extern uint32_t moodUntil;

// External function dependencies
extern void setEyesMood(MoodState mood);
