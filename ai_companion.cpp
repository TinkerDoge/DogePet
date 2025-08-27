// AI Companion Module - Implementation
// Consolidated AI module handling all AI interaction, background chatter, and API communication
#include <Arduino.h>
#include "ai_companion.h"
#include "animation_engine.h"
#include "audio.h"
#include "motion.h"
#include <ChronosESP32.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <string.h>
#include <cstring>
#include <cstdlib>
#include <esp_system.h>
#include <cstdio>

// Silence all Serial logs in this module when AI debug logs are disabled
#if defined(ARDUINO)
#if !ENABLE_AI_DEBUG_LOGS
struct AI_QuietSerial_ {
    template<typename... Args> void printf(const char*, Args...){ }
    template<typename... Args> void print(Args...){ }
    template<typename... Args> void println(Args...){ }
};
static AI_QuietSerial_ AI_QSERIAL;
#undef Serial
#define Serial AI_QSERIAL
#endif
#endif

// Lint-time fallbacks for non-Arduino analysis environments
#ifndef ARDUINO
struct DummySerial_ { template<typename... Args> void printf(const char*, Args...){ } void println(const char*){} void print(const char*){} };
static DummySerial_ Serial;
static inline unsigned long millis(){ return 0; }
static inline void delay(unsigned long) {}
#endif

#ifndef ESP_PLATFORM
static inline uint32_t esp_random(){ return (uint32_t)rand(); }
#endif

// Minimal C string fallbacks for lint-only environments (must appear before other fallbacks)
#ifndef ARDUINO
static inline size_t strlen(const char* s){ size_t n=0; if(!s) return 0; while(s[n]) ++n; return n; }
static inline void* memcpy(void* d, const void* s, size_t n){
    unsigned char* dp=(unsigned char*)d; const unsigned char* sp=(const unsigned char*)s;
    for(size_t i=0;i<n;++i) dp[i]=sp[i]; return d;
}
static inline char* strcpy(char* d, const char* s){ char* o=d; while((*d++=*s++)); return o; }
static inline char* strncpy(char* d, const char* s, size_t n){ size_t i=0; for(; i<n && s[i]; ++i) d[i]=s[i]; for(; i<n; ++i) d[i]='\0'; return d; }
static inline const char* strchr(const char* s, int c){ while(*s){ if(*s==(char)c) return s; ++s; } return c==0?s:nullptr; }
static inline const char* strstr(const char* h, const char* n){ if(!*n) return h; for(; *h; ++h){ if(*h==*n){ const char* a=h; const char* b=n; while(*a && *b && *a==*b){ ++a; ++b; } if(!*b) return h; } } return nullptr; }
#endif

#if !defined(ARDUINO)
#ifndef HAVE_STRLCPY_FALLBACK
#define HAVE_STRLCPY_FALLBACK 1
static inline size_t strlcpy(char* dst, const char* src, size_t size){
    size_t len = strlen(src);
    size_t copy = (size == 0) ? 0 : (len < (size - 1) ? len : (size - 1));
    if (size > 0) { memcpy(dst, src, copy); dst[copy] = '\0'; }
    return len;
}
#endif
#endif

// Ensure stdint types are available
#ifndef uint32_t
typedef unsigned long uint32_t;
#endif

// External dependencies
extern ChronosESP32 chrono;
extern MoodState curMood;

// Internal state variables
namespace AICompanion {
    // API key storage
    char apiKey[GEMINI_API_KEY_MAX_LEN] = {0};

    // State tracking
    AIState currentState = AI_IDLE;
    char lastError[64] = {0};

    // Minimal chat history (previous model reply)
    bool hasHistory = false;
    char lastModelText[GEMINI_MAX_RESPONSE_LEN] = {0};

    // Usage tracking for token monitoring
    uint32_t requestCount = 0;
    uint32_t errorCount = 0;

    // AI timing management
    uint32_t lastAISendMs = 0;
    uint32_t lastAIChatterMs = 0;
    char aiResponse[1024] = {0};

    // Missing variable declarations
    bool enabled = false;

    // Helper: strip markdown code fences (```...```) and inline backticks
    static String stripCodeFences(const String& in) {
        String s = in;
        // Unwrap or remove all triple-backtick fenced blocks, preserving inner content when present
        int start = s.indexOf("```");
        while (start >= 0) {
            int afterOpenNewline = s.indexOf('\n', start + 3);
            int contentStart = (afterOpenNewline > start) ? afterOpenNewline + 1 : start + 3;
            int end = s.indexOf("```", contentStart);
            if (end < 0) {
                // No closing fence: take remainder as inner content
                String inner = s.substring(contentStart);
                s = s.substring(0, start) + inner;
                break;
            } else {
                String inner = s.substring(contentStart, end);
                s = s.substring(0, start) + inner + s.substring(end + 3);
            }
            // look for next fence
            start = s.indexOf("```");
        }

        // Remove any remaining inline backticks and trim whitespace
        s.replace("`", "");
        s.trim();
        return s;
    }

    // Helper: generate a short random cute thought
    static String generateRandomThought() {
        static const char* THOUGHTS[] = {
            "I'm here and listening! ????",
            "What's up? ????",
            "Hi! Need me? ???????",
            "Ready when you are! ????",
            "Thinking... but happy to chat! ????????",
            "Ping received! ????",
            "At your service! ???????",
            "I like being called! ????",
            "Curious mode: ON! ???????",
            "Let's do this! ????"
        };
        int n = (int)(sizeof(THOUGHTS)/sizeof(THOUGHTS[0]));
        int idx = (int)(esp_random() % (uint32_t)n);
        return String(THOUGHTS[idx]);
    }

    // Helper: sanitize any model text for toast display
    static String sanitizeDisplayText(const String& input) {
        String s = stripCodeFences(input);
        // Remove any inline or line-start SFX specifications (case-insensitive)
        {
            String sLower = String(s);
            sLower.toLowerCase();
            int pos = sLower.indexOf("sfx:");
            while (pos >= 0) {
                int eol = s.indexOf('\n', pos);
                if (eol < 0) eol = s.length();
                s.remove(pos, eol - pos);
                sLower = String(s);
                sLower.toLowerCase();
                pos = sLower.indexOf("sfx:");
            }
        }
        // Remove bracket tags and common meta markers
        s.replace("[THINKING]", "");
        s.replace("[VOICE_TRIGGER]", "");
        // Remove lines that are not user-facing (SFX/spec/prompts/system/history)
        String out; out.reserve(s.length());
        int start = 0;
        while (start < (int)s.length()) {
            int nl = s.indexOf('\n', start);
            if (nl < 0) nl = s.length();
            String line = s.substring(start, nl);
            String ltrim = line; ltrim.trim();
            bool drop = false;
            if (ltrim.startsWith("SFX:") || ltrim.startsWith("sfx:")) drop = true;
            if (ltrim.startsWith("System:") || ltrim.startsWith("Instruction:")) drop = true;
            if (ltrim.startsWith("Inner thought") || ltrim.startsWith("Thought:")) drop = true;
            
            if (ltrim.startsWith("Previous reply for context:")) drop = true;
            if (ltrim.startsWith("DogePet:")) {
                // Often echo of prompt; drop it
                drop = true;
            }
            if (!drop) {
                if (out.length() > 0) out += '\n';
                out += line;
            }
            start = nl + 1;
        }
        out.trim();
        return out;
    }

    // Minimal inline parser to play compact SFX sequences without linking against Audio helper
    static float clampf(float v, float a, float b){ if (v<a) return a; if (v>b) return b; return v; }

    static void playSfxSequenceInline(const char* spec) {
        if (!spec) return;
        const char* s = strstr(spec, ":");
        if (s) s++; else s = spec;
        while (*s) {
            while (*s==' '||*s=='\t'||*s==';') s++;
            if (!*s) break;
            float f0=0, f1=0; unsigned ms=0; char w='S'; float volf=180.0f;
            const char* p = s;
            f0 = strtof(p, (char**)&p);
            if (*p=='>'){ p++; f1 = strtof(p,(char**)&p);} else { f1 = f0; }
            if (*p==',') p++;
            ms = (unsigned)strtoul(p, (char**)&p, 10);
            if (*p==',') p++;
            if (*p){ w = *p; p++; }
            if (*p==',') p++;
            // volume can be 0..1 float or 0..255 int
            char* pend = nullptr;
            float tryF = strtof(p, &pend);
            if (pend && pend != p) {
                if (tryF <= 1.0f) volf = clampf(tryF, 0.0f, 1.0f) * 255.0f; else volf = clampf(tryF, 0.0f, 255.0f);
                p = pend;
            } else {
                unsigned voli = (unsigned)strtoul(p, (char**)&p, 10);
                volf = (float)voli;
            }
            uint8_t wave = (w=='Q'||w=='q') ? Audio::WAVE_SQUARE : (w=='N'||w=='n') ? Audio::WAVE_NOISE : Audio::WAVE_SINE;
            if (ms > 2000) ms = 2000; if (volf > 255.0f) volf = 255.0f; if (volf < 0.0f) volf = 0.0f;
            Audio::playBeep(f0, (uint16_t)ms, (uint8_t)volf, wave);
            delay(6);
            s = strchr(p, ';');
            if (!s) break; else s++;
        }
    }

    // ===== GEMINI AI IMPLEMENTATION (consolidated) =====

    // Forward declarations for helper functions
    static size_t buildRequestJson(const char* userMessage, char* buffer, size_t bufferSize);
    static bool makeHttpRequest(const char* jsonPayload, char* response, size_t maxResponseLen);
    static bool extractResponseText(const char* jsonResponse, char* output, size_t maxOutputLen);
    static bool testJsonStructure();

    // ===== HELPER FUNCTION DEFINITIONS =====

    size_t buildRequestJson(const char* userMessage, char* buffer, size_t bufferSize) {
        StaticJsonDocument<512> doc;

        // Build request for Gemini 2.5 Flash API structure
        JsonArray contents = doc.createNestedArray("contents");
        JsonObject contentObj = contents.createNestedObject();
        JsonArray parts = contentObj.createNestedArray("parts");
        JsonObject part = parts.createNestedObject();

        // Combine system prompt with user message for efficiency
        String fullPrompt = String(GEMINI_SYSTEM_PROMPT) + "\n\n" + userMessage;
        part["text"] = fullPrompt;

        // Optimized generation config for shorter responses
        JsonObject gen = doc.createNestedObject("generationConfig");
        gen["maxOutputTokens"] = 1024;
        gen["temperature"] = 0.7;
        gen["responseMimeType"] = "text/plain";

        size_t len = serializeJson(doc, buffer, bufferSize);
        Serial.printf("[GEMINI DEBUG] Generated JSON (length: %d): %s\n", len, buffer);
        return len;
    }

    bool makeHttpRequest(const char* jsonPayload, char* response, size_t maxResponseLen) {
        Serial.println("[GEMINI DEBUG] Creating WiFiClientSecure");
        WiFiClientSecure client;
        client.setInsecure();
        client.setTimeout(15000);

        char path[GEMINI_MAX_URL_LEN];
        Serial.printf("[GEMINI DEBUG] Building path with model: %s, API key length: %d\n",
                     GEMINI_MODEL, strlen(apiKey));
        int pathLen = snprintf(path, sizeof(path), "/v1beta/models/%s:generateContent?key=%s", GEMINI_MODEL, apiKey);
        Serial.printf("[GEMINI DEBUG] Path length: %d, buffer size: %d\n", pathLen, sizeof(path));
        Serial.printf("[GEMINI DEBUG] Constructed path: %s\n", path);

        if (pathLen >= sizeof(path)) {
            Serial.println("[GEMINI DEBUG] WARNING: Path was truncated!");
            strcpy(lastError, "URL too long");
            return false;
        }

        Serial.println("[GEMINI DEBUG] Attempting to connect to generativelanguage.googleapis.com:443");
        if (!client.connect("generativelanguage.googleapis.com", 443)) {
            strcpy(lastError, "Connection failed");
            Serial.println("[GEMINI DEBUG] Connection failed!");
            return false;
        }
        Serial.println("[GEMINI DEBUG] Connected successfully!");

        Serial.println("[GEMINI DEBUG] Sending HTTP headers");
        client.printf("POST %s HTTP/1.1\r\n", path);
        client.printf("Host: generativelanguage.googleapis.com\r\n");
        client.printf("Content-Type: application/json\r\n");
        client.printf("Content-Length: %d\r\n", strlen(jsonPayload));
        client.printf("Connection: close\r\n\r\n");

        Serial.println("[GEMINI DEBUG] Sending JSON payload");
        client.print(jsonPayload);
        Serial.println("[GEMINI DEBUG] Request sent, waiting for response");

        // Wait for response
        unsigned long timeout = millis() + GEMINI_REQUEST_TIMEOUT_MS;
        while (client.available() == 0) {
            if (millis() > timeout) {
                client.stop();
                strcpy(lastError, "Request timeout");
                Serial.printf("[GEMINI DEBUG] Request timeout after %d ms\n", GEMINI_REQUEST_TIMEOUT_MS);
                return false;
            }
            delay(10);
        }

        Serial.println("[GEMINI DEBUG] Response received, reading data");

        // Read response fully
        String responseString;
        uint32_t readStart = millis();
        while (client.connected() || client.available()) {
            if (client.available()) {
                char buf[512];
                int n = client.read((uint8_t*)buf, sizeof(buf) - 1);
                if (n > 0) {
                    buf[n] = '\0';
                    responseString += buf;
                    readStart = millis();
                }
            } else {
                if (millis() - readStart > GEMINI_REQUEST_TIMEOUT_MS) break;
                delay(10);
            }
        }
        client.stop();

        Serial.printf("[GEMINI DEBUG] Response length: %d\n", responseString.length());

        if (responseString.length() == 0) {
            strcpy(lastError, "Empty response");
            Serial.println("[GEMINI DEBUG] Empty response received");
            return false;
        }

        // Try to locate JSON body
        int idx = responseString.indexOf('{');
        if (idx >= 0) {
            String body = responseString.substring(idx);
            Serial.printf("[GEMINI DEBUG] Raw response body: %s\n", body.c_str());
            return extractResponseText(body.c_str(), response, maxResponseLen);
        }

        strcpy(lastError, "No JSON body found");
        Serial.println("[GEMINI DEBUG] No JSON body found in response");
        return false;
    }

    bool extractResponseText(const char* jsonResponse, char* output, size_t maxOutputLen) {
        StaticJsonDocument<1536> doc;

        DeserializationError error = deserializeJson(doc, jsonResponse);
        if (error) {
            snprintf(lastError, sizeof(lastError), "JSON parse error: %s", error.c_str());
            return false;
        }

        // Check for API errors
        if (doc.containsKey("error")) {
            const char* errorMsg = doc["error"]["message"] | "Unknown API error";
            strncpy(lastError, errorMsg, sizeof(lastError) - 1);
            return false;
        }

        // Gemini 2.5 Flash response structure
        const char* responseText = nullptr;

        if (doc.containsKey("candidates") && doc["candidates"].size() > 0) {
            JsonObject candidate = doc["candidates"][0];
            if (candidate.containsKey("content") && candidate["content"].containsKey("parts")) {
                JsonArray parts = candidate["content"]["parts"];
                if (parts.size() > 0 && parts[0].containsKey("text")) {
                    responseText = parts[0]["text"];
                }
            }
        }

        if (!responseText && doc.containsKey("text")) {
            responseText = doc["text"];
        }

        if (!responseText) {
            strcpy(lastError, "No response text found");
            return false;
        }

        // Copy response with size limit
        size_t len = strlen(responseText);
        if (len >= maxOutputLen) {
            len = maxOutputLen - 1;
        }

        memcpy(output, responseText, len);
        output[len] = '\0';

        return true;
    }

    bool testJsonStructure() {
        char testBuffer[256];
        const char* testMessage = "Test: hello";

        size_t len = buildRequestJson(testMessage, testBuffer, sizeof(testBuffer));
        if (len == 0) {
            Serial.println("JSON build test FAILED - no data generated");
            return false;
        }

        Serial.printf("JSON structure test - length: %d\n", len);
        Serial.printf("JSON: %s\n", testBuffer);

        String jsonStr = String(testBuffer);
        bool hasContents = jsonStr.indexOf("\"contents\"") >= 0;
        bool hasParts = jsonStr.indexOf("\"parts\"") >= 0;
        bool hasText = jsonStr.indexOf("\"text\"") >= 0;
        bool hasConfig = jsonStr.indexOf("\"generationConfig\"") >= 0;

        Serial.printf("Structure validation: contents=%d, parts=%d, text=%d, config=%d\n",
                      hasContents, hasParts, hasText, hasConfig);

        return hasContents && hasParts && hasText && hasConfig;
    }

    // ===== MAIN AI METHODS =====

    bool begin(const char* key) {
        if (!key || strlen(key) >= GEMINI_API_KEY_MAX_LEN) {
            strcpy(lastError, "Invalid API key");
            return false;
        }

        strcpy(apiKey, key);
        enabled = true;
        currentState = AI_IDLE;
        strcpy(lastError, "");

        Serial.println("Gemini AI initialized");
        return true;
    }

    bool sendMessage(const char* message, char* response, size_t maxResponseLen) {
        requestCount++;

        Serial.printf("[GEMINI DEBUG] sendMessage() called (request #%d) with message: %s\n", requestCount, message);
        Serial.printf("[GEMINI DEBUG] enabled: %s, apiKey set: %s, currentState: %d\n",
                     enabled ? "true" : "false",
                     apiKey[0] ? "true" : "false",
                     currentState);

        if (!enabled || !apiKey[0] || currentState != AI_IDLE) {
            strcpy(lastError, "AI not ready");
            errorCount++;
            Serial.printf("[GEMINI DEBUG] AI not ready - enabled: %s, apiKey: %s, state: %d\n",
                         enabled ? "true" : "false",
                         apiKey[0] ? "set" : "not set",
                         currentState);
            return false;
        }

        if (!message || !response || maxResponseLen < 32) {
            strcpy(lastError, "Invalid parameters");
            errorCount++;
            Serial.println("[GEMINI DEBUG] Invalid parameters");
            return false;
        }

        // Check WiFi connectivity
        wl_status_t wifiStatus = WiFi.status();
        Serial.printf("[GEMINI DEBUG] WiFi status: %d (WL_CONNECTED = %d)\n", wifiStatus, WL_CONNECTED);
        if (wifiStatus != WL_CONNECTED) {
            strcpy(lastError, "WiFi not connected");
            errorCount++;
            Serial.println("[GEMINI DEBUG] WiFi not connected");
            return false;
        }

        Serial.println("[GEMINI DEBUG] Setting state to AI_PROCESSING");
        currentState = AI_PROCESSING;

        // Build JSON request
        char jsonBuffer[2048];
        Serial.println("[GEMINI DEBUG] Building JSON request");
        String fullMsg;
        if (hasHistory && lastModelText[0]) {
            fullMsg.reserve(strlen(lastModelText) + strlen(message) + 64);
            fullMsg += "Previous reply for context: ";
            fullMsg += lastModelText;
            fullMsg += "\nCurrent: ";
            fullMsg += message;
        } else {
            fullMsg = String(message);
        }
        fullMsg += "\n\nDo not use markdown or code fences.";
        size_t jsonLen = buildRequestJson(fullMsg.c_str(), jsonBuffer, sizeof(jsonBuffer));
        Serial.printf("[GEMINI DEBUG] JSON built, length: %d\n", jsonLen);
        if (jsonLen == 0) {
            currentState = AI_ERROR;
            strcpy(lastError, "Failed to build request");
            Serial.println("[GEMINI DEBUG] Failed to build JSON request");
            return false;
        }

        Serial.printf("[GEMINI DEBUG] JSON payload: %s\n", jsonBuffer);

        // Make HTTP request
        Serial.println("[GEMINI DEBUG] Making HTTP request");
        bool success = makeHttpRequest(jsonBuffer, response, maxResponseLen);

        if (success) {
            Serial.printf("[GEMINI DEBUG] Success! Response: %s\n", response);
            strlcpy(lastModelText, response, sizeof(lastModelText));
            hasHistory = true;
        } else {
            errorCount++;
            Serial.printf("[GEMINI DEBUG] HTTP request failed: %s\n", lastError);
        }

        // Always return to IDLE to allow retries even after errors
        currentState = AI_IDLE;
        return success;
    }

    void initialize() {
        // AI initialization is handled in main setup()
    }

    bool isEnabled() {
        return enabled;
    }

    bool isReady() {
        return currentState == AI_IDLE;
    }

    AIState getState() {
        return currentState;
    }

    const char* getLastError() {
        return lastError;
    }

    void enable(bool state) {
        enabled = state;
    }

    void clearHistory() {
        hasHistory = false;
        lastModelText[0] = '\0';
    }

    void getUsageStats(uint32_t& totalRequests, uint32_t& failedRequests) {
        totalRequests = requestCount;
        failedRequests = errorCount;
    }

    // Generate SHORT AI prompts for token efficiency
    String generateChatterPrompt() {
        // Get concise robot state
        String state = getRobotStateInfo();

        // Ultra-short prompt with random scenario
        const char* scenarios[] = {
            "React to time", "Comment on mood", "Think about notification",
            "Wonder about owner", "Reflect on purpose", "Imagine human activity",
            "Consider learning", "Think about interaction", "Wonder about AI future", "Reflect on being alive"
        };

        int idx = (int)(esp_random() % (uint32_t)(sizeof(scenarios)/sizeof(scenarios[0])));
        String prompt = "DogePet robot state: " + state + ". " + String(scenarios[idx]) + ". Reply as JSON.";

        return prompt;
    }

    // Get CONCISE robot state information for token efficiency
    String getRobotStateInfo() {
        String state = "";

        // Compact time and mood info
        state += String(chrono.getHourC()) + ":" + String(chrono.getMinute());

        // Mood as single char
        char moodChar;
        switch(curMood) {
            case MS_DEFAULT: moodChar = 'N'; break;  // Neutral
            case MS_HAPPY: moodChar = 'H'; break;    // Happy
            case MS_ANGRY: moodChar = 'A'; break;    // Angry
            case MS_FURIOUS: moodChar = 'F'; break;  // Furious
            case MS_TIRED: moodChar = 'T'; break;    // Tired
            default: moodChar = '?'; break;
        }
        state += " M:" + String(moodChar);

        // Battery as single number
        state += " B:" + String(vbatPercent);

        // Activity status (single char)
        char activityChar = 'C';  // Calm
        if (jiggling) activityChar = 'J';      // Jiggling
        else if (furiousJiggling) activityChar = 'F';  // Furious
        else if (shaking) activityChar = 'S';          // Shaking
        state += " A:" + String(activityChar);

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

        // Procedural SFX sequence (compact format) — only when explicit JSON field present
        {
            String sfxSeq = extractJsonValue(response, "sfx");
            if (sfxSeq.length() > 0) {
                playSfxSequenceInline(sfxSeq.c_str());
            }
        }

        // Extract toast message (preferred order: toast_message, message, thought)
        String toastMessage = extractJsonValue(response, "toast_message");
        if (toastMessage.length() == 0) toastMessage = extractJsonValue(response, "message");
        if (toastMessage.length() == 0) toastMessage = extractJsonValue(response, "text");
        if (toastMessage.length() == 0) toastMessage = extractJsonValue(response, "reply");
        if (toastMessage.length() > 0) toastMessage = sanitizeDisplayText(toastMessage);
        if (toastMessage.length() > 0) {
            showToast(toastMessage, 8000);
            didShowToast = true;
        }

        // Extract thought (display as longer toast if no specific message)
        String thought = extractJsonValue(response, "thought");
        if (thought.length() > 0 && toastMessage.length() == 0) {
            thought = sanitizeDisplayText(thought);
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
                // try generic fields from JSON-like content
                String tryMsg = extractJsonValue(fallback, "message");
                if (tryMsg.length() == 0) tryMsg = extractJsonValue(fallback, "text");
                if (tryMsg.length() == 0) tryMsg = extractJsonValue(fallback, "reply");
                if (tryMsg.length() > 0) fallback = sanitizeDisplayText(tryMsg); else fallback = generateRandomThought();
            }
            // Always sanitize plain-text fallback as well
            fallback = sanitizeDisplayText(fallback);
            if (fallback.length() > 200) fallback = fallback.substring(0, 197) + "...";
            showToast(fallback, 9000);
        }
    }

    // Extract value from JSON-like string (simple parser)
    String extractJsonValue(String json, String key) {
        String searchKey = "\"" + key + "\"";
        int keyIndex = json.indexOf(searchKey);
        if (keyIndex == -1) return "";

        int colon = json.indexOf(':', keyIndex + searchKey.length());
        if (colon == -1) return "";

        // Skip whitespace after colon
        int i = colon + 1;
        while (i < json.length() && (json.charAt(i) == ' ' || json.charAt(i) == '\t' || json.charAt(i) == '\r' || json.charAt(i) == '\n')) i++;
        if (i >= json.length()) return "";

        char c = json.charAt(i);
        // Quoted string value
        if (c == '"') {
            int startQuote = i;
            int endQuote = json.indexOf('"', startQuote + 1);
            // handle simple escaped-quote skipping (best-effort)
            while (endQuote > 0 && json.charAt(endQuote - 1) == '\\') {
                endQuote = json.indexOf('"', endQuote + 1);
            }
            if (endQuote == -1) return "";
            String value = json.substring(startQuote + 1, endQuote);
            value.trim();
            return value;
        }

        // Unquoted token (number, boolean, or bare word) - read until comma or closing brace
        int j = i;
        while (j < json.length()) {
            char ch = json.charAt(j);
            if (ch == ',' || ch == '}' || ch == '\n' || ch == '\r') break;
            j++;
        }
        String value = json.substring(i, j);
        value.trim();
        // Strip trailing commas/spaces
        while (value.endsWith(",")) value = value.substring(0, value.length() - 1);
        value.trim();
        // Remove surrounding quotes if any
        if (value.startsWith("\"") && value.endsWith("\"") && value.length() >= 2) {
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
        if (!AICompanion::isReady()) {
            if (ENABLE_AI_DEBUG_LOGS) Serial.printf("[DEBUG] AI not ready, state: %d\n", AICompanion::getState());
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
        
        Serial.printf("[DEBUG] Checking conditions - GEMINI_AI: %s, AI.isEnabled(): %s, wifiEnabled: %s\n",
                     ENABLE_GEMINI_AI ? "true" : "false",
                     AICompanion::isEnabled() ? "true" : "false",
                     wifiEnabled ? "true" : "false");
        
        if (!ENABLE_GEMINI_AI || !AICompanion::isEnabled() || !wifiEnabled) {
            if (!wifiEnabled) {
                Serial.println("[DEBUG] AI disabled - WiFi is off");
                showToast("AI: WiFi Off", 2000);
            } else if (!ENABLE_GEMINI_AI) {
                Serial.println("[DEBUG] AI disabled - GEMINI_AI config is false");
            } else {
                Serial.println("[DEBUG] AI disabled - AICompanion::isEnabled() is false");
            }
            return;
        }

        Serial.printf("[DEBUG] AICompanion state: %d, isReady(): %s\n",
                     AICompanion::getState(), AICompanion::isReady() ? "true" : "false");

        if (!AICompanion::isReady()) {
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

        Serial.println("[DEBUG] Calling AICompanion::sendMessage()");

        // For voice triggers, use ultra-short prompt for efficiency
        const char* sendPtr = message;
        String wrapped;
        if (isVoiceTrigger) {
            wrapped.reserve(256);
            wrapped += "Say something cute and short with an emoji.";
            sendPtr = wrapped.c_str();
        }
        
        // Send message to Gemini
        if (AICompanion::sendMessage(sendPtr, aiResponse, sizeof(aiResponse))) {
            lastAISendMs = millis();
            Serial.printf("[DEBUG] AI Response received: %s\n", aiResponse);

            if (isBackgroundChatter) {
                Serial.println("[DEBUG] Processing as background chatter - calling executeAIActions()");
                AICompanion::executeAIActions(aiResponse);
            } else if (isVoiceTrigger) {
                Serial.println("[DEBUG] Processing as voice trigger - showing text toast");
                String cleaned = sanitizeDisplayText(String(aiResponse));
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
            Serial.printf("[DEBUG] AI Error occurred: %s\n", AICompanion::getLastError());
            showToast("AI Error! 😅", 3000);
        }
    }
}
