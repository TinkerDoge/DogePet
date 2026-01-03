// Events.h - Central event handler for touch → behavior reactions
#pragma once

#include <Arduino.h>

class Events {
public:
    static void init();
    static void update();  // Call in loop() - processes touch events
};
