// Settings.cpp - Runtime and NVS persistent settings
#include "Settings.h"
#include "Face.h"
#include "Audio.h"
#include "Haptics.h"
#include "LED.h"
#include <ArduinoJson.h>

// =============================================================================
// STATIC MEMBER DEFINITIONS
// =============================================================================
FaceSettings   Settings::face;
AudioSettings  Settings::audio;
HapticSettings Settings::haptic;
LEDSettings    Settings::led;
MotionSettings Settings::motion;
PowerSettings  Settings::power;
PinSettings    Settings::pins;
char           Settings::botName[32];
bool           Settings::pendingReboot = false;
Preferences    Settings::prefs;

// NVS namespace
static const char* NVS_NAMESPACE = "dogepet";

// =============================================================================
// LIFECYCLE
// =============================================================================

void Settings::begin() {
    loadDefaults();
    prefs.begin(NVS_NAMESPACE, false);
    loadFromNVS();
    prefs.end();
    Serial.println("{\"status\":\"info\",\"msg\":\"Settings loaded from NVS\"}");
}

void Settings::save() {
    prefs.begin(NVS_NAMESPACE, false);
    saveToNVS();
    prefs.end();
    Serial.println("{\"status\":\"ok\",\"msg\":\"Settings saved to NVS\"}");
}

void Settings::resetDefaults() {
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
    loadDefaults();
    pendingReboot = true;
    Serial.println("{\"status\":\"info\",\"msg\":\"Settings reset to defaults. Reboot required.\"}");
}

// =============================================================================
// LOAD DEFAULTS FROM config.h
// =============================================================================

void Settings::loadDefaults() {
    // Bot name
    strncpy(botName, DEVICE_NAME, sizeof(botName) - 1);
    botName[sizeof(botName) - 1] = '\0';
    
    // Face (Dynamic)
    face.width         = DEFAULT_EYE_WIDTH;
    face.height        = DEFAULT_EYE_HEIGHT;
    face.radius        = DEFAULT_EYE_BORDER_RADIUS;
    face.spacing       = DEFAULT_EYE_SPACING;
    face.autoBlink     = DEFAULT_EYE_AUTO_BLINK;
    face.idleMode      = DEFAULT_EYE_IDLE_MODE;
    face.blinkInterval = DEFAULT_BLINK_INTERVAL;
    face.idleInterval  = DEFAULT_IDLE_INTERVAL;
    face.contrast      = DEFAULT_OLED_CONTRAST;
    face.curious       = false;
    face.sweat         = false;
    
    // Audio (Dynamic)
    audio.volume        = DEFAULT_AUDIO_VOLUME;
    audio.micLogEnabled = true;
    
    // Haptic (Dynamic)
    haptic.intensity = DEFAULT_HAPTIC_INTENSITY;
    
    // LED (Dynamic)
    led.brightness = LED_BRIGHTNESS;
    led.r = LED_COLOR_R;
    led.g = LED_COLOR_G;
    led.b = LED_COLOR_B;
    
    // Motion (Persistent)
    motion.tiltDeg        = DEFAULT_TILT_THRESHOLD_DEG;
    motion.shakeAngryDps  = DEFAULT_SHAKE_ANGRY_DPS;
    motion.shakeFuriousDps = DEFAULT_SHAKE_FURIOUS_DPS;
    motion.tapSpikeDps    = DEFAULT_TAP_SPIKE_DPS;
    
    // Power (Persistent)
    power.idleTimeoutMs  = DEFAULT_IDLE_TIMEOUT_MS;
    power.sleepTimeoutMs = DEFAULT_SLEEP_TIMEOUT_MS;
    
    // Pins (Persistent)
    pins.i2cSda     = I2C_SDA;
    pins.i2cScl     = I2C_SCL;
    pins.funcBtn    = FUNC_BTN;
    pins.touchChin  = TOUCH_CHIN;
    pins.i2sBclk    = I2S_BCLK;
    pins.i2sLrc     = I2S_LRC;
    pins.i2sDo      = I2S_DO;
    pins.i2sDi      = I2S_DI;
    pins.ledPin     = LED_PIN;
    pins.vbatPin    = VBAT_PIN;
    pins.vibroLeft  = VIBRO_LEFT;
    pins.vibroRight = VIBRO_RIGHT;
}

// =============================================================================
// NVS LOAD/SAVE
// =============================================================================

void Settings::loadFromNVS() {
    // Bot name
    String name = prefs.getString("bot_name", DEVICE_NAME);
    strncpy(botName, name.c_str(), sizeof(botName) - 1);
    
    // Face (Dynamic)
    face.width         = prefs.getUChar("f_width", DEFAULT_EYE_WIDTH);
    face.height        = prefs.getUChar("f_height", DEFAULT_EYE_HEIGHT);
    face.radius        = prefs.getUChar("f_radius", DEFAULT_EYE_BORDER_RADIUS);
    face.spacing       = prefs.getChar("f_spacing", DEFAULT_EYE_SPACING);
    face.autoBlink     = prefs.getBool("f_blink", DEFAULT_EYE_AUTO_BLINK);
    face.idleMode      = prefs.getBool("f_idle", DEFAULT_EYE_IDLE_MODE);
    face.blinkInterval = prefs.getUChar("f_blink_int", DEFAULT_BLINK_INTERVAL);
    face.idleInterval  = prefs.getUChar("f_idle_int", DEFAULT_IDLE_INTERVAL);
    face.contrast      = prefs.getUChar("f_contrast", DEFAULT_OLED_CONTRAST);
    face.curious       = prefs.getBool("f_curious", DEFAULT_EYE_CURIOUS_MODE);
    face.sweat         = prefs.getBool("f_sweat", false);
    
    // Audio (Dynamic)
    audio.volume        = prefs.getUChar("a_volume", DEFAULT_AUDIO_VOLUME);
    audio.micLogEnabled = prefs.getBool("a_miclog", true);
    
    // Haptic (Dynamic)
    haptic.intensity = prefs.getUChar("h_intensity", DEFAULT_HAPTIC_INTENSITY);
    
    // LED (Dynamic)
    led.brightness = prefs.getUChar("l_bright", LED_BRIGHTNESS);
    led.r = prefs.getUChar("l_r", LED_COLOR_R);
    led.g = prefs.getUChar("l_g", LED_COLOR_G);
    led.b = prefs.getUChar("l_b", LED_COLOR_B);
    
    // Motion (Persistent)
    motion.tiltDeg        = prefs.getFloat("m_tilt", DEFAULT_TILT_THRESHOLD_DEG);
    motion.shakeAngryDps  = prefs.getFloat("m_shake", DEFAULT_SHAKE_ANGRY_DPS);
    motion.shakeFuriousDps = prefs.getFloat("m_furious", DEFAULT_SHAKE_FURIOUS_DPS);
    motion.tapSpikeDps    = prefs.getFloat("m_tap", DEFAULT_TAP_SPIKE_DPS);
    
    // Power (Persistent)
    power.idleTimeoutMs  = prefs.getULong("p_idle", DEFAULT_IDLE_TIMEOUT_MS);
    power.sleepTimeoutMs = prefs.getULong("p_sleep", DEFAULT_SLEEP_TIMEOUT_MS);
    
    // Pins (Persistent) - use config.h constants as defaults
    pins.i2cSda     = prefs.getUChar("pin_sda", I2C_SDA);
    pins.i2cScl     = prefs.getUChar("pin_scl", I2C_SCL);
    pins.funcBtn    = prefs.getUChar("pin_btn", FUNC_BTN);
    pins.touchChin  = prefs.getUChar("pin_chin", TOUCH_CHIN);
    pins.i2sBclk    = prefs.getUChar("pin_bclk", I2S_BCLK);
    pins.i2sLrc     = prefs.getUChar("pin_lrc", I2S_LRC);
    pins.i2sDo      = prefs.getUChar("pin_do", I2S_DO);
    pins.i2sDi      = prefs.getUChar("pin_di", I2S_DI);
    pins.ledPin     = prefs.getUChar("pin_led", LED_PIN);
    pins.vbatPin    = prefs.getUChar("pin_vbat", VBAT_PIN);
    pins.vibroLeft  = prefs.getUChar("pin_vl", VIBRO_LEFT);
    pins.vibroRight = prefs.getUChar("pin_vr", VIBRO_RIGHT);
}

void Settings::saveToNVS() {
    // Bot name
    prefs.putString("bot_name", botName);
    
    // Face (Dynamic)
    prefs.putUChar("f_width", face.width);
    prefs.putUChar("f_height", face.height);
    prefs.putUChar("f_radius", face.radius);
    prefs.putChar("f_spacing", face.spacing);
    prefs.putBool("f_blink", face.autoBlink);
    prefs.putBool("f_idle", face.idleMode);
    prefs.putUChar("f_blink_int", face.blinkInterval);
    prefs.putUChar("f_idle_int", face.idleInterval);
    prefs.putUChar("f_contrast", face.contrast);
    prefs.putBool("f_curious", face.curious);
    prefs.putBool("f_sweat", face.sweat);
    
    // Audio (Dynamic)
    prefs.putUChar("a_volume", audio.volume);
    prefs.putBool("a_miclog", audio.micLogEnabled);
    
    // Haptic (Dynamic)
    prefs.putUChar("h_intensity", haptic.intensity);
    
    // LED (Dynamic)
    prefs.putUChar("l_bright", led.brightness);
    prefs.putUChar("l_r", led.r);
    prefs.putUChar("l_g", led.g);
    prefs.putUChar("l_b", led.b);
    
    // Motion (Persistent)
    prefs.putFloat("m_tilt", motion.tiltDeg);
    prefs.putFloat("m_shake", motion.shakeAngryDps);
    prefs.putFloat("m_furious", motion.shakeFuriousDps);
    prefs.putFloat("m_tap", motion.tapSpikeDps);
    
    // Power (Persistent)
    prefs.putULong("p_idle", power.idleTimeoutMs);
    prefs.putULong("p_sleep", power.sleepTimeoutMs);
    
    // Pins (Persistent)
    prefs.putUChar("pin_sda", pins.i2cSda);
    prefs.putUChar("pin_scl", pins.i2cScl);
    prefs.putUChar("pin_btn", pins.funcBtn);
    prefs.putUChar("pin_chin", pins.touchChin);
    prefs.putUChar("pin_bclk", pins.i2sBclk);
    prefs.putUChar("pin_lrc", pins.i2sLrc);
    prefs.putUChar("pin_do", pins.i2sDo);
    prefs.putUChar("pin_di", pins.i2sDi);
    prefs.putUChar("pin_led", pins.ledPin);
    prefs.putUChar("pin_vbat", pins.vbatPin);
    prefs.putUChar("pin_vl", pins.vibroLeft);
    prefs.putUChar("pin_vr", pins.vibroRight);
}

// =============================================================================
// APPLY DYNAMIC SETTINGS TO MODULES
// =============================================================================

void Settings::applyFaceSettings() {
    // Called to push current face settings to the Face module
    // Face::applySettings() will read Settings::face
    Face::applySettings();
    Serial.println("{\"status\":\"info\",\"msg\":\"Face settings applied\",\"dynamic\":true}");
}

void Settings::applyAudioSettings() {
    // Apply audio volume and mic logging
    Audio::setVolume(audio.volume);
    Audio::setMicLogEnabled(audio.micLogEnabled);
    Serial.println("{\"status\":\"info\",\"msg\":\"Audio settings applied\",\"dynamic\":true}");
}

void Settings::applyHapticSettings() {
    // Haptic intensity is applied per-effect, just log
    Serial.println("{\"status\":\"info\",\"msg\":\"Haptic settings applied\",\"dynamic\":true}");
}

void Settings::applyLEDSettings() {
    // Apply LED color and brightness
    LED::setColor(led.r, led.g, led.b);
    LED::setBrightness(led.brightness);
    Serial.println("{\"status\":\"info\",\"msg\":\"LED settings applied\",\"dynamic\":true}");
}

// =============================================================================
// JSON SERIALIZATION
// =============================================================================

String Settings::toJson() {
    JsonDocument doc;
    
    // Device info
    doc["bot_name"] = botName;
    doc["firmware"] = FIRMWARE_VER;
    doc["pending_reboot"] = pendingReboot;
    
    // Dynamic settings
    JsonObject f = doc["face"].to<JsonObject>();
    f["width"] = face.width;
    f["height"] = face.height;
    f["radius"] = face.radius;
    f["spacing"] = face.spacing;
    f["auto_blink"] = face.autoBlink;
    f["idle_mode"] = face.idleMode;
    f["blink_interval"] = face.blinkInterval;
    f["idle_interval"] = face.idleInterval;
    f["contrast"] = face.contrast;
    f["curious"] = face.curious;
    f["sweat"] = face.sweat;
    
    JsonObject a = doc["audio"].to<JsonObject>();
    a["volume"] = audio.volume;
    a["mic_log"] = audio.micLogEnabled;
    
    JsonObject h = doc["haptic"].to<JsonObject>();
    h["intensity"] = haptic.intensity;
    
    JsonObject l = doc["led"].to<JsonObject>();
    l["brightness"] = led.brightness;
    l["r"] = led.r;
    l["g"] = led.g;
    l["b"] = led.b;
    
    // Persistent settings
    JsonObject m = doc["motion"].to<JsonObject>();
    m["tilt_deg"] = motion.tiltDeg;
    m["shake_angry_dps"] = motion.shakeAngryDps;
    m["shake_furious_dps"] = motion.shakeFuriousDps;
    m["tap_spike_dps"] = motion.tapSpikeDps;
    
    JsonObject p = doc["power"].to<JsonObject>();
    p["idle_timeout_ms"] = power.idleTimeoutMs;
    p["sleep_timeout_ms"] = power.sleepTimeoutMs;
    
    JsonObject pins_obj = doc["pins"].to<JsonObject>();
    pins_obj["i2c_sda"] = pins.i2cSda;
    pins_obj["i2c_scl"] = pins.i2cScl;
    pins_obj["func_btn"] = pins.funcBtn;
    pins_obj["touch_chin"] = pins.touchChin;
    pins_obj["i2s_bclk"] = pins.i2sBclk;
    pins_obj["i2s_lrc"] = pins.i2sLrc;
    pins_obj["i2s_do"] = pins.i2sDo;
    pins_obj["i2s_di"] = pins.i2sDi;
    pins_obj["led"] = pins.ledPin;
    pins_obj["vbat"] = pins.vbatPin;
    pins_obj["vibro_left"] = pins.vibroLeft;
    pins_obj["vibro_right"] = pins.vibroRight;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String Settings::toDynamicJson() {
    JsonDocument doc;
    
    doc["bot_name"] = botName;
    
    JsonObject f = doc["face"].to<JsonObject>();
    f["width"] = face.width;
    f["height"] = face.height;
    f["radius"] = face.radius;
    f["spacing"] = face.spacing;
    f["auto_blink"] = face.autoBlink;
    f["idle_mode"] = face.idleMode;
    f["blink_interval"] = face.blinkInterval;
    f["idle_interval"] = face.idleInterval;
    f["contrast"] = face.contrast;
    
    JsonObject a = doc["audio"].to<JsonObject>();
    a["volume"] = audio.volume;
    a["mic_log"] = audio.micLogEnabled;
    
    JsonObject h = doc["haptic"].to<JsonObject>();
    h["intensity"] = haptic.intensity;
    
    JsonObject l = doc["led"].to<JsonObject>();
    l["brightness"] = led.brightness;
    l["r"] = led.r;
    l["g"] = led.g;
    l["b"] = led.b;
    
    String output;
    serializeJson(doc, output);
    return output;
}

String Settings::toPersistentJson() {
    JsonDocument doc;
    
    doc["pending_reboot"] = pendingReboot;
    
    JsonObject m = doc["motion"].to<JsonObject>();
    m["tilt_deg"] = motion.tiltDeg;
    m["shake_angry_dps"] = motion.shakeAngryDps;
    m["shake_furious_dps"] = motion.shakeFuriousDps;
    m["tap_spike_dps"] = motion.tapSpikeDps;
    
    JsonObject p = doc["power"].to<JsonObject>();
    p["idle_timeout_ms"] = power.idleTimeoutMs;
    p["sleep_timeout_ms"] = power.sleepTimeoutMs;
    
    JsonObject pins_obj = doc["pins"].to<JsonObject>();
    pins_obj["i2c_sda"] = pins.i2cSda;
    pins_obj["i2c_scl"] = pins.i2cScl;
    pins_obj["func_btn"] = pins.funcBtn;
    pins_obj["touch_chin"] = pins.touchChin;
    pins_obj["i2s_bclk"] = pins.i2sBclk;
    pins_obj["i2s_lrc"] = pins.i2sLrc;
    pins_obj["i2s_do"] = pins.i2sDo;
    pins_obj["i2s_di"] = pins.i2sDi;
    pins_obj["led"] = pins.ledPin;
    pins_obj["vbat"] = pins.vbatPin;
    pins_obj["vibro_left"] = pins.vibroLeft;
    pins_obj["vibro_right"] = pins.vibroRight;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// =============================================================================
// JSON PARSING
// =============================================================================

bool Settings::fromJson(const String& json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("{\"status\":\"error\",\"msg\":\"JSON parse error: %s\"}\n", err.c_str());
        return false;
    }
    
    bool persistentChanged = false;
    bool dynamicChanged = false;
    
    // Bot name
    if (doc.containsKey("bot_name")) {
        strncpy(botName, doc["bot_name"].as<const char*>(), sizeof(botName) - 1);
        dynamicChanged = true;
    }
    
    // Face settings (Dynamic)
    if (doc.containsKey("face")) {
        JsonObject f = doc["face"];
        if (f.containsKey("width")) face.width = f["width"];
        if (f.containsKey("height")) face.height = f["height"];
        if (f.containsKey("radius")) face.radius = f["radius"];
        if (f.containsKey("spacing")) face.spacing = f["spacing"];
        if (f.containsKey("auto_blink")) face.autoBlink = f["auto_blink"];
        if (f.containsKey("idle_mode")) face.idleMode = f["idle_mode"];
        if (f.containsKey("blink_interval")) face.blinkInterval = f["blink_interval"];
        if (f.containsKey("idle_interval")) face.idleInterval = f["idle_interval"];
        if (f.containsKey("contrast")) face.contrast = f["contrast"];
        if (f.containsKey("curious")) face.curious = f["curious"];
        if (f.containsKey("sweat")) face.sweat = f["sweat"];
        dynamicChanged = true;
        applyFaceSettings();
    }
    
    // Audio settings (Dynamic)
    if (doc.containsKey("audio")) {
        JsonObject a = doc["audio"];
        if (a.containsKey("volume")) audio.volume = a["volume"];
        if (a.containsKey("mic_log")) audio.micLogEnabled = a["mic_log"];
        dynamicChanged = true;
        applyAudioSettings();
    }
    
    // Haptic settings (Dynamic)
    if (doc.containsKey("haptic")) {
        JsonObject h = doc["haptic"];
        if (h.containsKey("intensity")) haptic.intensity = h["intensity"];
        dynamicChanged = true;
        applyHapticSettings();
    }
    
    // LED settings (Dynamic)
    if (doc.containsKey("led")) {
        JsonObject l = doc["led"];
        if (l.containsKey("brightness")) led.brightness = l["brightness"];
        if (l.containsKey("r")) led.r = l["r"];
        if (l.containsKey("g")) led.g = l["g"];
        if (l.containsKey("b")) led.b = l["b"];
        dynamicChanged = true;
        applyLEDSettings();
    }
    
    // Motion settings (Persistent - requires reboot)
    if (doc.containsKey("motion")) {
        JsonObject m = doc["motion"];
        if (m.containsKey("tilt_deg")) motion.tiltDeg = m["tilt_deg"];
        if (m.containsKey("shake_angry_dps")) motion.shakeAngryDps = m["shake_angry_dps"];
        if (m.containsKey("shake_furious_dps")) motion.shakeFuriousDps = m["shake_furious_dps"];
        if (m.containsKey("tap_spike_dps")) motion.tapSpikeDps = m["tap_spike_dps"];
        persistentChanged = true;
    }
    
    // Power settings (Persistent - requires reboot)
    if (doc.containsKey("power")) {
        JsonObject p = doc["power"];
        if (p.containsKey("idle_timeout_ms")) power.idleTimeoutMs = p["idle_timeout_ms"];
        if (p.containsKey("sleep_timeout_ms")) power.sleepTimeoutMs = p["sleep_timeout_ms"];
        persistentChanged = true;
    }
    
    // Pin settings (Persistent - requires reboot)
    if (doc.containsKey("pins")) {
        JsonObject pins_obj = doc["pins"];
        if (pins_obj.containsKey("i2c_sda")) pins.i2cSda = pins_obj["i2c_sda"];
        if (pins_obj.containsKey("i2c_scl")) pins.i2cScl = pins_obj["i2c_scl"];
        if (pins_obj.containsKey("func_btn")) pins.funcBtn = pins_obj["func_btn"];
        if (pins_obj.containsKey("touch_chin")) pins.touchChin = pins_obj["touch_chin"];
        if (pins_obj.containsKey("i2s_bclk")) pins.i2sBclk = pins_obj["i2s_bclk"];
        if (pins_obj.containsKey("i2s_lrc")) pins.i2sLrc = pins_obj["i2s_lrc"];
        if (pins_obj.containsKey("i2s_do")) pins.i2sDo = pins_obj["i2s_do"];
        if (pins_obj.containsKey("i2s_di")) pins.i2sDi = pins_obj["i2s_di"];
        if (pins_obj.containsKey("led")) pins.ledPin = pins_obj["led"];
        if (pins_obj.containsKey("vbat")) pins.vbatPin = pins_obj["vbat"];
        if (pins_obj.containsKey("vibro_left")) pins.vibroLeft = pins_obj["vibro_left"];
        if (pins_obj.containsKey("vibro_right")) pins.vibroRight = pins_obj["vibro_right"];
        persistentChanged = true;
    }
    
    // Check for "save" flag (default: true)
    bool shouldSave = true;
    if (doc.containsKey("save")) {
        shouldSave = doc["save"];
    }

    // Save and flag reboot if persistent changed
    if (persistentChanged) {
        pendingReboot = true;
        if (shouldSave) save();
        Serial.println("{\"status\":\"ok\",\"msg\":\"Persistent settings changed. Reboot required.\"}");
    } else if (dynamicChanged) {
        if (shouldSave) save();
        Serial.println("{\"status\":\"ok\",\"msg\":\"Dynamic settings applied.\"}");
    } else {
        Serial.println("{\"status\":\"ok\",\"msg\":\"No changes detected.\"}");
    }
    
    return true;
}
