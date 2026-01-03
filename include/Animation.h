#ifndef ANIMATION_H
#define ANIMATION_H

#include "Face.h"

class Animation {
public:
    static void init();
    static void playStartupSequence();
    static void tick(); // Call in loop for behavior tree

private:
    static void blinkThenLaugh();
};

#endif
