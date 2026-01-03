// Animation.h - High-level animation sequences
#pragma once

#include <Arduino.h>
#include "Face.h"

class Animation {
public:
    static void init();
    static void playStartupSequence();
    static void tick();  // Call in loop() for behavior tree
};
