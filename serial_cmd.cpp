// DogePet Serial Command Implementation
// JSON-based protocol for PC companion app communication

#include "include/serial_cmd.h"
#include "FluxGarage_RoboEyes.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include "config.h"

// =============================================================================
// GLOBALS
// =============================================================================
static roboEyes* pEyes = nullptr;
static String serialBuffer = "";
static Preferences prefs;

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
// PERSISTENCE HELPER
// =============================================================================
static void saveSettings() {
    prefs.begin("dogepet", false); // Read/Write
    prefs.putUChar("width", eyeSettings.eyeWidth);
    prefs.putUChar("height", eyeSettings.eyeHeight);
    prefs.putUChar("radius", eyeSettings.borderRadius);
    prefs.putChar("spacing", eyeSettings.spacing);
    prefs.putUChar("mood", eyeSettings.mood);
    prefs.putUChar("pos", eyeSettings.position);
    prefs.putBool("blink", eyeSettings.autoBlink);
    prefs.putBool("idle", eyeSettings.idleMode);
    prefs.putBool("cyclops", eyeSettings.cyclops);
    prefs.putBool("curio", eyeSettings.curiosity);
    prefs.putBool("sweat", eyeSettings.sweat);
    prefs.end();
}

static void loadSettings() {
    prefs.begin("dogepet", true); // Read Only
    eyeSettings.eyeWidth = prefs.getUChar("width", 28);
    eyeSettings.eyeHeight = prefs.getUChar("height", 40);
    eyeSettings.borderRadius = prefs.getUChar("radius", 8);
    eyeSettings.spacing = prefs.getChar("spacing", 10);
    eyeSettings.mood = prefs.getUChar("mood", 0);
    eyeSettings.position = prefs.getUChar("pos", 0);
    eyeSettings.autoBlink = prefs.getBool("blink", true);
    eyeSettings.idleMode = prefs.getBool("idle", true);
    eyeSettings.cyclops = prefs.getBool("cyclops", false);
    eyeSettings.curiosity = prefs.getBool("curio", false);
    eyeSettings.sweat = prefs.getBool("sweat", false);
    prefs.end();
}

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
// EYE COMMAND HANDLERS
// =============================================================================

// Handle connect command (handshake)
static void handleConnect() {
    Serial.println("{\"status\":\"ok\",\"msg\":\"DogePet Protected Mode Active\"}");
}

// Handle set_eyes command
static void handleSetEyes(JsonObject obj) {
    // Update eye settings from JSON
    if (obj.containsKey("width")) eyeSettings.eyeWidth = obj["width"].as<uint8_t>();
    if (obj.containsKey("height")) eyeSettings.eyeHeight = obj["height"].as<uint8_t>();
    if (obj.containsKey("radius")) eyeSettings.borderRadius = obj["radius"].as<uint8_t>();
    if (obj.containsKey("spacing")) eyeSettings.spacing = obj["spacing"].as<int8_t>();
    if (obj.containsKey("mood")) eyeSettings.mood = obj["mood"].as<uint8_t>();
    if (obj.containsKey("position")) eyeSettings.position = obj["position"].as<uint8_t>();
    if (obj.containsKey("autoBlink")) eyeSettings.autoBlink = obj["autoBlink"].as<bool>();
    if (obj.containsKey("idleMode")) eyeSettings.idleMode = obj["idleMode"].as<bool>();
    if (obj.containsKey("cyclops")) eyeSettings.cyclops = obj["cyclops"].as<bool>();
    if (obj.containsKey("curiosity")) eyeSettings.curiosity = obj["curiosity"].as<bool>();
    if (obj.containsKey("sweat")) eyeSettings.sweat = obj["sweat"].as<bool>();
    
    // Apply changes to RoboEyes
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

// =============================================================================
// HARDWARE CONFIGURATION
// =============================================================================

// Hardware Configuration
static HardwareConfig hwConfig = {
    .i2c_sda = I2C_SDA,
    .i2c_scl = I2C_SCL,
    .func_btn = FUNC_BTN,
    .led_pin = LED_PIN,
    .vibro_left = VIBRO_LEFT,
    .vibro_right = VIBRO_RIGHT,
    .vbat_pin = VBAT_PIN,
    .i2s_do = I2S_DO,
    .i2s_bclk = I2S_BCLK,
    .i2s_lrc = I2S_LRC,
    .mic_sd = 0,
    .mic_ws = 0,
    .mic_sck = 0
};

// =============================================================================
// HARDWARE CONFIG IMPLEMENTATION
// =============================================================================
// Load hardware config from NVS
void loadHardwareConfig() {
    prefs.begin("dogepet_hw", true);
    hwConfig.i2c_sda = prefs.getInt("sda", I2C_SDA);
    hwConfig.i2c_scl = prefs.getInt("scl", I2C_SCL);
    hwConfig.func_btn = prefs.getInt("btn", FUNC_BTN);
    hwConfig.led_pin = prefs.getInt("led", LED_PIN);
    hwConfig.vibro_left = prefs.getInt("vibL", VIBRO_LEFT);
    hwConfig.vibro_right = prefs.getInt("vibR", VIBRO_RIGHT);
    hwConfig.vbat_pin = prefs.getInt("vbat", VBAT_PIN);
    // Audio defaults
    hwConfig.i2s_do = prefs.getInt("i2s_do", 26);
    hwConfig.i2s_bclk = prefs.getInt("i2s_bclk", 27);
    hwConfig.i2s_lrc = prefs.getInt("i2s_lrc", 25);
    hwConfig.mic_sd = prefs.getInt("mic_sd", 32);
    hwConfig.mic_ws = prefs.getInt("mic_ws", 33);
    hwConfig.mic_sck = prefs.getInt("mic_sck", 25); // often shared
    prefs.end();
}

static void saveHardwareConfig() {
    prefs.begin("dogepet_hw", false);
    prefs.putInt("sda", hwConfig.i2c_sda);
    prefs.putInt("scl", hwConfig.i2c_scl);
    prefs.putInt("btn", hwConfig.func_btn);
    prefs.putInt("led", hwConfig.led_pin);
    prefs.putInt("vibL", hwConfig.vibro_left);
    prefs.putInt("vibR", hwConfig.vibro_right);
    prefs.putInt("vbat", hwConfig.vbat_pin);
    prefs.putInt("i2s_do", hwConfig.i2s_do);
    prefs.putInt("i2s_bclk", hwConfig.i2s_bclk);
    prefs.putInt("i2s_lrc", hwConfig.i2s_lrc);
    prefs.putInt("mic_sd", hwConfig.mic_sd);
    prefs.putInt("mic_ws", hwConfig.mic_ws);
    prefs.putInt("mic_sck", hwConfig.mic_sck);
    prefs.end();
}

HardwareConfig* getHardwareConfig() {
    return &hwConfig;
}

// =============================================================================
// TESTING HELPERS
// =============================================================================
static void testLED(bool on) {
    pinMode(hwConfig.led_pin, OUTPUT);
    digitalWrite(hwConfig.led_pin, on ? HIGH : LOW);
}

static void testVibro(uint8_t motor, bool on) {
    int pin = (motor == 0) ? hwConfig.vibro_left : hwConfig.vibro_right;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, on ? HIGH : LOW);
}

static void testTone(bool on) {
    // Simple square wave on I2S_DO pin for testing connectivity
    // Note: Proper I2S requires driver, this is a "buzz" test
    if (on) {
        tone(hwConfig.i2s_do, 440); 
    } else {
        noTone(hwConfig.i2s_do);
        pinMode(hwConfig.i2s_do, INPUT); // Reset to high-Z
    }
}

// =============================================================================
// SENSOR HELPERS
// =============================================================================
static float readBattery() {
    // Simple average
    long sum = 0;
    for(int i=0; i<10; i++) {
        sum += analogRead(hwConfig.vbat_pin);
        delay(1);
    }
    float raw = sum / 10.0;
    // Voltage divider calculation (3.3V ref, 12-bit ADC)
    // Formula from HARDWARE.md: ADC_raw * (3.3 / 4095) * 2 * 1.0518
    return raw * (3.3 / 4095.0) * 2.0 * 1.0518;
}

// =============================================================================
// COMMAND HANDLERS
// =============================================================================

// Handle set_pinout command
static void handleSetPinout(JsonObject obj) {
    if (obj.containsKey("sda")) hwConfig.i2c_sda = obj["sda"].as<int>();
    if (obj.containsKey("scl")) hwConfig.i2c_scl = obj["scl"].as<int>();
    if (obj.containsKey("btn")) hwConfig.func_btn = obj["btn"].as<int>();
    if (obj.containsKey("led")) hwConfig.led_pin = obj["led"].as<int>();
    // Extended pinout for phase 4/5
    if (obj.containsKey("vibL")) hwConfig.vibro_left = obj["vibL"].as<int>();
    if (obj.containsKey("vibR")) hwConfig.vibro_right = obj["vibR"].as<int>();
    if (obj.containsKey("vbat")) hwConfig.vbat_pin = obj["vbat"].as<int>();
    
    if (obj.containsKey("mic_sd")) hwConfig.mic_sd = obj["mic_sd"].as<int>();
    if (obj.containsKey("mic_ws")) hwConfig.mic_ws = obj["mic_ws"].as<int>();
    if (obj.containsKey("mic_sck")) hwConfig.mic_sck = obj["mic_sck"].as<int>();
    
    if (obj.containsKey("i2s_do")) hwConfig.i2s_do = obj["i2s_do"].as<int>();
    if (obj.containsKey("i2s_bclk")) hwConfig.i2s_bclk = obj["i2s_bclk"].as<int>();
    if (obj.containsKey("i2s_lrc")) hwConfig.i2s_lrc = obj["i2s_lrc"].as<int>();
    
    saveHardwareConfig();
    Serial.println("{\"status\":\"ok\",\"msg\":\"Pinout saved. Restart required.\"}");
}

// Handle get_pinout command
static void handleGetPinout() {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["status"] = "ok";
    obj["sda"] = hwConfig.i2c_sda;
    obj["scl"] = hwConfig.i2c_scl;
    obj["btn"] = hwConfig.func_btn;
    obj["led"] = hwConfig.led_pin;
    obj["vibL"] = hwConfig.vibro_left;
    obj["vibR"] = hwConfig.vibro_right;
    obj["vbat"] = hwConfig.vbat_pin;
    obj["mic_sd"] = hwConfig.mic_sd;
    obj["mic_ws"] = hwConfig.mic_ws;
    obj["mic_sck"] = hwConfig.mic_sck;
    obj["i2s_do"] = hwConfig.i2s_do;
    obj["i2s_bclk"] = hwConfig.i2s_bclk;
    obj["i2s_lrc"] = hwConfig.i2s_lrc;
    
    String response;
    serializeJson(doc, response);
    Serial.println(response);
}

// Handle get_sensors command
static void handleGetSensors() {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    obj["status"] = "ok";
    obj["vbat"] = readBattery();
    // Use simulated data for now until drivers are connected
    obj["mic_db"] = random(30, 80); // Placeholder
    obj["ax"] = random(-100, 100) / 100.0;
    obj["ay"] = random(-100, 100) / 100.0;
    obj["az"] = 0.98 + (random(-10, 10) / 100.0);
    
    String response;
    serializeJson(doc, response);
    Serial.println(response);
}

// Handle action command
static void handleAction(JsonObject obj) {
    if (!obj.containsKey("type")) {
        Serial.println("{\"status\":\"error\",\"msg\":\"Missing action type\"}");
        return;
    }
    
    const char* actionType = obj["type"].as<const char*>();
    
    if (strcmp(actionType, "test_led_on") == 0) {
        testLED(true);
    } else if (strcmp(actionType, "test_led_off") == 0) {
        testLED(false);
    } else if (strcmp(actionType, "test_vib_l_on") == 0) {
        testVibro(0, true);
    } else if (strcmp(actionType, "test_vib_l_off") == 0) {
        testVibro(0, false);
    } else if (strcmp(actionType, "test_vib_r_on") == 0) {
        testVibro(1, true);
    } else if (strcmp(actionType, "test_vib_r_off") == 0) {
        testVibro(1, false);
    } else if (strcmp(actionType, "test_tone_on") == 0) {
        testTone(true);
    } else if (strcmp(actionType, "test_tone_off") == 0) {
        testTone(false);
    } else if (strcmp(actionType, "test_oled_pattern") == 0) {
        if(pEyes) {
           Serial.println("{\"status\":\"ok\",\"msg\":\"OLED Pattern Test\"}");
        }
    } else if (strcmp(actionType, "test_oled_clear") == 0) {
        if(pEyes) {
           pEyes->close();
        }
    } else if (strcmp(actionType, "test_oled_text") == 0) {
        // "value": "Hello World"
        const char* txt = obj["value"] | "TEST";
        // Draw text to display if possible. 
        // For RoboEyes, maybe stick to Serial log or if we have direct GFX access:
        // display.setCursor(0,0); display.print(txt); display.display();
        // Since RoboEyes manages the display, we might conflict. 
        // Just log for now as proof of concept.
        Serial.print("{\"status\":\"ok\",\"msg\":\"OLED Text: ");
        Serial.print(txt);
        Serial.println("\"}");
        
    } else if (strcmp(actionType, "test_tone_custom") == 0) {
        // "value": "1000,500" (freq,dur)
        String val = obj["value"].as<String>();
        int commaValues = val.indexOf(',');
        if(commaValues != -1) {
            int freq = val.substring(0, commaValues).toInt();
            int dur = val.substring(commaValues+1).toInt();
            tone(hwConfig.i2s_do, freq, dur); // Use simple tone() or I2S generic
            // Note: ESP32 standard tone() uses ledc. 
        }
        
    } else if (strcmp(actionType, "test_vib_seq1") == 0) {
        // Heartbeat
        testVibro(0, true); delay(100); testVibro(0, false);
        delay(100);
        testVibro(0, true); delay(100); testVibro(0, false);
        
    } else if (strcmp(actionType, "test_vib_seq2") == 0) {
        // Alarm
        for(int i=0; i<3; i++) {
            testVibro(1, true); delay(200); testVibro(1, false); delay(100);
        }
    
    } else if (strcmp(actionType, "test_mic_read") == 0) {
         Serial.println("{\"status\":\"ok\",\"msg\":\"Mic Sample: ...\"}");
    } else if (strcmp(actionType, "test_mpu_read") == 0) {
         Serial.println("{\"status\":\"ok\",\"msg\":\"MPU Sample: ...\"}");
    } else if (pEyes) {
        // Delegate eye actions
        if (strcmp(actionType, "blink") == 0) pEyes->blink();
        else if (strcmp(actionType, "laugh") == 0) pEyes->anim_laugh();
        else if (strcmp(actionType, "confused") == 0) pEyes->anim_confused();
        else if (strcmp(actionType, "close") == 0) pEyes->close();
        else if (strcmp(actionType, "open") == 0) pEyes->open();
        else if (strcmp(actionType, "save_restart") == 0) {
            Serial.println("{\"status\":\"ok\",\"msg\":\"Saving and Restarting...\"}");
            saveSettings();
            delay(500);
            ESP.restart();
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
    
    if (!obj.containsKey("cmd")) {
        Serial.println("{\"status\":\"error\",\"msg\":\"Missing cmd field\"}");
        return;
    }
    
    const char* cmdType = obj["cmd"].as<const char*>();
    
    if (strcmp(cmdType, CMD_CONNECT) == 0) {
        handleConnect();
    } else if (strcmp(cmdType, CMD_SET_EYES) == 0) {
        handleSetEyes(obj);
    } else if (strcmp(cmdType, CMD_GET_SETTINGS) == 0) {
        handleGetSettings();
    } else if (strcmp(cmdType, CMD_ACTION) == 0) {
        handleAction(obj);
    } else if (strcmp(cmdType, CMD_SET_PINOUT) == 0) {
        handleSetPinout(obj);
    } else if (strcmp(cmdType, CMD_GET_PINOUT) == 0) {
        handleGetPinout();
    } else if (strcmp(cmdType, CMD_GET_SENSORS) == 0) {
        handleGetSensors();
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
    
    // Load persisted settings
    loadSettings();
    applyEyeSettings();
    
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
