// DogePet Serial Command Implementation
// JSON-based protocol for PC companion app communication

#include "include/serial_cmd.h"
#include "include/FluxGarage_RoboEyes.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include "include/config.h"
#include <MPU6050.h>

// =============================================================================
// GLOBALS
// =============================================================================
static roboEyes* pEyes = nullptr;
static String serialBuffer = "";
static Preferences prefs;
static MPU6050 mpu;
static bool mpuInitialized = false;

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
// Note: MIC and AMP share I2S clock pins per HARDWARE.md
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
    .mic_sd = I2S_DI,       // GPIO 11 - mic data input
    .mic_ws = I2S_LRC,      // GPIO 16 - shared with audio
    .mic_sck = I2S_BCLK     // GPIO 17 - shared with audio
};

// =============================================================================
// HARDWARE CONFIG IMPLEMENTATION
// =============================================================================

// Check if NVS has valid data or needs reset
static bool isNVSValid() {
    prefs.begin("dogepet_hw", true);
    int sda = prefs.getInt("sda", -999);
    prefs.end();
    // If we get default -999, NVS has never been written
    // If we get 255 or other invalid values, NVS is corrupted
    return (sda != -999 && sda >= 0 && sda <= 48);
}

// Reset NVS to factory defaults
static void resetHardwareConfig() {
    prefs.begin("dogepet_hw", false);
    prefs.clear();  // Clear all keys
    prefs.end();
    
    // Reinitialize with defaults from config.h
    hwConfig.i2c_sda = I2C_SDA;
    hwConfig.i2c_scl = I2C_SCL;
    hwConfig.func_btn = FUNC_BTN;
    hwConfig.led_pin = LED_PIN;
    hwConfig.vibro_left = VIBRO_LEFT;
    hwConfig.vibro_right = VIBRO_RIGHT;
    hwConfig.vbat_pin = VBAT_PIN;
    hwConfig.i2s_do = I2S_DO;
    hwConfig.i2s_bclk = I2S_BCLK;
    hwConfig.i2s_lrc = I2S_LRC;
    hwConfig.mic_sd = I2S_DI;
    hwConfig.mic_ws = I2S_LRC;
    hwConfig.mic_sck = I2S_BCLK;
    
    // Save defaults
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
    
    Serial.println("{\"status\":\"info\",\"msg\":\"NVS reset to factory defaults\"}");
}

// Load hardware config from NVS
void loadHardwareConfig() {
    // Check if NVS is valid, reset if needed
    if (!isNVSValid()) {
        Serial.println("{\"status\":\"warning\",\"msg\":\"Invalid NVS detected, resetting to defaults\"}");
        resetHardwareConfig();
        return;
    }
    prefs.begin("dogepet_hw", true);
    
    // Load and validate pins (ESP32-S3 valid GPIO: 0-48, but some reserved)
    auto loadPin = [](const char* key, int defaultVal) -> int {
        int pin = prefs.getInt(key, defaultVal);
        // Validate: must be 0-48, not 255 or negative
        if (pin < 0 || pin > 48) {
            Serial.printf("{\"warning\":\"Invalid pin %s=%d, using default %d\"}\n", key, pin, defaultVal);
            return defaultVal;
        }
        return pin;
    };
    
    hwConfig.i2c_sda = loadPin("sda", I2C_SDA);
    hwConfig.i2c_scl = loadPin("scl", I2C_SCL);
    hwConfig.func_btn = loadPin("btn", FUNC_BTN);
    hwConfig.led_pin = loadPin("led", LED_PIN);
    hwConfig.vibro_left = loadPin("vibL", VIBRO_LEFT);
    hwConfig.vibro_right = loadPin("vibR", VIBRO_RIGHT);
    hwConfig.vbat_pin = loadPin("vbat", VBAT_PIN);
    hwConfig.i2s_do = loadPin("i2s_do", I2S_DO);
    hwConfig.i2s_bclk = loadPin("i2s_bclk", I2S_BCLK);
    hwConfig.i2s_lrc = loadPin("i2s_lrc", I2S_LRC);
    hwConfig.mic_sd = loadPin("mic_sd", I2S_DI);
    hwConfig.mic_ws = loadPin("mic_ws", I2S_LRC);
    hwConfig.mic_sck = loadPin("mic_sck", I2S_BCLK);
    
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
    if (pin <= 0 || pin > 48) return;  // Skip invalid pins
    pinMode(pin, OUTPUT);
    digitalWrite(pin, on ? HIGH : LOW);
}

// LEDC channel for tone generation (ESP32 uses LEDC for tone())
#define TONE_LEDC_CHANNEL 0
static bool toneActive = false;

static void testTone(bool on) {
    // ESP32-S3: Use LEDC for simple tone generation on I2S_DO pin
    // Note: This is a connectivity test, not proper I2S audio
    int pin = hwConfig.i2s_do;
    if (pin <= 0 || pin > 48) {
        Serial.println("{\"status\":\"error\",\"msg\":\"Invalid audio pin\"}");
        return;
    }
    
    if (on) {
        // Setup LEDC for 440Hz tone
        ledcAttach(pin, 440, 8);  // pin, freq, resolution
        ledcWrite(pin, 127);       // 50% duty cycle
        toneActive = true;
    } else {
        if (toneActive) {
            ledcDetach(pin);
            pinMode(pin, INPUT);  // High-Z
            toneActive = false;
        }
    }
}

// =============================================================================
// SENSOR HELPERS
// =============================================================================
static float readBatteryVoltage() {
    // Validate ADC pin
    if (hwConfig.vbat_pin < 0 || hwConfig.vbat_pin > 48) {
        return 0.0f;  // Invalid pin
    }
    
    // Simple average
    long sum = 0;
    for(int i=0; i<10; i++) {
        sum += analogRead(hwConfig.vbat_pin);
        delay(1);
    }
    float raw = sum / 10.0;
    // Voltage divider calculation (3.3V ref, 12-bit ADC)
    // Formula from HARDWARE.md: ADC_raw * (3.3 / 4095) * 2 * VBAT_CAL
    return raw * (3.3 / 4095.0) * 2.0 * VBAT_CAL;
}

static int readBatteryPercent(float voltage) {
    // Li-Po discharge curve approximation (3.2V=0%, 4.05V=100%)
    float pct = (voltage - VBAT_MIN_V) / (VBAT_MAX_V - VBAT_MIN_V) * 100.0;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return (int)pct;
}

static bool ensureMPUReady() {
    if (mpuInitialized) return true;
    
    // MPU6050 needs Wire to be initialized first
    // Wire is initialized in main setup() before setupSerialCmd()
    mpu.initialize();
    delay(10);  // Give MPU time to settle
    
    if (mpu.testConnection()) {
        mpuInitialized = true;
        Serial.println("{\"status\":\"info\",\"msg\":\"MPU6050 initialized\"}" );
        return true;
    } else {
        Serial.println("{\"status\":\"warning\",\"msg\":\"MPU6050 not detected on I2C bus\"}" );
    }
    return false;
}

static void readMPUSample(float& ax, float& ay, float& az) {
    if (!ensureMPUReady()) {
        ax = ay = 0.0f;
        az = 0.0f;
        return;
    }
    int16_t rawAx, rawAy, rawAz;
    mpu.getAcceleration(&rawAx, &rawAy, &rawAz);
    // Convert to g assuming default sensitivity (16384 LSB/g)
    ax = rawAx / 16384.0f;
    ay = rawAy / 16384.0f;
    az = rawAz / 16384.0f;
}

static int readMicSample() {
    // INMP441 is I2S-only, not analog. Proper audio capture requires I2S driver.
    // Return -1 to indicate "not available via simple read"
    // TODO: Implement proper I2S microphone capture for real audio levels
    return -1;  // Indicates I2S mic, not analog readable
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
    
    // Battery voltage and percentage
    float voltage = readBatteryVoltage();
    obj["vbat"] = voltage;
    obj["vbat_pct"] = readBatteryPercent(voltage);

    // Microphone: I2S only, returns -1 for "not analog readable"
    obj["mic_raw"] = readMicSample();

    // MPU6050 sample (in g). Falls back to zeros if not connected.
    float ax, ay, az;
    readMPUSample(ax, ay, az);
    obj["ax"] = ax;
    obj["ay"] = ay;
    obj["az"] = az;
    
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
        int commaIdx = val.indexOf(',');
        if(commaIdx != -1) {
            int freq = val.substring(0, commaIdx).toInt();
            int dur = val.substring(commaIdx+1).toInt();
            int pin = hwConfig.i2s_do;
            if (pin > 0 && pin <= 48 && freq > 0) {
                ledcAttach(pin, freq, 8);
                ledcWrite(pin, 127);
                delay(dur);
                ledcDetach(pin);
                pinMode(pin, INPUT);
            }
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
            int micVal = readMicSample();
            Serial.print("{\"status\":\"ok\",\"mic_raw\":");
            Serial.print(micVal);
            Serial.println("}");
            return;
        } else if (strcmp(actionType, "test_mpu_read") == 0) {
            float ax, ay, az;
            readMPUSample(ax, ay, az);
            Serial.print("{\"status\":\"ok\",\"ax\":");
            Serial.print(ax, 3);
            Serial.print(",\"ay\":");
            Serial.print(ay, 3);
            Serial.print(",\"az\":");
            Serial.print(az, 3);
            Serial.println("}");
            return;
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
    } else if (strcmp(cmdType, CMD_GET_EYES) == 0 || strcmp(cmdType, CMD_GET_SETTINGS) == 0) {
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
        // Print loaded hardware configuration for diagnostics
    Serial.println("{\\\"status\\\":\\\"info\\\",\\\"msg\\\":\\\"Hardware Config Loaded:\\\"}");
    Serial.printf("  I2C: SDA=%d SCL=%d\\n", hwConfig.i2c_sda, hwConfig.i2c_scl);
    Serial.printf("  I2S: BCLK=%d LRC=%d DO=%d DI=%d\\n", 
                  hwConfig.i2s_bclk, hwConfig.i2s_lrc, hwConfig.i2s_do, hwConfig.mic_sd);
    Serial.printf("  GPIO: BTN=%d LED=%d VBAT=%d\\n", 
                  hwConfig.func_btn, hwConfig.led_pin, hwConfig.vbat_pin);
    Serial.printf("  Motors: VibL=%d VibR=%d\\n", hwConfig.vibro_left, hwConfig.vibro_right);
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
