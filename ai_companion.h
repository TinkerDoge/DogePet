// AI Companion Module - Header
// Handles AI interaction, background chatter, and structured response processing
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "config.h"

// Ensure stdint types are available
#ifndef uint32_t
typedef unsigned long uint32_t;
#endif
#ifndef uint16_t  
typedef unsigned short uint16_t;
#endif

// Forward declarations
class GeminiAI;
extern GeminiAI DogeAI;

// AI companion functionality
namespace AICompanion {
    // AI state management
    void initialize();
    bool isEnabled();
    bool isReady();
    
    // Message handling
    void handleMessage(const char* message);
    void handleBackgroundChatter();
    
    // Robot state information for AI context
    String getRobotStateInfo();
    String generateChatterPrompt();
    
    // Structured response processing
    void executeAIActions(const char* aiResponse);
    String extractJsonValue(String json, String key);
    
    // AI timing management
    extern uint32_t lastAISendMs;
    extern uint32_t lastAIChatterMs;
    extern char aiResponse[256];
}

// External dependencies (declared in main)
extern bool wifiEnabled;
extern uint32_t moodUntil;
extern char lastNotifTitle[64];
extern char lastNotifBody[128];
extern bool jiggling;
extern bool furiousJiggling;
extern bool shaking;
extern int vbatPercent;
extern bool batteryCharging;

// External function dependencies
extern void showToast(const String& s, uint16_t ms);
