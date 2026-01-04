#include "ConfigManager.h"
#include <ArduinoJson.h>
#include "config.h" 

DogeSettings settings;
Preferences ConfigManager::prefs;

void ConfigManager::begin() {
    prefs.begin("doge-config", false);
    if (!prefs.isKey("init")) {
        resetToDefaults();
        prefs.putBool("init", true);
    } else {
        load();
    }
}

void ConfigManager::load() {
    if (prefs.getBytesLength("blob") != sizeof(DogeSettings)) {
        resetToDefaults();
        return;
    }
    prefs.getBytes("blob", &settings, sizeof(DogeSettings));
}

void ConfigManager::save() {
    prefs.putBytes("blob", &settings, sizeof(DogeSettings));
}

void ConfigManager::resetToDefaults() {
    strcpy(settings.bot_name, DEVICE_NAME);
    
    settings.eye.width = DEFAULT_EYE_WIDTH;
    settings.eye.height = DEFAULT_EYE_HEIGHT;
    settings.eye.radius = DEFAULT_EYE_BORDER_RADIUS;
    settings.eye.spacing = DEFAULT_EYE_SPACING;
    settings.eye.auto_blink = DEFAULT_EYE_AUTO_BLINK;
    settings.eye.idle_mode = DEFAULT_EYE_IDLE_MODE;
    settings.eye.sweat = false;
    settings.eye.cyclops = false;
    settings.eye.blink_int = DEFAULT_BLINK_INTERVAL;
    settings.eye.idle_int = DEFAULT_IDLE_INTERVAL;
    
    settings.motion.tilt = DEFAULT_TILT_THRESHOLD_DEG;
    settings.motion.shake = DEFAULT_SHAKE_ANGRY_DPS;
    settings.motion.furious = DEFAULT_SHAKE_FURIOUS_DPS;
    settings.motion.tap = DEFAULT_TAP_SPIKE_DPS;
    
    settings.power.idle_ms = DEFAULT_IDLE_TIMEOUT_MS;
    settings.power.sleep_ms = DEFAULT_SLEEP_TIMEOUT_MS;
    
    settings.audio.vol = DEFAULT_AUDIO_VOLUME;
    settings.audio.mic_thresh = MIC_CHANGE_THRESHOLD_DB;
    
    settings.led.brightness = LED_BRIGHTNESS;
    settings.led.r = LED_COLOR_R;
    settings.led.g = LED_COLOR_G;
    settings.led.b = LED_COLOR_B;
    
    settings.screen.contrast = DEFAULT_OLED_CONTRAST;
    settings.screen.flip = false;
    
    settings.haptic_int = DEFAULT_HAPTIC_INTENSITY;
    
    save();
}

String ConfigManager::getJson() {
    StaticJsonDocument<1024> doc;
    doc["bot_name"] = settings.bot_name;
    
    JsonObject eye = doc.createNestedObject("eye");
    eye["width"] = settings.eye.width;
    eye["height"] = settings.eye.height;
    eye["radius"] = settings.eye.radius;
    eye["spacing"] = settings.eye.spacing;
    eye["auto_blink"] = settings.eye.auto_blink;
    eye["idle_mode"] = settings.eye.idle_mode;
    eye["sweat"] = settings.eye.sweat;
    eye["cyclops"] = settings.eye.cyclops;
    eye["blink_int"] = settings.eye.blink_int;
    eye["idle_int"] = settings.eye.idle_int;
    
    JsonObject mot = doc.createNestedObject("motion");
    mot["tilt"] = settings.motion.tilt;
    mot["shake"] = settings.motion.shake;
    mot["furious"] = settings.motion.furious;
    mot["tap"] = settings.motion.tap;
    
    JsonObject pwr = doc.createNestedObject("power");
    pwr["idle_ms"] = settings.power.idle_ms;
    pwr["sleep_ms"] = settings.power.sleep_ms;
    
    JsonObject aud = doc.createNestedObject("audio");
    aud["vol"] = settings.audio.vol;
    aud["mic_t"] = settings.audio.mic_thresh;
    
    JsonObject led = doc.createNestedObject("led");
    led["bri"] = settings.led.brightness;
    led["r"] = settings.led.r;
    led["g"] = settings.led.g;
    led["b"] = settings.led.b;
    
    JsonObject scr = doc.createNestedObject("screen");
    scr["con"] = settings.screen.contrast;
    scr["flip"] = settings.screen.flip;
    
    doc["haptic_int"] = settings.haptic_int;
    
    String output;
    serializeJson(doc, output);
    return output;
}

bool ConfigManager::applyJson(String json) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) return false;
    
    if (doc.containsKey("bot_name")) strncpy(settings.bot_name, doc["bot_name"], 31);
    
    if (doc["eye"].is<JsonObject>()) {
        JsonObject eye = doc["eye"];
        if (eye.containsKey("width")) settings.eye.width = eye["width"];
        if (eye.containsKey("height")) settings.eye.height = eye["height"];
        if (eye.containsKey("radius")) settings.eye.radius = eye["radius"];
        if (eye.containsKey("spacing")) settings.eye.spacing = eye["spacing"];
        if (eye.containsKey("auto_blink")) settings.eye.auto_blink = eye["auto_blink"];
        if (eye.containsKey("idle_mode")) settings.eye.idle_mode = eye["idle_mode"];
        if (eye.containsKey("sweat")) settings.eye.sweat = eye["sweat"];
        if (eye.containsKey("cyclops")) settings.eye.cyclops = eye["cyclops"];
        if (eye.containsKey("blink_int")) settings.eye.blink_int = eye["blink_int"];
        if (eye.containsKey("idle_int")) settings.eye.idle_int = eye["idle_int"];
    }
    
    if (doc["motion"].is<JsonObject>()) {
        JsonObject mot = doc["motion"];
        if (mot.containsKey("tilt")) settings.motion.tilt = mot["tilt"];
        if (mot.containsKey("shake")) settings.motion.shake = mot["shake"];
        if (mot.containsKey("furious")) settings.motion.furious = mot["furious"];
        if (mot.containsKey("tap")) settings.motion.tap = mot["tap"];
    }
    
    if (doc["power"].is<JsonObject>()) {
        JsonObject pwr = doc["power"];
        if (pwr.containsKey("idle_ms")) settings.power.idle_ms = pwr["idle_ms"];
        if (pwr.containsKey("sleep_ms")) settings.power.sleep_ms = pwr["sleep_ms"];
    }
    
    if (doc["audio"].is<JsonObject>()) {
        JsonObject aud = doc["audio"];
        if (aud.containsKey("vol")) settings.audio.vol = aud["vol"];
        if (aud.containsKey("mic_t")) settings.audio.mic_thresh = aud["mic_t"];
    }
    
    if (doc["led"].is<JsonObject>()) {
        JsonObject led = doc["led"];
        if (led.containsKey("bri")) settings.led.brightness = led["bri"];
        if (led.containsKey("r")) settings.led.r = led["r"];
        if (led.containsKey("g")) settings.led.g = led["g"];
        if (led.containsKey("b")) settings.led.b = led["b"];
    }
    
    if (doc["screen"].is<JsonObject>()) {
        JsonObject scr = doc["screen"];
        if (scr.containsKey("con")) settings.screen.contrast = scr["con"];
        if (scr.containsKey("flip")) settings.screen.flip = scr["flip"];
    }
    
    if (doc.containsKey("haptic_int")) settings.haptic_int = doc["haptic_int"];
    
    save();
    return true;
}
