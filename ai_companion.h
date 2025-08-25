// AI Companion Module - Header
// Consolidated AI module handling all AI interaction, background chatter, and API communication
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <WiFi.h>
#include "config.h"

// Ensure stdint types are available
#ifndef uint32_t
typedef unsigned long uint32_t;
#endif
#ifndef uint16_t
typedef unsigned short uint16_t;
#endif

// Gemini API Configuration
#define GEMINI_API_BASE_URL "https://generativelanguage.googleapis.com"
#define GEMINI_API_ENDPOINT "/v1beta/models/"
#define GEMINI_API_KEY_MAX_LEN 128
#define GEMINI_MAX_RESPONSE_LEN 1024
#define GEMINI_REQUEST_TIMEOUT_MS 15000
#define GEMINI_MAX_URL_LEN 256

// AI Personality settings
#define GEMINI_SYSTEM_PROMPT "You are DogePet, a cute robot. Keep responses under 250 chars. Use emojis. Optionally include a compact SFX field 'sfx' describing a short sequence in this format: 'f0>f1,ms,wave,vol; ...' where wave=S/Q/N. Keep sfx under 3 entries."

// AI State
enum AIState {
    AI_IDLE,
    AI_LISTENING,
    AI_PROCESSING,
    AI_RESPONDING,
    AI_ERROR
};

// Consolidated AI companion functionality
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

    // Gemini AI integration (consolidated)
    bool begin(const char* apiKey);
    bool sendMessage(const char* message, char* response, size_t maxResponseLen);
    AIState getState();
    const char* getLastError();
    void enable(bool state);
    void clearHistory();
    void getUsageStats(uint32_t& totalRequests, uint32_t& failedRequests);
    static bool testJsonStructure();

    // AI timing management
    extern uint32_t lastAISendMs;
    extern uint32_t lastAIChatterMs;
    extern char aiResponse[1024];

    // Internal state variables
    extern AIState currentState;
    extern char lastError[64];
    extern bool enabled;
    extern uint32_t requestCount;
    extern uint32_t errorCount;
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
