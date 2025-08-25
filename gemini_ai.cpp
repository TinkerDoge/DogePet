// Gemini AI Implementation for DogePet
#include "gemini_ai.h"
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// Global AI instance
GeminiAI DogeAI;

GeminiAI::GeminiAI() {
    currentState = AI_IDLE;
    enabled = false;
    apiKey[0] = '\0';
    lastError[0] = '\0';
}

bool GeminiAI::begin(const char* apiKey) {
    if (!apiKey || strlen(apiKey) >= GEMINI_API_KEY_MAX_LEN) {
        strcpy(lastError, "Invalid API key");
        return false;
    }

    strcpy(this->apiKey, apiKey);
    enabled = true;
    currentState = AI_IDLE;
    strcpy(lastError, "");

    Serial.println("Gemini AI initialized");
    return true;
}

bool GeminiAI::sendMessage(const char* message, char* response, size_t maxResponseLen) {
    Serial.printf("[GEMINI DEBUG] sendMessage() called with message: %s\n", message);
    Serial.printf("[GEMINI DEBUG] enabled: %s, apiKey set: %s, currentState: %d\n",
                 enabled ? "true" : "false", 
                 apiKey[0] ? "true" : "false", 
                 currentState);
    
    if (!enabled || !apiKey[0] || currentState != AI_IDLE) {
        strcpy(lastError, "AI not ready");
        Serial.printf("[GEMINI DEBUG] AI not ready - enabled: %s, apiKey: %s, state: %d\n",
                     enabled ? "true" : "false",
                     apiKey[0] ? "set" : "not set",
                     currentState);
        return false;
    }

    if (!message || !response || maxResponseLen < 32) {
        strcpy(lastError, "Invalid parameters");
        Serial.println("[GEMINI DEBUG] Invalid parameters");
        return false;
    }

    // Check WiFi connectivity
    wl_status_t wifiStatus = WiFi.status();
    Serial.printf("[GEMINI DEBUG] WiFi status: %d (WL_CONNECTED = %d)\n", wifiStatus, WL_CONNECTED);
    if (wifiStatus != WL_CONNECTED) {
        strcpy(lastError, "WiFi not connected");
        Serial.println("[GEMINI DEBUG] WiFi not connected");
        return false;
    }

    Serial.println("[GEMINI DEBUG] Setting state to AI_PROCESSING");
    currentState = AI_PROCESSING;

    // Build JSON request
    char jsonBuffer[1536];
    Serial.println("[GEMINI DEBUG] Building JSON request");
    // Build full prompt as a String to avoid UTF-8 truncation issues
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
        // Store last model text for minimal history
        strlcpy(lastModelText, response, sizeof(lastModelText));
        hasHistory = true;
    } else {
        Serial.printf("[GEMINI DEBUG] HTTP request failed: %s\n", lastError);
    }

    // Always return to IDLE to allow retries even after errors
    currentState = AI_IDLE;
    return success;
}

size_t GeminiAI::buildRequestJson(const char* userMessage, char* buffer, size_t bufferSize) {
    // Use ArduinoJson for memory-efficient JSON building
    StaticJsonDocument<1024> doc;

    // Build request following Generative Language API v1 generate schema.
    // Use a simple text prompt and generation config.
    JsonArray contents = doc.createNestedArray("contents");
    JsonObject entry = contents.createNestedObject();
    entry["role"] = "user";
    JsonArray parts = entry.createNestedArray("parts");
    JsonObject p = parts.createNestedObject();
    p["text"] = userMessage;

    JsonObject gen = doc.createNestedObject("generationConfig");
    gen["maxOutputTokens"] = 250;
    gen["temperature"] = 0.6;

    // safetySettings removed by request (no safety constraints)

    // Nudge model away from markdown by appending a reminder
    // (kept minimal to respect token budget)

    size_t len = serializeJson(doc, buffer, bufferSize);
    Serial.printf("[GEMINI DEBUG] Generated JSON (length: %d): %s\n", len, buffer);
    return len;
}

bool GeminiAI::makeHttpRequest(const char* jsonPayload, char* response, size_t maxResponseLen) {
    Serial.println("[GEMINI DEBUG] Creating WiFiClientSecure");
    WiFiClientSecure client;
    client.setInsecure(); // For testing - use proper certificates in production
    client.setTimeout(15000); // 15s read timeout

    // Build the path part only (without https://hostname)
    char path[GEMINI_MAX_URL_LEN];
    Serial.printf("[GEMINI DEBUG] Building path with model: %s, endpoint: %s, API key length: %d\n", 
                 GEMINI_MODEL, GEMINI_API_ENDPOINT, strlen(apiKey));
    // Prefer v1beta endpoint path for broader model support
    int pathLen = snprintf(path, sizeof(path), "/v1beta/models/%s%s?key=%s", GEMINI_MODEL, GEMINI_API_ENDPOINT, apiKey);
    Serial.printf("[GEMINI DEBUG] Path length: %d, buffer size: %d\n", pathLen, sizeof(path));
    Serial.printf("[GEMINI DEBUG] Constructed path: %s\n", path);
    
    if (pathLen >= sizeof(path)) {
        Serial.println("[GEMINI DEBUG] WARNING: Path was truncated!");
        strcpy(lastError, "URL too long");
        return false;
    }

    // Connect to server
    Serial.println("[GEMINI DEBUG] Attempting to connect to generativelanguage.googleapis.com:443");
    if (!client.connect("generativelanguage.googleapis.com", 443)) {
        strcpy(lastError, "Connection failed");
        Serial.println("[GEMINI DEBUG] Connection failed!");
        return false;
    }
    Serial.println("[GEMINI DEBUG] Connected successfully!");

    // Send HTTP POST request
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

    // Read response fully (guard against closed socket)
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

    client.stop();
    Serial.printf("[GEMINI DEBUG] Response length: %d\n", responseString.length());

    if (responseString.length() == 0) {
        strcpy(lastError, "Empty response");
        Serial.println("[GEMINI DEBUG] Empty response received");
        return false;
    }

    // Try to locate JSON body: find first '{'
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

bool GeminiAI::extractResponseText(const char* jsonResponse, char* output, size_t maxOutputLen) {
    StaticJsonDocument<4096> doc;

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

    // Several possible response shapes:
    // 1) candidates[0].content.parts[0].text (v1beta)
    // 2) outputs[0].content[0].text (some v1 variants)
    // 3) outputs[0].text
    // 4) candidates[0].output
    // 5) text field directly
    const char* responseText = nullptr;

    // Preferred v1beta structure
    if (doc["candidates"][0]["content"]["parts"][0]["text"]) {
        responseText = doc["candidates"][0]["content"]["parts"][0]["text"];
    }

    if (!responseText && doc.containsKey("outputs") && doc["outputs"].size() > 0) {
        // outputs is an array; look for content/text
        if (doc["outputs"][0].containsKey("content") && doc["outputs"][0]["content"].size() > 0) {
            responseText = doc["outputs"][0]["content"][0]["text"] | nullptr;
        }
        if (!responseText && doc["outputs"][0].containsKey("text")) {
            responseText = doc["outputs"][0]["text"] | nullptr;
        }
    }

    if (!responseText && doc.containsKey("candidates") && doc["candidates"].size() > 0) {
        responseText = doc["candidates"][0]["output"] | nullptr;
    }

    if (!responseText && doc.containsKey("text")) {
        responseText = doc["text"] | nullptr;
    }

    if (!responseText) {
        strcpy(lastError, "No response text found");
        return false;
    }

    // Copy response, ensuring null termination
    size_t len = strlen(responseText);
    if (len >= maxOutputLen) {
        len = maxOutputLen - 1;
    }

    memcpy(output, responseText, len);
    output[len] = '\0';

    // Clean up response (remove extra whitespace)
    char* clean = output;
    while (*clean) {
        if (*clean == '\n' || *clean == '\r') {
            *clean = ' ';
        }
        clean++;
    }

    return true;
}
