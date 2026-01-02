// DogePet Serial Command Implementation
// JSON-based protocol for PC companion app communication

#include "include/serial_cmd.h"
#include "FluxGarage_RoboEyes.h"
#include <ArduinoJson.h>

// =============================================================================
// GLOBALS
// =============================================================================
static roboEyes* pEyes = nullptr;
static String serialBuffer = "";

// Current settings (runtime state)
static struct {
    uint8_t eyeWidth = 28;
    uint8_t eyeHeight = 40;
    uint8_t borderRadius = 8;
    int8_t spacing = 10;
    uint8_t mood = 0;       // 0=Default, 1=Tired, 2=Angry, 3=Happy
    uint8_t position = 0;   // 0=Center, 1-8=N,NE,E,SE,S,SW,W,NW
    bool autoBlink = true;
    bool idleMode = true;
    bool cyclops = false;
    bool curiosity = false;
    bool sweat = false;
    uint8_t blinkInterval = 3;
    uint8_t blinkVariation = 4;
    uint8_t idleInterval = 4;
    uint8_t idleVariation = 5;
} eyeSettings;

// =============================================================================
// APPLY SETTINGS TO ROBOEYES
// =============================================================================
static void applyEyeSettings() {
    if (!pEyes) return;
    
    // Geometry
    pEyes->setWidth(eyeSettings.eyeWidth, eyeSettings.eyeWidth);
    pEyes->setHeight(eyeSettings.eyeHeight, eyeSettings.eyeHeight);
    pEyes->setBorderradius(eyeSettings.borderRadius, eyeSettings.borderRadius);
    pEyes->setSpacebetween(eyeSettings.spacing);
    
    // Mood
    pEyes->setMood(eyeSettings.mood);
    
    // Position (only set if not center)
    if (eyeSettings.position > 0) {
        pEyes->setPosition(eyeSettings.position);
    }
    
    // Animations
    pEyes->setAutoblinker(eyeSettings.autoBlink, eyeSettings.blinkInterval, eyeSettings.blinkVariation);
    pEyes->setIdleMode(eyeSettings.idleMode, eyeSettings.idleInterval, eyeSettings.idleVariation);
    pEyes->setCyclops(eyeSettings.cyclops);
    pEyes->setCuriosity(eyeSettings.curiosity);
    pEyes->setSweat(eyeSettings.sweat);
}

// =============================================================================
// COMMAND HANDLERS
// =============================================================================

// Handle set_eyes command
static void handleSetEyes(JsonObject obj) {
    // Update settings from JSON - ArduinoJson 7.x syntax
    if (obj["width"].is<int>()) eyeSettings.eyeWidth = obj["width"].as<int>();
    if (obj["height"].is<int>()) eyeSettings.eyeHeight = obj["height"].as<int>();
    if (obj["radius"].is<int>()) eyeSettings.borderRadius = obj["radius"].as<int>();
    if (obj["spacing"].is<int>()) eyeSettings.spacing = obj["spacing"].as<int>();
    if (obj["mood"].is<int>()) eyeSettings.mood = obj["mood"].as<int>();
    if (obj["position"].is<int>()) eyeSettings.position = obj["position"].as<int>();
    if (obj["autoBlink"].is<bool>()) eyeSettings.autoBlink = obj["autoBlink"].as<bool>();
    if (obj["idleMode"].is<bool>()) eyeSettings.idleMode = obj["idleMode"].as<bool>();
    if (obj["cyclops"].is<bool>()) eyeSettings.cyclops = obj["cyclops"].as<bool>();
    if (obj["curiosity"].is<bool>()) eyeSettings.curiosity = obj["curiosity"].as<bool>();
    if (obj["sweat"].is<bool>()) eyeSettings.sweat = obj["sweat"].as<bool>();
    
    // Apply to RoboEyes
    applyEyeSettings();
    
    Serial.println("{\"status\":\"ok\"}");
}

// Handle get_settings command
static void handleGetSettings() {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["status"] = "ok";
    obj["width"] = eyeSettings.eyeWidth;
    obj["height"] = eyeSettings.eyeHeight;
    obj["radius"] = eyeSettings.borderRadius;
    obj["spacing"] = eyeSettings.spacing;
    obj["mood"] = eyeSettings.mood;
    obj["position"] = eyeSettings.position;
    obj["autoBlink"] = eyeSettings.autoBlink;
    obj["idleMode"] = eyeSettings.idleMode;
    obj["cyclops"] = eyeSettings.cyclops;
    obj["curiosity"] = eyeSettings.curiosity;
    obj["sweat"] = eyeSettings.sweat;
    
    String response;
    serializeJson(doc, response);
    Serial.println(response);
}

// Handle action command
static void handleAction(JsonObject obj) {
    if (!obj["type"].is<const char*>()) {
        Serial.println("{\"status\":\"error\",\"msg\":\"Missing action type\"}");
        return;
    }
    
    const char* actionType = obj["type"].as<const char*>();
    
    if (pEyes) {
        if (strcmp(actionType, "blink") == 0) {
            pEyes->blink();
        } else if (strcmp(actionType, "laugh") == 0) {
            pEyes->anim_laugh();
        } else if (strcmp(actionType, "confused") == 0) {
            pEyes->anim_confused();
        } else if (strcmp(actionType, "close") == 0) {
            pEyes->close();
        } else if (strcmp(actionType, "open") == 0) {
            pEyes->open();
        } else {
            Serial.println("{\"status\":\"error\",\"msg\":\"Unknown action\"}");
            return;
        }
    }
    
    Serial.println("{\"status\":\"ok\"}");
}

// Process a complete JSON command
static void processCommand(const String& cmd) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, cmd);
    
    if (err) {
        Serial.println("{\"status\":\"error\",\"msg\":\"Invalid JSON\"}");
        return;
    }
    
    JsonObject obj = doc.as<JsonObject>();
    
    if (!obj["cmd"].is<const char*>()) {
        Serial.println("{\"status\":\"error\",\"msg\":\"Missing cmd field\"}");
        return;
    }
    
    const char* cmdType = obj["cmd"].as<const char*>();
    
    if (strcmp(cmdType, CMD_SET_EYES) == 0) {
        handleSetEyes(obj);
    } else if (strcmp(cmdType, CMD_GET_SETTINGS) == 0) {
        handleGetSettings();
    } else if (strcmp(cmdType, CMD_ACTION) == 0) {
        handleAction(obj);
    } else {
        Serial.println("{\"status\":\"error\",\"msg\":\"Unknown command\"}");
    }
}

// =============================================================================
// PUBLIC FUNCTIONS
// =============================================================================

void setupSerialCmd(roboEyes* eyesPtr) {
    pEyes = eyesPtr;
    serialBuffer.reserve(512);  // Pre-allocate buffer
    Serial.println("{\"status\":\"ready\",\"device\":\"DogePet\"}");
}

void processSerialCmd() {
    while (Serial.available()) {
        char c = Serial.read();
        
        if (c == '\n' || c == '\r') {
            // End of command
            if (serialBuffer.length() > 0) {
                processCommand(serialBuffer);
                serialBuffer = "";
            }
        } else {
            serialBuffer += c;
            
            // Prevent buffer overflow
            if (serialBuffer.length() > 500) {
                serialBuffer = "";
                Serial.println("{\"status\":\"error\",\"msg\":\"Command too long\"}");
            }
        }
    }
}
