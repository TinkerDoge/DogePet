// DogePet Serial Command Implementation
// JSON-based protocol for PC companion app communication

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "include/serial_cmd.h"
#include "include/config.h"

// Modules
#include "include/Face.h"
#include "include/Motion.h"
#include "include/Audio.h"
#include "include/Power.h"
#include "include/Haptics.h"
#include "include/LED.h"

// Hardware Config (Runtime)
HardwareConfig hwConfig; // Global instance

static String serialBuffer = "";
static Preferences prefs;

// Current settings (runtime state)
static struct {
    uint8_t eyeWidth = EYE_WIDTH;
    uint8_t eyeHeight = EYE_HEIGHT;
    uint8_t borderRadius = EYE_BORDER_RADIUS;
    int8_t spacing = EYE_SPACING;
    uint8_t mood = 0;
    uint8_t position = 0; // 0=Center, 1=N, 2=NE... clockwise
    
    // Animation flags
    bool autoBlink = true;
    bool idleMode = true;
    bool sweat = false;
    bool curiosity = false;
    bool cyclops = false;
    
    // Timings (in SECONDS for RoboEyes library)
    uint16_t blinkInterval = 3;   // Seconds between blinks
    uint16_t blinkVariation = 1;  // Random variation (+/- seconds)
    uint16_t idleInterval = 4;    // Seconds between idle movements
    uint16_t idleVariation = 2;   // Random variation (+/- seconds)
} eyeSettings;

// Runtime configuration (peripheral settings)
static struct {
    uint8_t ledBrightness = LED_BRIGHTNESS;
    uint8_t audioVolume = AUDIO_VOLUME;
    uint32_t audioSampleRate = AUDIO_SAMPLE_RATE;
    uint32_t displayUpdateMs = DISPLAY_UPDATE_MS;
    uint16_t btnDebounceMs = BTN_DEBOUNCE_MS;
    char deviceName[32] = "DogePet";
} runtimeConfig;

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================
static void handleConnect();
static void handleGetPinout();
static void handleSetPinout(JsonObject obj);
static void handleGetSensors();
static void handleAction(JsonObject obj);
static void handleGetEyes();
static void handleSetEyes(JsonObject obj);
static void handleGetConfig();
static void handleSetConfig(JsonObject obj);
static void applyEyeSettings();
static void loadRuntimeConfig();
static void saveRuntimeConfig();

// =============================================================================
// NVS / CONFIG HELPERS
// =============================================================================
static bool isNVSValid() {
    if (!prefs.begin("dogepet_hw", true)) {
        // NVS namespace doesn't exist yet (first boot)
        prefs.end();
        return false;
    }
    int sda = prefs.getInt("sda", -999);
    prefs.end();
    return (sda != -999 && sda >= 0 && sda <= 48);
}

static void resetHardwareConfig() {
    prefs.begin("dogepet_hw", false);
    prefs.clear();
    prefs.end();
    
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
    prefs.putInt("mic_sd", hwConfig.mic_sd);
    prefs.end();
    
    Serial.println("{\"status\":\"info\",\"msg\":\"NVS reset to factory defaults\"}");
}

void loadHardwareConfig() {
    if (!isNVSValid()) {
        resetHardwareConfig();
        return;
    }
    prefs.begin("dogepet_hw", true);
    
    auto loadPin = [](const char* key, int defaultVal) -> int {
        int pin = prefs.getInt(key, defaultVal);
        if (pin < 0 || pin > 48) return defaultVal;
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
    hwConfig.mic_sd = loadPin("mic_sd", I2S_DI);
    
    // Shared I2S clocks defaults (simplification)
    hwConfig.i2s_bclk = I2S_BCLK;
    hwConfig.i2s_lrc = I2S_LRC;
    hwConfig.mic_ws = I2S_LRC;
    hwConfig.mic_sck = I2S_BCLK;
    
    prefs.end();
}

HardwareConfig* getHardwareConfig() {
    return &hwConfig;
}

static void loadSettings() {
    if (!prefs.begin("dogepet_eyes", true)) {
        // First boot - use defaults
        prefs.end();
        return;
    }
    eyeSettings.eyeWidth = prefs.getUChar("width", EYE_WIDTH);
    eyeSettings.eyeHeight = prefs.getUChar("height", EYE_HEIGHT);
    eyeSettings.borderRadius = prefs.getUChar("radius", EYE_BORDER_RADIUS);
    eyeSettings.spacing = prefs.getChar("spacing", EYE_SPACING);
    
    // Validate
    if (eyeSettings.eyeWidth < 10) eyeSettings.eyeWidth = 10;
    if (eyeSettings.eyeHeight < 10) eyeSettings.eyeHeight = 10;
    
    eyeSettings.autoBlink = prefs.getBool("blink", true);
    eyeSettings.idleMode = prefs.getBool("idle", true);
    
    prefs.end();
}

static void saveSettings() {
    prefs.begin("dogepet_eyes", false);
    prefs.putUChar("width", eyeSettings.eyeWidth);
    prefs.putUChar("height", eyeSettings.eyeHeight);
    prefs.putUChar("radius", eyeSettings.borderRadius);
    prefs.putChar("spacing", eyeSettings.spacing);
    prefs.putBool("blink", eyeSettings.autoBlink);
    prefs.putBool("idle", eyeSettings.idleMode);
    prefs.end();
}

// =============================================================================
// MAIN SERIAL PROCESSOR
// =============================================================================
void processSerialCmd() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            serialBuffer.trim();
            if (serialBuffer.length() > 0) {
                // Parse JSON
                JsonDocument doc; // ArduinoJson 7 compatible logic (or v6 DynamicJsonDocument)
                // Use standard DynamicJsonDocument for maximum compatibility if library version is unsure, 
                // but standard ArduinoJson 6 pattern works well
                DynamicJsonDocument jsonDoc(1024);
                DeserializationError error = deserializeJson(jsonDoc, serialBuffer);
                
                if (!error) {
                    JsonObject obj = jsonDoc.as<JsonObject>();
                    if (obj.containsKey("cmd")) {
                        const char* cmd = obj["cmd"];
                        
                        if (strcmp(cmd, "connect") == 0) handleConnect();
                        else if (strcmp(cmd, "get_pinout") == 0) handleGetPinout();
                        else if (strcmp(cmd, "set_pinout") == 0) handleSetPinout(obj);
                        else if (strcmp(cmd, "get_sensors") == 0) handleGetSensors();
                        else if (strcmp(cmd, "action") == 0) handleAction(obj);
                        else if (strcmp(cmd, CMD_GET_EYES) == 0) handleGetEyes();
                        else if (strcmp(cmd, CMD_GET_SETTINGS) == 0) handleGetEyes();
                        else if (strcmp(cmd, "set_eyes") == 0) handleSetEyes(obj);
                        else if (strcmp(cmd, "get_config") == 0) handleGetConfig();
                        else if (strcmp(cmd, "set_config") == 0) handleSetConfig(obj);
                        else {
                            Serial.println("{\"status\":\"error\",\"msg\":\"Unknown command\"}");
                        }
                    } else {
                        Serial.println("{\"status\":\"error\",\"msg\":\"Missing cmd field\"}");
                    }
                } else {
                    Serial.println("{\"status\":\"error\",\"msg\":\"Invalid JSON\"}");
                }
            }
            serialBuffer = "";
        } else {
            if (serialBuffer.length() < 512) {
                serialBuffer += c;
            }
        }
    }
}

// =============================================================================
// HANDLERS
// =============================================================================
static void handleConnect() {
    Serial.println("{\"status\":\"ok\",\"msg\":\"DogePet Protected Mode Active\"}");
}

static void handleGetPinout() {
    DynamicJsonDocument doc(512);
    doc["status"] = "ok";
    // Return runtime-loaded config (from NVS or defaults)
    doc["sda"] = hwConfig.i2c_sda;
    doc["scl"] = hwConfig.i2c_scl;
    doc["btn"] = hwConfig.func_btn;
    doc["led"] = hwConfig.led_pin;
    doc["vibL"] = hwConfig.vibro_left;
    doc["vibR"] = hwConfig.vibro_right;
    doc["vbat"] = hwConfig.vbat_pin;
    doc["mic_sd"] = hwConfig.mic_sd;
    doc["mic_ws"] = hwConfig.mic_ws;
    doc["mic_sck"] = hwConfig.mic_sck;
    doc["i2s_do"] = hwConfig.i2s_do;
    doc["i2s_bclk"] = hwConfig.i2s_bclk;
    doc["i2s_lrc"] = hwConfig.i2s_lrc;
    
    serializeJson(doc, Serial);
    Serial.println();
}

static void handleSetPinout(JsonObject obj) {
    prefs.begin("dogepet_hw", false);
    if (obj.containsKey("sda")) prefs.putInt("sda", obj["sda"]);
    if (obj.containsKey("scl")) prefs.putInt("scl", obj["scl"]);
    if (obj.containsKey("vibL")) prefs.putInt("vibL", obj["vibL"]);
    if (obj.containsKey("vibR")) prefs.putInt("vibR", obj["vibR"]);
    if (obj.containsKey("btn")) prefs.putInt("btn", obj["btn"]);
    if (obj.containsKey("led")) prefs.putInt("led", obj["led"]);
    if (obj.containsKey("vbat")) prefs.putInt("vbat", obj["vbat"]);
    if (obj.containsKey("mic_sd")) prefs.putInt("mic_sd", obj["mic_sd"]);
    if (obj.containsKey("i2s_do")) prefs.putInt("i2s_do", obj["i2s_do"]);
    if (obj.containsKey("i2s_bclk")) prefs.putInt("i2s_bclk", obj["i2s_bclk"]);
    if (obj.containsKey("i2s_lrc")) prefs.putInt("i2s_lrc", obj["i2s_lrc"]);
    prefs.end();
    
    Serial.println("{\"status\":\"ok\",\"msg\":\"Pinout saved. Restart required.\"}");
}

static void handleGetSensors() {
    DynamicJsonDocument doc(768);
    doc["status"] = "ok";
    
    // Power
    doc["vbat"] = Power::getVoltage();
    doc["vbat_pct"] = Power::getPercent();
    
    // Mic (dB) 
    doc["mic_db"] = Audio::readMicDB();
    
    // Motion (Accelerometer + Gyroscope)
    int16_t ax, ay, az, gx, gy, gz;
    Motion::getRawData(ax, ay, az, gx, gy, gz);
    // Accelerometer (convert to G, FS=2G -> 16384 LSB/G)
    doc["ax"] = ax / 16384.0f;
    doc["ay"] = ay / 16384.0f;
    doc["az"] = az / 16384.0f;
    // Gyroscope (convert to deg/s, FS=250 -> 131 LSB/(deg/s))
    doc["gx"] = gx / 131.0f;
    doc["gy"] = gy / 131.0f;
    doc["gz"] = gz / 131.0f;
    
    serializeJson(doc, Serial);
    Serial.println();
}

static void handleAction(JsonObject obj) {
    const char* type = obj["type"];
    String val = obj["value"] | "";
    
    roboEyes* eyes = Face::getEyes(); 
    
    if (strcmp(type, "blink") == 0) {
        eyes->blink();
    } 
    else if (strcmp(type, "laugh") == 0) {
        // Simple manual anim
        int originalPos = eyeSettings.position;
        eyes->setMood(3); // Happy
        eyes->setPosition(1); Face::update(); delay(100);
        eyes->setPosition(5); Face::update(); delay(100);
        eyes->setPosition(originalPos);
        applyEyeSettings(); 
    }
    else if (strcmp(type, "close") == 0) {
        eyeSettings.eyeHeight = 2; // Close
        applyEyeSettings();
    }
    else if (strcmp(type, "open") == 0) {
        loadSettings(); 
        applyEyeSettings();
    }
    else if (strcmp(type, "save_restart") == 0) {
        saveSettings();
        ESP.restart();
    }
    else if (strcmp(type, "test_vib_l_on") == 0) Haptics::buzz(255, 0, 0);
    else if (strcmp(type, "test_vib_l_off") == 0) Haptics::buzz(0, 0, 0);
    else if (strcmp(type, "test_vib_r_on") == 0) Haptics::buzz(0, 255, 0);
    else if (strcmp(type, "test_vib_r_off") == 0) Haptics::buzz(0, 0, 0);
    else if (strcmp(type, "test_vib_seq1") == 0) Haptics::doubleClick();
    else if (strcmp(type, "test_vib_seq2") == 0) Haptics::alarm();
    
    // LED tests
    else if (strcmp(type, "test_led_on") == 0) { LED::on(); }
    else if (strcmp(type, "test_led_off") == 0) { LED::off(); }
    
    // OLED tests
    else if (strcmp(type, "test_oled_pattern") == 0) { Face::showTestPattern(); }
    else if (strcmp(type, "test_oled_clear") == 0) { Face::clear(); }
    else if (strcmp(type, "test_oled_text") == 0) { Face::showText(val.c_str()); }
    
    // Audio tests
    else if (strcmp(type, "test_tone_on") == 0) Audio::playTone(440, 1000); 
    else if (strcmp(type, "test_tone_off") == 0) Audio::stop(); 
    else if (strcmp(type, "test_tone_custom") == 0) {
        int comma = val.indexOf(',');
        if (comma > 0) {
            int f = val.substring(0, comma).toInt();
            int d = val.substring(comma+1).toInt();
            Audio::playTone(f, d);
        }
    }
    else if (strcmp(type, "test_mic_read") == 0) Audio::testMicLog();
    else if (strcmp(type, "test_mpu_read") == 0) {
        int16_t ax, ay, az, gx, gy, gz;
        Motion::getRawData(ax, ay, az, gx, gy, gz);
        Serial.printf("{\"status\":\"data\",\"type\":\"imu\",\"ax\":%d,\"ay\":%d,\"az\":%d,\"gx\":%d,\"gy\":%d,\"gz\":%d}\n", ax, ay, az, gx, gy, gz);
    }
    
    Serial.println("{\"status\":\"ok\"}");
}

static void handleGetEyes() {
    DynamicJsonDocument doc(512);
    doc["status"] = "ok";
    doc["width"] = eyeSettings.eyeWidth;
    doc["height"] = eyeSettings.eyeHeight;
    doc["radius"] = eyeSettings.borderRadius;
    doc["spacing"] = eyeSettings.spacing;
    doc["mood"] = eyeSettings.mood;
    doc["position"] = eyeSettings.position;
    doc["autoBlink"] = eyeSettings.autoBlink;
    doc["idleMode"] = eyeSettings.idleMode;
    doc["sweat"] = eyeSettings.sweat;
    doc["curiosity"] = eyeSettings.curiosity;
    doc["cyclops"] = eyeSettings.cyclops;
    
    serializeJson(doc, Serial);
    Serial.println();
}

static void handleSetEyes(JsonObject obj) {
    if (obj.containsKey("width")) eyeSettings.eyeWidth = obj["width"];
    if (obj.containsKey("height")) eyeSettings.eyeHeight = obj["height"];
    if (obj.containsKey("radius")) eyeSettings.borderRadius = obj["radius"];
    if (obj.containsKey("spacing")) eyeSettings.spacing = obj["spacing"];
    if (obj.containsKey("mood")) eyeSettings.mood = obj["mood"];
    if (obj.containsKey("position")) eyeSettings.position = obj["position"];
    if (obj.containsKey("autoBlink")) eyeSettings.autoBlink = obj["autoBlink"];
    if (obj.containsKey("idleMode")) eyeSettings.idleMode = obj["idleMode"];
    if (obj.containsKey("sweat")) eyeSettings.sweat = obj["sweat"];
    if (obj.containsKey("curiosity")) eyeSettings.curiosity = obj["curiosity"];
    if (obj.containsKey("cyclops")) eyeSettings.cyclops = obj["cyclops"];
    
    applyEyeSettings();
    saveSettings(); 
    
    Serial.println("{\"status\":\"ok\"}");
}

static void applyEyeSettings() {
    roboEyes* eyes = Face::getEyes();
    if (!eyes) return;
    
    eyes->setWidth(eyeSettings.eyeWidth, eyeSettings.eyeWidth);
    eyes->setHeight(eyeSettings.eyeHeight, eyeSettings.eyeHeight);
    eyes->setBorderradius(eyeSettings.borderRadius, eyeSettings.borderRadius);
    eyes->setSpacebetween(eyeSettings.spacing);
    eyes->setMood(eyeSettings.mood);
    eyes->setPosition(eyeSettings.position);
    eyes->setAutoblinker(eyeSettings.autoBlink, eyeSettings.blinkInterval, eyeSettings.blinkVariation);
    eyes->setIdleMode(eyeSettings.idleMode, eyeSettings.idleInterval, eyeSettings.idleVariation);
    eyes->setCyclops(eyeSettings.cyclops);
    eyes->setCuriosity(eyeSettings.curiosity);
    eyes->setSweat(eyeSettings.sweat);
}

// =============================================================================
// RUNTIME CONFIG HANDLERS
// =============================================================================
static void loadRuntimeConfig() {
    if (!prefs.begin("dogepet_cfg", true)) {
        prefs.end();
        return;
    }
    runtimeConfig.ledBrightness = prefs.getUChar("led_bright", LED_BRIGHTNESS);
    runtimeConfig.audioVolume = prefs.getUChar("audio_vol", AUDIO_VOLUME);
    runtimeConfig.audioSampleRate = prefs.getUInt("audio_rate", AUDIO_SAMPLE_RATE);
    runtimeConfig.displayUpdateMs = prefs.getUInt("disp_ms", DISPLAY_UPDATE_MS);
    runtimeConfig.btnDebounceMs = prefs.getUShort("btn_debounce", BTN_DEBOUNCE_MS);
    prefs.getString("dev_name", runtimeConfig.deviceName, sizeof(runtimeConfig.deviceName));
    prefs.end();
}

static void saveRuntimeConfig() {
    prefs.begin("dogepet_cfg", false);
    prefs.putUChar("led_bright", runtimeConfig.ledBrightness);
    prefs.putUChar("audio_vol", runtimeConfig.audioVolume);
    prefs.putUInt("audio_rate", runtimeConfig.audioSampleRate);
    prefs.putUInt("disp_ms", runtimeConfig.displayUpdateMs);
    prefs.putUShort("btn_debounce", runtimeConfig.btnDebounceMs);
    prefs.putString("dev_name", runtimeConfig.deviceName);
    prefs.end();
}

static void applyRuntimeConfig() {
    LED::setBrightness(runtimeConfig.ledBrightness);
    Audio::setVolume(runtimeConfig.audioVolume);
}

static void handleGetConfig() {
    DynamicJsonDocument doc(512);
    doc["status"] = "ok";
    doc["ledBrightness"] = runtimeConfig.ledBrightness;
    doc["audioVolume"] = runtimeConfig.audioVolume;
    doc["audioSampleRate"] = runtimeConfig.audioSampleRate;
    doc["displayUpdateMs"] = runtimeConfig.displayUpdateMs;
    doc["btnDebounceMs"] = runtimeConfig.btnDebounceMs;
    doc["deviceName"] = runtimeConfig.deviceName;
    
    serializeJson(doc, Serial);
    Serial.println();
}

static void handleSetConfig(JsonObject obj) {
    if (obj.containsKey("ledBrightness")) runtimeConfig.ledBrightness = obj["ledBrightness"];
    if (obj.containsKey("audioVolume")) runtimeConfig.audioVolume = obj["audioVolume"];
    if (obj.containsKey("audioSampleRate")) runtimeConfig.audioSampleRate = obj["audioSampleRate"];
    if (obj.containsKey("displayUpdateMs")) runtimeConfig.displayUpdateMs = obj["displayUpdateMs"];
    if (obj.containsKey("btnDebounceMs")) runtimeConfig.btnDebounceMs = obj["btnDebounceMs"];
    if (obj.containsKey("deviceName")) {
        strlcpy(runtimeConfig.deviceName, obj["deviceName"] | "DogePet", sizeof(runtimeConfig.deviceName));
    }
    
    applyRuntimeConfig();
    saveRuntimeConfig();
    
    Serial.println("{\"status\":\"ok\"}");
}

void setupSerialCmd(roboEyes* eyesPtr) {
    // eyesPtr ignored now, we use Face::getEyes() via applyEyeSettings
    serialBuffer.reserve(512);
    
    Serial.println("{\"status\":\"info\",\"msg\":\"Serial Command Ready\"}");
    
    loadSettings();
    loadRuntimeConfig();
    applyEyeSettings();
    applyRuntimeConfig();
}
