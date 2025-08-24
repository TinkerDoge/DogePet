// AI Companion Module - Implementation
// Handles AI interaction, background chatter, and structured response processing
#include "ai_companion.h"
#include "gemini_ai.h"
#include "animation_engine.h"
#include "motion.h"
#include <ChronosESP32.h>
#include <string.h>

// Ensure stdint types are available
#ifndef uint32_t
typedef unsigned long uint32_t;
#endif

// External dependencies
extern ChronosESP32 chrono;
extern MoodState curMood;

namespace AICompanion {
    // Helper: strip markdown code fences (```...```) and inline backticks
    static String stripCodeFences(const String& in) {
        String s = in;
        // Remove leading/trailing triple backtick blocks
        int start = s.indexOf("```");
        if (start >= 0) {
            int end = s.indexOf("```", start + 3);
            if (end > start) {
                // Skip optional language tag after opening ```
                int contentStart = s.indexOf('\n', start + 3);
                if (contentStart < 0) contentStart = start + 3;
                String inner = s.substring(contentStart + 1, end);
                s = inner;
            }
        }
        // Remove any stray backticks
        s.replace("`", "");
        return s;
    }

    // Helper: generate a short random cute thought
    static String generateRandomThought() {
        static const char* THOUGHTS[] = {
            "I'm here and listening! 😊",
            "What's up? 👀",
            "Hi! Need me? 🤖✨",
            "Ready when you are! 🌟",
            "Thinking... but happy to chat! 🤔😊",
            "Ping received! 💡",
            "At your service! 🛠️",
            "I like being called! 😄",
            "Curious mode: ON! 🧠✨",
            "Let's do this! 🚀"
        };
        int n = (int)(sizeof(THOUGHTS)/sizeof(THOUGHTS[0]));
        return String(THOUGHTS[random(0, n)]);
    }
    // AI state variables
    uint32_t lastAISendMs = 0;
    uint32_t lastAIChatterMs = 0;
    char aiResponse[256];

    void initialize() {
        // AI initialization is handled in main setup()
    }

    bool isEnabled() {
        return ENABLE_GEMINI_AI && DogeAI.isEnabled() && wifiEnabled;
    }

    bool isReady() {
        return DogeAI.isReady();
    }

    // Generate structured AI prompts with robot state information
    String generateChatterPrompt() {
        // Get current robot state information
        String robotState = getRobotStateInfo();

        // Create a structured prompt that includes robot state and asks for structured response
        String prompt = "You are DogePet, a companion robot with animated eyes and emotions. ";
        prompt += "Current robot state: " + robotState;
        prompt += "\n\n";
        prompt += "Please respond with a JSON-like structure containing:\n";
        prompt += "{\n";
        prompt += "  \"animation\": \"eye animation parameters (e.g., blink, look_left, look_right, widen, narrow, happy, sad, curious, angry)\",\n";
        prompt += "  \"sound_fx\": \"sound effect to play (e.g., curious, happy, thinking, chatter, question, surprise)\",\n";
        prompt += "  \"toast_message\": \"short message to display on screen (keep under 200 chars)\",\n";
        prompt += "  \"thought\": \"your inner thoughts or random observation (1-2 sentences)\"\n";
        prompt += "}\n\n";
        prompt += "Choose one of these scenarios:\n";

        const char* scenarios[] = {
            "React to the current time of day",
            "Comment on your current mood/emotion",
            "Think about the last notification you saw",
            "Wonder about your owner's activities",
            "Reflect on your purpose as a companion robot",
            "Imagine what humans might be doing right now",
            "Consider what you'd like to learn next",
            "Think about your favorite interaction today",
            "Wonder about the future of AI companions",
            "Reflect on the meaning of being 'alive' as a robot"
        };

        int numScenarios = sizeof(scenarios) / sizeof(scenarios[0]);
        prompt += String(scenarios[random(0, numScenarios)]);

        return prompt;
    }

    // Get current robot state information for AI context
    String getRobotStateInfo() {
        String state = "";

        // Time information
        state += "Time: " + String(chrono.getHourC()) + ":" + String(chrono.getMinute());

        // Mood information
        String moodStr;
        switch(curMood) {
            case MS_DEFAULT: moodStr = "neutral"; break;
            case MS_HAPPY: moodStr = "happy"; break;
            case MS_ANGRY: moodStr = "angry"; break;
            case MS_FURIOUS: moodStr = "furious"; break;
            case MS_TIRED: moodStr = "tired"; break;
            default: moodStr = "unknown"; break;
        }
        state += ", Mood: " + moodStr;

        // Battery information
        state += ", Battery: " + String(vbatPercent) + "%";
        if (batteryCharging) state += " (charging)";

        // Last notification info (if available)
        if (strlen(lastNotifTitle) > 0) {
            state += ", Last notification: '" + String(lastNotifTitle) + "'";
            if (strlen(lastNotifBody) > 0) {
                String body = String(lastNotifBody);
                if (body.length() > 50) body = body.substring(0, 47) + "...";
                state += " - " + body;
            }
        }

        // Activity status
        state += ", Activity: ";
        if (jiggling) state += "jiggling";
        else if (furiousJiggling) state += "furious";
        else if (shaking) state += "shaking";
        else state += "calm";

        return state;
    }

    // Parse AI response and execute robot actions
    void executeAIActions(const char* aiResponse) {
        String response = String(aiResponse);
        response = stripCodeFences(response);
        bool didShowToast = false;

        // Extract animation command
        String animation = extractJsonValue(response, "animation");
        if (animation.length() > 0) {
            AnimationEngine::executeEyeAnimation(animation);
        }

        // Extract sound FX command
        String soundFx = extractJsonValue(response, "sound_fx");
        if (soundFx.length() > 0) {
            AnimationEngine::executeSoundFX(soundFx);
        }

        // Extract toast message
        String toastMessage = extractJsonValue(response, "toast_message");
        if (toastMessage.length() > 0) {
            showToast(toastMessage, 8000);
            didShowToast = true;
        }

        // Extract thought (display as longer toast if no specific message)
        String thought = extractJsonValue(response, "thought");
        if (thought.length() > 0 && toastMessage.length() == 0) {
            // Split long thoughts into shorter toasts
            if (thought.length() > 60) {
                thought = thought.substring(0, 57) + "...";
            }
            showToast(thought, 8000);
            didShowToast = true;
        }

        // Fallback: if no structured fields were actionable, show raw response as toast
        if (!didShowToast) {
            String fallback = response;
            // If the cleaned response looks like JSON and has no toast_message, generate a random thought
            if ((fallback.startsWith("{") || fallback.startsWith("[")) && extractJsonValue(fallback, "toast_message").length() == 0) {
                fallback = generateRandomThought();
            }
            if (fallback.length() > 200) fallback = fallback.substring(0, 197) + "...";
            showToast(fallback, 9000);
        }
    }

    // Extract value from JSON-like string (simple parser)
    String extractJsonValue(String json, String key) {
        String searchKey = "\"" + key + "\":";
        int keyIndex = json.indexOf(searchKey);

        if (keyIndex == -1) return "";

        int valueStart = json.indexOf("\"", keyIndex + searchKey.length());
        if (valueStart == -1) return "";

        int valueEnd = json.indexOf("\"", valueStart + 1);
        if (valueEnd == -1) return "";

        String value = json.substring(valueStart + 1, valueEnd);

        // Clean up the value (remove extra quotes, trim)
        value.trim();
        if (value.startsWith("\"") && value.endsWith("\"")) {
            value = value.substring(1, value.length() - 1);
        }

        return value;
    }

    // Handle background AI chatter
    void handleBackgroundChatter() {
        if (ENABLE_AI_DEBUG_LOGS) Serial.println("[DEBUG] handleBackgroundChatter() called");
        
        if (!ENABLE_AI_CHATTER) {
            if (ENABLE_AI_DEBUG_LOGS) Serial.println("[DEBUG] AI_CHATTER disabled in config");
            return;
        }
        
        if (!ENABLE_GEMINI_AI) {
            if (ENABLE_AI_DEBUG_LOGS) Serial.println("[DEBUG] GEMINI_AI disabled in config");
            return;
        }
        
        if (!wifiEnabled) {
            if (ENABLE_AI_DEBUG_LOGS) Serial.println("[DEBUG] WiFi not enabled");
            return;
        }

        uint32_t currentMs = millis();
        uint32_t timeSinceLastChatter = currentMs - lastAIChatterMs;
        if (ENABLE_AI_DEBUG_LOGS) Serial.printf("[DEBUG] Time since last chatter: %lu ms (interval: %lu ms)\n", 
                     timeSinceLastChatter, AI_CHATTER_INTERVAL_MS);
        
        if (timeSinceLastChatter < AI_CHATTER_INTERVAL_MS) {
            static uint32_t lastNotTimeLog = 0;
            if (ENABLE_AI_DEBUG_LOGS && currentMs - lastNotTimeLog > 3000) {
                Serial.printf("[DEBUG] Not time for chatter yet, waiting %lu ms\n", 
                             AI_CHATTER_INTERVAL_MS - timeSinceLastChatter);
                lastNotTimeLog = currentMs;
            }
            return;
        }

        // Only chatter when idle (not processing other messages)
        if (!DogeAI.isReady()) {
            if (ENABLE_AI_DEBUG_LOGS) Serial.printf("[DEBUG] DogeAI not ready, state: %d\n", DogeAI.getState());
            return;
        }

        if (ENABLE_AI_DEBUG_LOGS) Serial.println("[DEBUG] Starting AI background chatter");
        lastAIChatterMs = currentMs;

        // Generate a random prompt and send to AI
        String prompt = generateChatterPrompt();
        if (ENABLE_AI_DEBUG_LOGS) Serial.printf("[DEBUG] Generated prompt: %s\n", prompt.c_str());

        // Send with a special marker to distinguish from user messages
        String aiPrompt = "[THINKING] " + prompt;
        if (ENABLE_AI_DEBUG_LOGS) Serial.printf("[DEBUG] Sending to handleMessage: %s\n", aiPrompt.c_str());
        handleMessage(aiPrompt.c_str());
    }

    // Handle AI message and send to Gemini
    void handleMessage(const char* message) {
        Serial.printf("[DEBUG] handleMessage() called with: %s\n", message);
        
        Serial.printf("[DEBUG] Checking conditions - GEMINI_AI: %s, DogeAI.isEnabled(): %s, wifiEnabled: %s\n",
                     ENABLE_GEMINI_AI ? "true" : "false",
                     DogeAI.isEnabled() ? "true" : "false", 
                     wifiEnabled ? "true" : "false");
        
        if (!ENABLE_GEMINI_AI || !DogeAI.isEnabled() || !wifiEnabled) {
            if (!wifiEnabled) {
                Serial.println("[DEBUG] AI disabled - WiFi is off");
                showToast("AI: WiFi Off", 2000);
            } else if (!ENABLE_GEMINI_AI) {
                Serial.println("[DEBUG] AI disabled - GEMINI_AI config is false");
            } else {
                Serial.println("[DEBUG] AI disabled - DogeAI.isEnabled() is false");
            }
            return;
        }

        Serial.printf("[DEBUG] DogeAI state: %d, isReady(): %s\n", 
                     DogeAI.getState(), DogeAI.isReady() ? "true" : "false");

        if (!DogeAI.isReady()) {
            Serial.println("[DEBUG] AI not ready - returning");
            return;
        }

        // Check cooldown (skip if voice triggered)
        uint32_t timeSinceLastSend = millis() - lastAISendMs;
        bool isBackgroundChatter = strstr(message, "[THINKING]") != nullptr;
        bool isVoiceTrigger = strstr(message, "[VOICE_TRIGGER]") != nullptr;
        if (ENABLE_AI_DEBUG_LOGS) Serial.printf("[DEBUG] Time since last send: %lu ms (cooldown: %lu ms)\n", 
                     timeSinceLastSend, GEMINI_COOLDOWN_MS);
        // Use randomized cooldown window; skip for voice triggers
        uint32_t cdMin = GEMINI_COOLDOWN_MIN_MS;
        uint32_t cdMax = GEMINI_COOLDOWN_MAX_MS;
        if (cdMax < cdMin) { uint32_t t = cdMin; cdMin = cdMax; cdMax = t; }
        uint32_t effectiveCooldown = GEMINI_COOLDOWN_MS;
        if (cdMax > cdMin) {
            // Choose a deterministic pseudo-random cooldown per cycle
            uint32_t jitter = (uint32_t)(esp_random() % (cdMax - cdMin + 1));
            effectiveCooldown = cdMin + jitter;
        }
        if (!isVoiceTrigger && timeSinceLastSend < effectiveCooldown) {
            if (ENABLE_AI_DEBUG_LOGS) Serial.printf("[DEBUG] AI cooldown active - waiting %lu ms\n", 
                         effectiveCooldown - timeSinceLastSend);
            return;
        }

        // Check message type
        // Note: isBackgroundChatter and isVoiceTrigger moved above for cooldown logic
        
        String messageType = "user message";
        if (isBackgroundChatter) messageType = "background chatter";
        else if (isVoiceTrigger) messageType = "voice trigger";
        
        Serial.printf("[DEBUG] Message type: %s\n", messageType.c_str());

        // Show appropriate processing indicator with emojis
        if (isBackgroundChatter) {
            Serial.println("[DEBUG] Showing 'Dreaming...' toast");
            showToast("Dreaming... 💭", 2000);
        } else if (isVoiceTrigger) {
            Serial.println("[DEBUG] Showing 'Listening...' toast");
            showToast("Listening... 👂", 2000);
        } else {
            Serial.println("[DEBUG] Showing 'Thinking...' toast");
            showToast("Thinking... 🤔", 2000);
        }

        Serial.println("[DEBUG] Calling DogeAI.sendMessage()");

        // For voice triggers, wrap with concise "called by user" instruction that asks for a plain-text cute thought
        const char* sendPtr = message;
        String wrapped;
        if (isVoiceTrigger) {
            wrapped.reserve(512);
            wrapped += GEMINI_SYSTEM_PROMPT;
            wrapped += "\n\nThe user is calling to you (not a noise).\n";
            wrapped += "Reply with ONE short, cute random thought as PLAIN TEXT only.\n";
            wrapped += "Constraints: <= 160 chars, include one emoji, no markdown, no quotes, no code.\n\n";
            // Strip tag from user content (keep for context if present)
            const char* content = strstr(message, "]");
            if (content && *(content+1) == ' ') content += 2; else content = message;
            wrapped += "User: ";
            wrapped += content;
            sendPtr = wrapped.c_str();
        }

        // Send message to Gemini
        if (DogeAI.sendMessage(sendPtr, aiResponse, sizeof(aiResponse))) {
            lastAISendMs = millis();
            Serial.printf("[DEBUG] AI Response received: %s\n", aiResponse);

            if (isBackgroundChatter) {
                Serial.println("[DEBUG] Processing as background chatter - calling executeAIActions()");
                executeAIActions(aiResponse);
            } else if (isVoiceTrigger) {
                Serial.println("[DEBUG] Processing as voice trigger - showing text toast");
                String cleaned = stripCodeFences(String(aiResponse));
                if (cleaned.length() > 200) cleaned = cleaned.substring(0, 197) + "...";
                showToast(cleaned, 9000);
                // Optional small friendly animation
                AnimationEngine::executeEyeAnimation("happy");
            } else {
                Serial.println("[DEBUG] Processing as user message - showing response toast");
                showToast(String(aiResponse), 5000);

                // Happy reaction for user interactions
                setEyesMood(MS_HAPPY);
                moodUntil = millis() + MOOD_HOLD_MS;
            }
        } else {
            // Show error
            Serial.printf("[DEBUG] AI Error occurred: %s\n", DogeAI.getLastError());
            showToast("AI Error! 😅", 3000);
        }
    }
}
