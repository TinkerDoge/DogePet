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
    char jsonBuffer[1024];
    Serial.println("[GEMINI DEBUG] Building JSON request");
    size_t jsonLen = buildRequestJson(message, jsonBuffer, sizeof(jsonBuffer));
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
        currentState = AI_IDLE;
        Serial.printf("[GEMINI DEBUG] Success! Response: %s\n", response);
    } else {
        currentState = AI_ERROR;
        Serial.printf("[GEMINI DEBUG] HTTP request failed: %s\n", lastError);
    }

    return success;
}

size_t GeminiAI::buildRequestJson(const char* userMessage, char* buffer, size_t bufferSize) {
    // Use ArduinoJson for memory-efficient JSON building
    StaticJsonDocument<1024> doc;

    // Simplified structure - try without system instruction first
    doc["contents"][0]["parts"][0]["text"] = userMessage;
    doc["contents"][0]["role"] = "user";

    // Add generation config for shorter responses
    doc["generationConfig"]["maxOutputTokens"] = 100;
    doc["generationConfig"]["temperature"] = 0.8;

    size_t len = serializeJson(doc, buffer, bufferSize);
    Serial.printf("[GEMINI DEBUG] Generated JSON (length: %d): %s\n", len, buffer);
    return len;
}

bool GeminiAI::makeHttpRequest(const char* jsonPayload, char* response, size_t maxResponseLen) {
    Serial.println("[GEMINI DEBUG] Creating WiFiClientSecure");
    WiFiClientSecure client;
    client.setInsecure(); // For testing - use proper certificates in production

    // Build the path part only (without https://hostname)
    char path[GEMINI_MAX_URL_LEN];
    Serial.printf("[GEMINI DEBUG] Building path with model: %s, endpoint: %s, API key length: %d\n", 
                 GEMINI_MODEL, GEMINI_API_ENDPOINT, strlen(apiKey));
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

    // Read response
    String responseString = "";
    bool inBody = false;
    int lineCount = 0;

    while (client.available()) {
        String line = client.readStringUntil('\n');
        lineCount++;
        
        // Log first few lines to see headers
        if (lineCount <= 10) {
            Serial.printf("[GEMINI DEBUG] Line %d: %s\n", lineCount, line.c_str());
        }

        if (line.startsWith("{")) {
            inBody = true;
            Serial.println("[GEMINI DEBUG] Found JSON start, beginning body parse");
        }

        if (inBody) {
            responseString += line;
            responseString += "\n";
        }
    }

    client.stop();
    Serial.printf("[GEMINI DEBUG] Response length: %d\n", responseString.length());

    if (responseString.length() == 0) {
        strcpy(lastError, "Empty response");
        Serial.println("[GEMINI DEBUG] Empty response received");
        return false;
    }

    Serial.printf("[GEMINI DEBUG] Raw response: %s\n", responseString.c_str());

    // Parse JSON response
    Serial.println("[GEMINI DEBUG] Parsing JSON response");
    return extractResponseText(responseString.c_str(), response, maxResponseLen);
}

bool GeminiAI::extractResponseText(const char* jsonResponse, char* output, size_t maxOutputLen) {
    StaticJsonDocument<2048> doc;

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

    // Extract response text
    const char* responseText = nullptr;

    // Try different response formats
    if (doc["candidates"][0]["content"]["parts"][0]["text"]) {
        responseText = doc["candidates"][0]["content"]["parts"][0]["text"];
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
