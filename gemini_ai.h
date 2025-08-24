// Gemini AI Integration for DogePet
// Connects to Google's Gemini API to enable AI conversation capabilities
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <WiFi.h>
#include "config.h"

// Gemini API Configuration
#define GEMINI_API_BASE_URL "https://generativelanguage.googleapis.com/v1beta/models/"
#define GEMINI_API_ENDPOINT ":generateContent"
#define GEMINI_API_KEY_MAX_LEN 64
#define GEMINI_MAX_RESPONSE_LEN 512
#define GEMINI_REQUEST_TIMEOUT_MS 15000
#define GEMINI_MAX_URL_LEN 256

// AI Personality settings
#define GEMINI_SYSTEM_PROMPT "You are DogePet, a cute robotic companion with animated eyes and emotions. You have a playful, friendly personality and love to chat about anything. Keep your responses short and sweet (under 100 characters) since you're displayed on a small screen. Use emojis frequently to show your personality: 😊😢😠🤔💭👂🎉🔥⚡💡🔋📱💻🎵🤖🛠⚙🔧🤗😴😪💤🌟✨🎊. Match your emoji to your mood and the conversation topic."

// AI State
enum AIState {
    AI_IDLE,
    AI_LISTENING,
    AI_PROCESSING,
    AI_RESPONDING,
    AI_ERROR
};

class GeminiAI {
public:
    // Constructor
    GeminiAI();

    // Initialize AI with API key
    bool begin(const char* apiKey);

    // Send message to Gemini and get response
    bool sendMessage(const char* message, char* response, size_t maxResponseLen);

    // Get current AI state
    AIState getState() { return currentState; }

    // Check if AI is ready for new input
    bool isReady() { return currentState == AI_IDLE; }

    // Get last error message
    const char* getLastError() { return lastError; }

    // Enable/disable AI features
    void enable(bool state) { enabled = state; }
    bool isEnabled() { return enabled; }

    // Optional: clear stored model history
    void clearHistory() { hasHistory = false; lastModelText[0] = '\0'; }

private:
    // API key storage
    char apiKey[GEMINI_API_KEY_MAX_LEN];

    // State tracking
    AIState currentState;
    char lastError[64];

    // HTTP client state
    bool enabled;

    // Minimal chat history (previous model reply)
    bool hasHistory = false;
    char lastModelText[GEMINI_MAX_RESPONSE_LEN];

    // HTTP request helper
    bool makeHttpRequest(const char* jsonPayload, char* response, size_t maxResponseLen);

    // JSON parsing helpers
    bool extractResponseText(const char* jsonResponse, char* output, size_t maxOutputLen);

    // Memory-efficient JSON building
    size_t buildRequestJson(const char* userMessage, char* buffer, size_t bufferSize);
};

// Global AI instance
extern GeminiAI DogeAI;
