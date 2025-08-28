// Author: [Doge De Shibe]
// Description: ESP32-S3 Super Mini companion bot with animated eyes, motion-reactive emotions

// =============================================================================
// INCLUDES & DEPENDENCIES
// =============================================================================
#include <Arduino.h>
#include <stdint.h>  // Standard integer types - must come early

// Ensure stdint types are available for linter and fix delay() issues
#if !defined(__STDINT_H) && !defined(_STDINT_H_)
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long int32_t;
typedef short int16_t;
#endif

// Ensure delay function is available
#ifndef delay
#define delay(x) ::delay(x)
#endif

#include <Wire.h>
#include <esp_system.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include <FS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>
#include <ChronosESP32.h>


// Custom headers
#include "icon/icons_auto.h"
#include "util.h"
#include "config.h"

// Sectors library (new modular structure)
#include "sectors/src/Sectors.h"


// =============================================================================
// GLOBAL VARIABLES & CONSTANTS
// =============================================================================
// Forward-declare enums to avoid Arduino prototype generation issues
enum FaceMode : uint8_t;

// MoodState enum (moved from motion.h)
enum MoodState : uint8_t { MS_DEFAULT, MS_HAPPY, MS_ANGRY, MS_FURIOUS, MS_TIRED };
// === Toast overlay (non-blocking notifications) ===
// Fixed-size buffers for lightweight typewriter/scatter overlay
char  toastText[500];      // visible portion (typewriter)
char  toastFullText[500];  // full text to be displayed
uint32_t toastUntil = 0;
bool toastVisible = false; // Track if toast is currently visible to reduce flicker
uint32_t lastToastDrawMs = 0; // Prevent too frequent toast updates
bool displayNeedsUpdate = false; // Track if display content has changed
// Typewriter effect variables
bool toastTypewriter = false; // Enable typewriter effect
uint8_t toastTypePos = 0; // Current position in typewriter
uint32_t toastTypeSpeed = 100; // ms between characters
uint32_t lastToastTypeMs = 0; // Last typewriter update time
// Binary scatter overlay flags (exported)
bool toastScatter = false;     // when true, draw each char at random positions
bool toastNoFrame = false;     // when true, skip banner frame/box

inline void showToastTypewriter(const String& s, uint16_t ms=3000, uint32_t typeSpeed=50) {
  // Copy string to full text buffer
  s.toCharArray(toastFullText, sizeof(toastFullText));
  toastFullText[sizeof(toastFullText)-1] = '\0'; // Ensure null termination
  toastText[0] = '\0'; // Clear current display text
  // Ensure the toast stays long enough to finish typing plus a grace period
  size_t len = strnlen(toastFullText, sizeof(toastFullText)-1);
  uint32_t minMs = (uint32_t)len * typeSpeed + 2000; // typing time + 2s grace
  uint32_t dur = (uint32_t)ms;
  if (dur < minMs) dur = minMs;
  // Cap to a reasonable upper bound to avoid runaway durations
  if (dur > 45000) dur = 45000; // 45s max
  toastUntil = millis() + (uint32_t)dur;
  toastVisible = true;
  toastTypewriter = true; // Enable typewriter effect
  toastTypePos = 0; // Start from beginning
  toastTypeSpeed = typeSpeed;
  lastToastTypeMs = millis();
  displayNeedsUpdate = true;
}

inline void showToast(const String& s, uint16_t ms=1200) {
  // Standard toasts: ensure banner mode (no scatter) and frame enabled
  toastScatter = false;
  toastNoFrame = false;
  showToastTypewriter(s, ms, 40); // Fast typewriter (40ms between chars)
}

// Generate binary sequence for chatter communication
String generateChatterBinary() {
  String binary = "";
  
  // Different binary patterns for variety
  int pattern = random(0, 4);
  
  switch (pattern) {
    case 0: // Classic binary groups
      {
        int groups = random(3, 5); // 3-4 groups
        for (int g = 0; g < groups; g++) {
          if (g > 0) binary += " "; // Space between groups
          int bits = random(4, 7); // 4-6 bits per group
          for (int b = 0; b < bits; b++) {
            binary += (random(0, 2) == 0) ? "0" : "1";
          }
        }
      }
      break;
      
    case 1: // ASCII-like patterns (common binary values)
      {
        const char* patterns[] = {"01001000", "01100101", "01101100", "01101111", "01011111"};
        int count = random(2, 4);
        for (int i = 0; i < count; i++) {
          if (i > 0) binary += " ";
          binary += patterns[random(0, 5)];
        }
      }
      break;
      
    case 2: // Header-like pattern
      binary = "10101011 ";
      for (int i = 0; i < random(2, 4); i++) {
        for (int b = 0; b < 8; b++) {
          binary += (random(0, 2) == 0) ? "0" : "1";
        }
        if (i < 2) binary += " ";
      }
      break;
      
    case 3: // Mixed short/long groups
      {
        int totalGroups = random(3, 6);
        for (int g = 0; g < totalGroups; g++) {
          if (g > 0) binary += " ";
          int bits = (g % 2 == 0) ? random(3, 5) : random(6, 9); // Alternate short/long
          for (int b = 0; b < bits; b++) {
            binary += (random(0, 2) == 0) ? "0" : "1";
          }
        }
      }
      break;
  }
  
  return binary;
}

// Show binary chatter communication
inline void showBinaryChatter() {
  String binaryMsg = generateChatterBinary();
  // Enable scatter + no frame for binary chatter
  toastScatter = true;
  toastNoFrame = true;
  showToastTypewriter(binaryMsg, 4000, 60); // Faster typing for more responsive feel
}

// =============================================================================
// HARDWARE CONFIGURATION
// =============================================================================
// Hardware object instances
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

// OLED Display configuration moved to config.h
Adafruit_SH1106G display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
// Make display globally accessible to sectors
Adafruit_SH1106G* oled_display = &display;

// Custom fonts generated by tools/font_to_header.py
// Time (HH:MM:SS)
#include "include/TechTime37.h"
// Numbers for date & battery
#include "include/Pixeboy20.h"
// Notification (Vietnamese-capable)
#include "include/ShopeeRegular12.h"
// Toast with emoji support
#include "include/ToastRenderer.h"

// =============================================================================
// GLOBAL CONSTANTS & CONFIGURATION
// =============================================================================


// === Frame Layout (shared by clock & notification) ===
const int FRAME_X = 0;                 // left margin
const int FRAME_Y = 0;                 // top margin
const int FRAME_W = SCREEN_W - 0;      // frame width
const int FRAME_H = 64;                // frame height
const uint8_t FRAME_RADIUS = 8;        // corner radius (UI)
const uint8_t FRAME_THICKNESS = 2;     // border thickness (UI)

// === Battery Sensing === (values come from config.h)

// === Motion/Emotion Constants ===
#define DEFAULT_SHAKING false
#define DEFAULT_FURIOUS_SHAKING false
#define DEFAULT_JIGGLING false
#define DEFAULT_FURIOUS_JIGGLING false
#define MOOD_HOLD_MS 2000  // How long temporary moods last

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

// === Emotion & Motion State ===
uint32_t lastMoveMs = 0;        // when significant motion last happened
uint32_t moodUntil  = 0;        // temporary mood timeout
uint32_t shakeStart = 0;        // for sustained shake
uint32_t furiousStart = 0;      // for furious shake detection
bool     shaking    = DEFAULT_SHAKING;
bool     furiousShaking = DEFAULT_FURIOUS_SHAKING;

// === Timing Variables ===
static uint32_t lastDisplayUpdateMs = 0;
static uint32_t lastRoboEyesUpdateMs = 0;

// === Power Management ===
static bool isSleeping = false;
static bool isDimmed = false;
static uint32_t lastActivityMs = 0;
static bool isLazy = false;
static uint32_t nextLazyJingleMs = 0;
// WiFi config portal moved into Sectors::Portal

// ===== SPIFFS helpers =====
// SPIFFS init & portal content handled by Sectors::Portal

// Forward declaration for notification popup
static uint32_t notifPopupUntil = 0;

// Notification scroll state variables
static int16_t notifScrollX = 0;
static uint32_t notifScrollT0 = 0;

// === Furious Jiggle System ===
uint32_t furiousJiggleEndMs = 0;
bool     furiousJiggling     = DEFAULT_FURIOUS_JIGGLING;

// === Regular Jiggle System ===
uint32_t jiggleNextMs = 0;
uint32_t jiggleEndMs  = 0;
bool     jiggling     = DEFAULT_JIGGLING;

// === Low Pass Filtering ===
float lpf_ax = 0, lpf_ay = 0, lpf_az = 0, lpf_g = 0;

// === Battery sense cache ===
uint32_t lastVbatReadMs = 0;
float    vbatVolts = 0.0f;
int      vbatPercent = -1; // -1 when unknown
bool     batteryCharging = false; // true when VBAT sustained high
uint8_t  vbatChargeCount = 0;     // consecutive high-read counter
// Chatty/silent state
bool            silentMode = false;
uint8_t         talkativeLevel = BOT_TALKATIVE_LEVEL;   // runtime talk level (0..5)
uint8_t         savedTalkativeLevel = BOT_TALKATIVE_LEVEL; // last non-zero level
// Touch edge tracker to improve responsiveness on non-eyes faces
static uint32_t lastTouchEdgeMs = 0;

// === Gemini AI Integration === (moved to ai_companion module)
// AI state variables now managed by AICompanion namespace

// === Debug System Variables ===
static float last_ax = 0, last_ay = 0, last_az = 0;
static float last_gx = 0, last_gy = 0, last_gz = 0;

uint8_t volume = AUDIO_DEFAULT_VOLUME;         // 0..255
uint8_t savedVolume = AUDIO_DEFAULT_VOLUME;    // restore after silent mode

// =============================================================================
// DISPLAY SYSTEM (RoboEyes via Sectors)
// =============================================================================

// Control RoboEyes flushing/viewport so HUD/toast can overlay without flicker
bool gEyesAutoFlush = true;   // default true; disabled while toast is visible
int  gEyesViewportYMax = 0;   // 0 = full screen, else max Y (exclusive)

// Thin wrappers for backward compatibility with existing code
void Eyes_SetMood(uint8_t mood) { Sectors::Display::setMood(mood); }
void Eyes_SetHFlicker(bool on, int level) { /* TODO: Implement in Display sector */ }
void Eyes_SetVFlicker(bool on, int level) { /* TODO: Implement in Display sector */ }
void Eyes_Blink() { 
  Sectors::Display::blink(); 
  // Play blink SFX if enabled and not silent
  if (ENABLE_TAP_SFX && !silentMode) {
    Sectors::Voice::sfxBlink();
  }
}

// Additional wrappers for animation engine
void Eyes_SetPosition(unsigned char position) { Sectors::Display::setPosition(position); }
void Eyes_Open() { Sectors::Display::open(); }

// Mood mapping from MoodState to Display constants
void setEyesMood(MoodState m) {
  switch (m) {
    case MS_DEFAULT: Sectors::Display::setMood(Sectors::Display::MOOD_DEFAULT); break;
    case MS_HAPPY:   Sectors::Display::setMood(Sectors::Display::MOOD_HAPPY); break;
    case MS_ANGRY:   Sectors::Display::setMood(Sectors::Display::MOOD_ANGRY); break;
    case MS_FURIOUS: Sectors::Display::setMood(Sectors::Display::MOOD_ANGRY); break; // Map furious to angry
    case MS_TIRED:   Sectors::Display::setMood(Sectors::Display::MOOD_TIRED); break;
  }
}

// REMOVED: Furious jiggle variables moved to GLOBAL VARIABLES section above

void setupRoboEyes() {
  // RoboEyes initialization now handled by Sectors::Display::begin() in Sectors::beginAll()
  Sectors::Display::setWidth(28, 28);
  Sectors::Display::setHeight(40, 40);
  Sectors::Display::setBorderradius(8, 8);
  Sectors::Display::setSpacebetween(10);
  // Give the internal idle engine room to roam
  Sectors::Display::setAutoblinker(true, 3, 4);  // avg ~3s with ±4s jitter
  Sectors::Display::setIdleMode(true, 4, 5);     // moderate wander frequency & randomness
  Sectors::Display::setMood(Sectors::Display::MOOD_DEFAULT);
  Sectors::Display::setCuriosity(true);
}

// =============================================================================
// MOTION PROCESSING & EMOTION SYSTEM
// =============================================================================

// IMU calibration offsets (extern declarations from mpu6050.h)
int32_t mpu_accel_off_x=0, mpu_accel_off_y=0, mpu_accel_off_z=0;
int32_t mpu_gyro_off_x=0, mpu_gyro_off_y=0, mpu_gyro_off_z=0;

// Emotion system (now handled by Sectors)
// Current mood tracking for local logic (could be moved to Sectors::Status later)
MoodState curMood = MS_DEFAULT;

// Note: keep the status LED behavior separate; do not change it when mood changes.
// Motion/emotion updates now handled by Sectors::MotionSector

// ===== Liveliness updater (~25Hz) =====

// ===== Liveliness / IMU reaction (now handled by Sectors::MotionSector) =====
// Notification attention animation (non-blocking state machine)
static uint8_t notifAnimState = 0; // 0=idle,1=widen,2=look,3=blink,4=restore
static uint32_t notifAnimT0 = 0;
int notif_orig_wL=0, notif_orig_wR=0, notif_orig_hL=0, notif_orig_hR=0;
MoodState notif_prevMood = MS_DEFAULT;

// helpers: trigger attention animation (called from notification callback)
inline void triggerNotificationAttention() {
  // save original default sizes (avoid capturing "1" mid-blink)
  notif_orig_wL = Eyes.eyeLwidthDefault; notif_orig_wR = Eyes.eyeRwidthDefault;
  notif_orig_hL = Eyes.eyeLheightDefault; notif_orig_hR = Eyes.eyeRheightDefault;
  notif_prevMood = Sectors::Animations::getCurrentMood();
  // start animation
  notifAnimState = 1; notifAnimT0 = millis();
  // quick widen (touch only "Next" targets; do not change defaults)
  Eyes.eyeLwidthNext = notif_orig_wL + 8; Eyes.eyeRwidthNext = notif_orig_wR + 8;
  Eyes.eyeLheightNext = notif_orig_hL + 6; Eyes.eyeRheightNext = notif_orig_hR + 6;
}

inline void updateNotificationAnim() {
  if (notifAnimState == 0) return;
  uint32_t now = millis();
  switch (notifAnimState) {
    case 1: // widened -> after short time, look toward NE then blink
      if (now - notifAnimT0 > 180) {
        Eyes.setPosition(NE);
        notifAnimState = 2; notifAnimT0 = now;
      }
      break;
    case 2: // hold look then blink
      if (now - notifAnimT0 > 220) {
        Eyes.blink();
        // also smile briefly
        Sectors::Animations::setMood(MS_HAPPY); moodUntil = now + 800;
        notifAnimState = 3; notifAnimT0 = now;
      }
      break;
    case 3: // restore sizes and mood
      if (now - notifAnimT0 > 180) {
        // Restore only the "Next" sizes, keep defaults untouched to avoid side-effects
        Eyes.eyeLwidthNext = notif_orig_wL; Eyes.eyeRwidthNext = notif_orig_wR;
        Eyes.eyeLheightNext = notif_orig_hL; Eyes.eyeRheightNext = notif_orig_hR;
        // Ensure eyes are opened in case a blink was mid-flight while not rendering eyes
        Eyes.open();
        Sectors::Animations::setMood(notif_prevMood);
        notifAnimState = 0;
      }
      break;
  }
}

// small micro-expression helpers
inline void doWinkLeft(){ Eyes.blink(true, false); }
inline void doWinkRight(){ Eyes.blink(false, true); }
inline void doSmile(uint16_t ms=700){ Sectors::Animations::setMood(MS_HAPPY); moodUntil = millis() + ms; } // Single call - acceptable


// =============================================================================
// FACE MODES & BLE SYSTEM
// =============================================================================

// Available face modes
enum FaceMode : uint8_t { FACE_EYES=0, FACE_CLOCK=1, FACE_NOTIF=2, FACE_COUNT };
FaceMode mode = FACE_EYES;

// BLE / Chronos time synchronization
ChronosESP32 chrono(BLE_DEVICE_NAME);
bool bleEnabled = true;

// notif cache - optimized to fixed char arrays
char lastNotifTitle[64];
char lastNotifBody[128];

// Utility functions
static String zeroPad2(int v){ char b[3]; snprintf(b, sizeof(b), "%02d", v); return String(b); }

// Battery helpers
// Battery functions moved to clock.cpp

// =============================================================================
// INPUT HANDLING & USER INTERFACE
// =============================================================================
static bool edgeRising(uint8_t pin, bool activeHigh=true, uint16_t debounceMs=30) {
  static uint32_t t0=0; static bool lastRaw=false, stable=false;
  uint32_t currentMs = millis();  // Cache current time to avoid multiple millis() calls
  bool raw = activeHigh ? (digitalRead(pin)==HIGH) : (digitalRead(pin)==LOW);
  if (raw != lastRaw) { lastRaw = raw; t0 = currentMs; }
  if (currentMs - t0 > debounceMs) {
    if (raw != stable) { bool old=stable; stable=raw; return (stable && !old); }
  }
  return false;
}

// Detect triple-tap on a digital input (rising edges) within a time window
static bool tripleTap(uint8_t pin, bool activeHigh=true, uint16_t debounceMs=12, uint16_t windowMs=650) {
  static uint8_t  tapCount = 0;
  static uint32_t windowStart = 0;
  if (edgeRising(pin, activeHigh, debounceMs)) {
    uint32_t now = millis();
    lastTouchEdgeMs = now;
    if (tapCount == 0 || (now - windowStart) > windowMs) {
      tapCount = 1; windowStart = now; return false;
    }
    tapCount++;
    if (tapCount >= 3) { tapCount = 0; return true; }
  }
  // expire window
  if (tapCount > 0 && (millis() - windowStart) > windowMs) tapCount = 0;
  return false;
}

// FUNC button controls (non-blocking)
enum BleUiState { BLEUI_IDLE, BLEUI_PROMPT, BLEUI_ENABLING, BLEUI_DISABLING, BLEUI_DONE };
BleUiState bleUi = BLEUI_IDLE;
uint32_t   bleUiT0 = 0;
uint32_t   btnDownT0 = 0;
bool        wifiEnabled = false;  // Track WiFi state
// HOLD_TIME_MS comes from config.h

// === AI and Animation functions moved to separate modules ===
// See ai_companion.cpp and animation_engine.cpp

// Detect double press on FUNC button
bool detectDoublePress() {
  static uint32_t lastPressTime = 0;
  static uint32_t pressCount = 0;
  static bool wasPressed = false;

  bool isPressed = (digitalRead(FUNC_BTN) == LOW);
  uint32_t currentTime = millis();

  if (isPressed && !wasPressed) {
    // Button just pressed
    if (currentTime - lastPressTime < 500) { // 500ms window for double press
      pressCount++;
    } else {
      pressCount = 1;
    }
    lastPressTime = currentTime;
  }

  wasPressed = isPressed;

  // Check for double press
  if (pressCount >= 2 && (currentTime - lastPressTime > 100)) {
    pressCount = 0;
    return true;
  }

  return false;
}

// Detect triple-press on FUNC button to toggle WiFi config portal
bool detectTriplePressFUNC() {
  static uint32_t lastPressTime = 0;
  static uint8_t pressCount = 0;
  static bool wasPressed = false;
  const uint32_t windowMs = 900;

  bool isPressed = (digitalRead(FUNC_BTN) == LOW);
  uint32_t now = millis();
  if (isPressed && !wasPressed) {
    if (now - lastPressTime < windowMs) {
      pressCount++;
    } else {
      pressCount = 1;
    }
    lastPressTime = now;
  }
  wasPressed = isPressed;
  if (pressCount >= 3 && (now - lastPressTime > 100)) { pressCount = 0; return true; }
  // expire window
  if (pressCount > 0 && (now - lastPressTime > windowMs)) pressCount = 0;
  return false;
}

// Config portal handlers moved to Sectors::Portal

void updateBleToggleUI() {
  bool btn = (digitalRead(FUNC_BTN)==LOW);

  // Priority: triple press toggles WiFi config portal
  if (detectTriplePressFUNC()) {
    if (!Sectors::Portal::isActive()) {
      // Disable WiFi and start AP portal via sector
      wifiEnabled = false;
      Sectors::Portal::start();
      showToast("Setup AP: DogePet-Setup", 3000);
    } else {
      bool ok = Sectors::Portal::stopAndConnect(WIFI_CONNECT_TIMEOUT_MS);
      wifiEnabled = ok;
      showToast(ok ? "WiFi Connected!" : "WiFi Failed", 2000);
    }
    return;
  }

  // Check for double press (WiFi toggle) first
  if (detectDoublePress()) {
    if (ENABLE_WIFI && ENABLE_GEMINI_AI) {
      wifiEnabled = !wifiEnabled;
      if (wifiEnabled) {
        Serial.println("WiFi: ENABLING");
        showToast("WiFi: ENABLING", 2000);
        WiFi.mode(WIFI_STA);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

        // Wait for connection with timeout
        unsigned long wifiStartTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - wifiStartTime) < WIFI_CONNECT_TIMEOUT_MS) {
          delay(500);
          Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("\nWiFi connected!");
          Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
          showToast("WiFi Connected! 📶", 2000);

          // Re-initialize AI via Sectors if WiFi is now available
          if (ENABLE_GEMINI_AI && GEMINI_API_KEY && strlen(GEMINI_API_KEY) > 0) {
            Sectors::beginAll(GEMINI_API_KEY); // Re-init with API key
            Serial.println("Gemini AI enabled - Send messages with 'AI:', '@doge', or 'DogePet:' prefix");
            showToast("AI Ready! 🤖✨", 2000);
          }
        } else {
          Serial.println("\nWiFi connection failed!");
          showToast("WiFi Failed 😢", 3000);
          wifiEnabled = false; // Reset flag on failure
        }
      } else {
        Serial.println("WiFi: DISABLING");
        showToast("WiFi: DISABLED", 2000);
        WiFi.disconnect(true);
        // AI will be disabled automatically due to no WiFi
      }
      Sectors::Voice::sfxConfirm();
    }
    return; // Don't process as BLE toggle
  }

  // If the face is active, keep BLE toggle completely minimal: no screen
  // updates or flashing. We still respect the hold time but only toggle
  // the internal BLE state and optionally print to Serial.
  if (mode == FACE_EYES) {
    static bool holding = false;
    if (btn && !holding) { holding = true; btnDownT0 = millis(); }
    if (!btn && holding) { holding = false; }
    if (holding) {
      uint32_t held = millis() - btnDownT0;
      if (held >= HOLD_TIME_MS) {
        // toggle BLE quietly and update status LED
        if (!bleEnabled) {
          bleEnabled = true;
          chrono.begin();
          Serial.println("BLE: ENABLING");
          strip.setPixelColor(0, strip.Color(0,0,255));
          strip.show();
          Sectors::Voice::sfxConfirm();
        } else {
          bleEnabled = false;
          chrono.stop(true);
          Serial.println("BLE: DISABLING");
          strip.setPixelColor(0, 0);
          strip.show();
          Sectors::Voice::sfxError();
        }
        holding = false;
      }
    }
    return;
  }

  // Non-face modes: show the normal minimal UI to indicate BLE state.
  switch (bleUi) {
    case BLEUI_IDLE:
      if (btn) { btnDownT0 = millis(); bleUi = BLEUI_PROMPT; }
      break;

    case BLEUI_PROMPT: {
      // show countdown while held
      uint32_t held = millis() - btnDownT0;
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE); display.setTextSize(1);
      display.setCursor(0,20); display.println("Hold FUNC_BTN");
      display.setCursor(0,32); display.println("for BLE toggle");
      display.setCursor(0,44); display.print("Release in: ");
      display.print( (HOLD_TIME_MS > held) ? ( (HOLD_TIME_MS - held + 999)/1000 ) : 0 );
      display.print("s");
      display.display();

      if (!btn) { bleUi = BLEUI_IDLE; break; }               // released early → cancel
      if (held >= HOLD_TIME_MS) {
        if (!bleEnabled) {
          bleEnabled = true; bleUi = BLEUI_ENABLING;  bleUiT0 = millis(); chrono.begin(); showToast("BLE: ENABLING");Serial.println("BLE: ENABLING");
          strip.setPixelColor(0, strip.Color(0,0,255)); strip.show();
          Sectors::Voice::sfxConfirm();
        }
        else {
          bleEnabled = false; bleUi = BLEUI_DISABLING; bleUiT0 = millis(); chrono.stop(true); showToast("BLE: DISABLING");Serial.println("BLE: DISABLING");
          strip.setPixelColor(0, 0); strip.show();
          Sectors::Voice::sfxError();
        }
      }
    } break;

    case BLEUI_ENABLING:
    case BLEUI_DISABLING: {
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE); display.setTextSize(1);
      display.setCursor(0,26);
      display.println(bleUi==BLEUI_ENABLING ? "BLE: ENABLING..." : "BLE: DISABLING...");
      display.display();
      if (millis()-bleUiT0 > 900) { bleUi = BLEUI_DONE; bleUiT0 = millis(); }
    } break;

    case BLEUI_DONE:
      display.clearDisplay();
      display.setTextColor(SH110X_WHITE); display.setTextSize(1);
      display.setCursor(0,26);
      showToast(bleEnabled ? "BLE: ENABLED" : "BLE: DISABLED");Serial.println(bleEnabled ? "BLE: ENABLED" : "BLE: DISABLED");
      display.display();
      if (millis()-bleUiT0 > 900) bleUi = BLEUI_IDLE;
      break;
  }
}

// =============================================================================
// DISPLAY RENDERING FUNCTIONS
// =============================================================================

// UI helper functions moved to individual modules

void drawCenterText(const String &l1, const String &l2="") {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE); display.setTextSize(1);
  int16_t x1,y1; uint16_t w,h;
  display.getTextBounds(l1,0,0,&x1,&y1,&w,&h);
  display.setCursor((SCREEN_W-w)/2, 16); display.println(l1);
  if (l2.length()) {
    display.getTextBounds(l2,0,0,&x1,&y1,&w,&h);
    display.setCursor((SCREEN_W-w)/2, 32); display.println(l2);
  }
  display.display();
}
// hasNotif() function moved to notification.cpp

// Power management: track activity and manage sleep/dim states
inline void updatePowerManagement() {
  uint32_t currentMs = millis();

  // Track activity from various sources
  if (lastMoveMs > lastActivityMs ||
      (millis() - lastTouchEdgeMs < 5000) ||  // recent touch
      hasNotif() ||                          // has notifications
      bleUi != BLEUI_IDLE ||                  // BLE UI active
      notifPopupUntil > currentMs) {          // popup active
    lastActivityMs = currentMs;
  }

  // Lazy mode entry
  if (ENABLE_LAZY_MODE) {
    if (!isLazy && (currentMs - lastActivityMs > LAZY_AFTER_MS)) {
      isLazy = true;
      // Close eyes, set calm mood
      Eyes.close();
      Sectors::Animations::setMood(MS_TIRED);
      // Silence chatter and mic reactions
      talkativeLevel = 0;
      // schedule first subtle jingle
      nextLazyJingleMs = currentMs + (uint32_t)(LAZY_JINGLE_MIN_MS + (esp_random() % (LAZY_JINGLE_MAX_MS - LAZY_JINGLE_MIN_MS + 1)));
    }
    // Wake on shake only
    if (isLazy) {
      // detect strong movement using existing flags
      if (shaking || furiousShaking) {
        isLazy = false;
        lastActivityMs = currentMs;
        Eyes.open();
        Sectors::Animations::setMood(MS_HAPPY);
      }
    }
  }

  // Check for dimming (LED only, display doesn't support brightness)
  if (!isSleeping && !isDimmed && (currentMs - lastActivityMs > DIM_AFTER_MS)) {
    isDimmed = true;
    if (ENABLE_LED_STATUS) {
      strip.setBrightness(SLEEP_BRIGHTNESS);
      strip.show();
    }
    Serial.println("LED dimmed for power saving");
  }
}

// Wake from sleep/dim on activity
inline void wakeUp() {
  if (isSleeping || isDimmed) {
    isSleeping = false;
    isDimmed = false;
    if (ENABLE_LED_STATUS) {
      strip.setBrightness(LED_BRIGHTNESS);
      strip.show();
    }
    displayNeedsUpdate = true; // Force display refresh after waking up
    Serial.println("Waking up from power saving mode");
  }
}

// Microphone noise detection and reaction
inline void checkMicrophoneNoise() {
  if (!ENABLE_MIC_LISTENING) return;

  static uint32_t lastNoiseReactionMs = 0;
  static uint32_t consecutiveNoiseCount = 0;

  // Check noise level periodically
  static uint32_t lastMicCheckMs = 0;
  uint32_t currentMs = millis();
  if (currentMs - lastMicCheckMs < MIC_CHECK_INTERVAL) return;
  lastMicCheckMs = currentMs;

  float micLevel = Sectors::Voice::getMicrophoneLevel();
  bool isLoud = Sectors::Voice::isLoudNoiseDetected();

  // Debug output (optional - remove for production)
  static uint32_t lastDebugMs = 0;
  if (currentMs - lastDebugMs > 1000) {
    bool audioPlaying = Sectors::Voice::isAudioPlaying();
    bool micCooldown = Sectors::Voice::isMicrophoneInCooldown();
    lastDebugMs = currentMs;
  }

  if (isLoud) {
    consecutiveNoiseCount++;
    wakeUp(); // Wake up on loud noise

    // Only react if enough time has passed since last reaction
    if (currentMs - lastNoiseReactionMs >= MIC_COOLDOWN_MS) {
      Serial.printf("Loud noise detected! Level: %.2f\n", micLevel);

      // If Gemini AI is connected, send voice trigger marker only (let AI module craft the prompt)
      if (ENABLE_GEMINI_AI && wifiEnabled) {
        Serial.println("[MIC DEBUG] Voice trigger -> AI (marker only)");
        const char* aiMarker = "[VOICE_TRIGGER]";
        Sectors::Brain::sendUserMessage(aiMarker);
      } else {
        // Fallback to original behavior when AI not available
        if (consecutiveNoiseCount >= 3) {
          // Multiple loud noises - get startled/annoyed
          Sectors::Animations::setMood(MS_ANGRY);
          moodUntil = currentMs + MOOD_HOLD_MS;
          if (ENABLE_MOOD_SFX) Sectors::Voice::playCuteNo(200);
          showToast("Hey! Too loud!", 1500);
        } else {
          // Single loud noise - curious reaction
          Sectors::Animations::setMood(MS_HAPPY);
          moodUntil = currentMs + MOOD_HOLD_MS / 2;
          if (ENABLE_MOOD_SFX) Sectors::Voice::playCuteQuestion(200);
          showToast("Huh? What was that?", 1200);
        }
      }

      // Reset consecutive count after reaction
      consecutiveNoiseCount = 0;

      lastNoiseReactionMs = currentMs;
    }
  } else {
    // Reset consecutive noise counter when it gets quiet
    if (consecutiveNoiseCount > 0) {
      consecutiveNoiseCount = 0;
    }
  }
}

// =============================================================================
// SERIAL CLI (diagnostics)
// =============================================================================

static String cliBuf;

static void cliPrintHelp() {
  Serial.println("\nDogePet CLI commands:");
  Serial.println("  help                - this help");
  Serial.println("  mood <default|happy|angry|furious|tired>");
  Serial.println("  blink               - blink eyes");
  Serial.println("  volume <0..255>     - set master volume");
  Serial.println("  say <confirm|error|notify|yes|no|question|startup>");
  Serial.println("  status              - print basic status");
}

static void cliHandleLine(const String& line) {
  String cmd = line; cmd.trim();
  if (cmd.length() == 0) return;
  if (cmd.equalsIgnoreCase("help")) { cliPrintHelp(); return; }

  if (cmd.startsWith("mood ")) {
    String m = cmd.substring(5); m.trim();
    if (m.equalsIgnoreCase("default")) setEyesMood(MS_DEFAULT);
    else if (m.equalsIgnoreCase("happy")) setEyesMood(MS_HAPPY);
    else if (m.equalsIgnoreCase("angry")) setEyesMood(MS_ANGRY);
    else if (m.equalsIgnoreCase("furious")) setEyesMood(MS_FURIOUS);
    else if (m.equalsIgnoreCase("tired")) setEyesMood(MS_TIRED);
    else Serial.println("Unknown mood");
    return;
  }

  if (cmd.equalsIgnoreCase("blink")) { Eyes_Blink(); return; }

  if (cmd.startsWith("volume ")) {
    int v = cmd.substring(7).toInt();
    if (v < 0) v = 0; if (v > 255) v = 255;
    volume = (uint8_t)v;
    Sectors::Voice::setMasterVolume(volume);
    Serial.printf("Volume set to %d\n", volume);
    return;
  }

  if (cmd.startsWith("say ")) {
    String s = cmd.substring(4); s.trim();
    if (s.equalsIgnoreCase("confirm")) Sectors::Voice::sfxConfirm();
    else if (s.equalsIgnoreCase("error")) Sectors::Voice::sfxError();
    else if (s.equalsIgnoreCase("notify")) Sectors::Voice::sfxNotify();
    else if (s.equalsIgnoreCase("yes")) Sectors::Voice::playCuteYes();
    else if (s.equalsIgnoreCase("no")) Sectors::Voice::playCuteNo();
    else if (s.equalsIgnoreCase("question")) Sectors::Voice::playCuteQuestion();
    else if (s.equalsIgnoreCase("startup")) Sectors::Voice::playCuteStartup();
    else Serial.println("Unknown sound");
    return;
  }

  if (cmd.equalsIgnoreCase("status")) {
    Serial.printf("WiFi: %s, BLE: %s\n", wifiEnabled ? "on" : "off", bleEnabled ? "on" : "off");
    Serial.printf("Volume: %u, Talkative: %u\n", volume, talkativeLevel);
    Serial.printf("Battery: %.2f V (%d%%)\n", vbatVolts, vbatPercent);
    return;
  }

  Serial.println("Unknown command. Type 'help'.");
}

static void cliPoll() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (cliBuf.length()) { cliHandleLine(cliBuf); cliBuf = ""; }
    } else if (isPrintable(c)) {
      cliBuf += c;
      if (cliBuf.length() > 120) cliBuf.remove(0, cliBuf.length() - 120);
    }
  }
}

// draw 1-bit bitmap helper
inline void drawIcon(int x, int y, const uint8_t* bmp) {
  display.drawBitmap(x, y, bmp, 12, 12, SH110X_WHITE);
}

// Top status bar (12 px tall): BLE + notif badge + optional play/pause hint
void drawStatusBar(bool playingHint=false) {
  display.fillRect(0, 0, SCREEN_W, 12, SH110X_BLACK);
  drawIcon(0, 0, ICON_BLE_12);
  if (!bleEnabled || !chrono.isConnected()) {
    display.drawLine(0, 11, 11, 0, SH110X_WHITE);
  }
  if (playingHint) {
    drawIcon(16, 0, ICON_PLAY_12);
  }
  drawIcon(SCREEN_W - 12, 0, ICON_BELL_12);
  if (hasNotif()) {
    display.fillCircle(SCREEN_W - 3, 2, 2, SH110X_WHITE);
  }
}

// Bottom dock (16 px tall): Eyes | Clock | Notif, highlight current
void drawModeDock(FaceMode m) {
  const int y = SCREEN_H - 16;
  display.fillRect(0, y, SCREEN_W, 16, SH110X_BLACK);
  int x1 = 20, x2 = SCREEN_W/2 - 6, x3 = SCREEN_W - 32;
  drawIcon(x1, y+2, ICON_EYE_12);
  drawIcon(x2, y+2, ICON_CLOCK_12);
  drawIcon(x3, y+2, ICON_BELL_12);
  int uW = 18;
  int ux = (m==FACE_EYES) ? (x1-3) : (m==FACE_CLOCK ? (x2-3) : (x3-3));
  display.fillRect(ux, y+14, uW, 2, SH110X_WHITE);
}

// =============================================================================
// ARDUINO MAIN FUNCTIONS
// =============================================================================

void setup() {
  Serial.begin(115200);
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);
  pinMode(FUNC_BTN,  INPUT_PULLUP);
  pinMode(VBAT_PIN,  INPUT);

  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  // Configure ADC width and attenuation for VBAT read on GPIO 7
  analogReadResolution(12); // 12-bit ADC
  analogSetPinAttenuation(VBAT_PIN, ADC_11db); // up to ~3.6V on pin (before divider)
  display.begin(SCREEN_ADDR, true);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0); display.println("PetBot booting...");
  display.display();

  // Custom UTF-8 font is header-only; no additional init required
  // I2S Audio init - now handled by Sectors::Voice::begin() in Sectors::beginAll()

  // Initialize microphone for noise/voice detection
  if (ENABLE_MIC_LISTENING) {
    if (Sectors::Voice::beginMicrophone(MIC_SAMPLE_RATE)) {
      Serial.println("Microphone initialized for noise detection");
    } else {
      Serial.println("Failed to initialize microphone");
    }
  }

  setupRoboEyes();
  // init WS2812 status LED
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.show();

  chrono.setNotificationCallback([](Notification n){
    // Copy strings to fixed char arrays with bounds checking
    n.title.toCharArray(lastNotifTitle, sizeof(lastNotifTitle));
    lastNotifTitle[sizeof(lastNotifTitle)-1] = '\0';
    n.message.toCharArray(lastNotifBody, sizeof(lastNotifBody));
    lastNotifBody[sizeof(lastNotifBody)-1] = '\0';

    // Check for AI messages (special prefixes)
    bool isAIMessage = false;
    char aiMessage[256] = {0};

    // AI message patterns
    if (strstr(lastNotifBody, "AI:") == lastNotifBody ||
        strstr(lastNotifBody, "@doge") == lastNotifBody ||
        strstr(lastNotifBody, "DogePet:") == lastNotifBody) {

      // Extract message after prefix
      const char* msgStart = strstr(lastNotifBody, ":");
      if (msgStart) {
        msgStart++; // Skip the colon
        // Skip whitespace
        while (*msgStart == ' ' || *msgStart == '\t') msgStart++;

        if (strlen(msgStart) > 0) {
          isAIMessage = true;
          strncpy(aiMessage, msgStart, sizeof(aiMessage) - 1);
          Serial.printf("AI Message detected: %s\n", aiMessage);
        }
      }
    }

    // Handle AI messages
    if (isAIMessage) {
      Sectors::Brain::sendUserMessage(aiMessage);
      return; // Don't process as regular notification
    }

    // Regular notification processing
    // Reset notification scroll position for new notification
    notifScrollX = 0;
    notifScrollT0 = 0;

    // Sequence-based cute notify; debounced
    static uint32_t lastNotifSfxMs = 0; const uint32_t NOTIF_SFX_GAP_MS = 500;
    uint32_t nowMs = millis();
    if (nowMs - lastNotifSfxMs >= NOTIF_SFX_GAP_MS) {
      if (ENABLE_NOTIFY_SFX) {
        Sectors::Voice::sfxNotify();
        Sectors::Voice::playCuteYes(120);
      }
      lastNotifSfxMs = nowMs;
    }
    triggerNotificationAttention();
    // Popup overlay for a few seconds
    notifPopupUntil = millis() + NOTIF_POPUP_MS;
  });
  chrono.setConnectionCallback([](bool c){
    Serial.printf("Chronos %s\n", c?"connected":"disconnected");
    if (ENABLE_BLE_SFX) {
      if (c) { Sectors::Voice::sfxDroidYes(); }
      else   { Sectors::Voice::sfxDroidNo(); }
    }
  });
  // Force BLE enabled on startup
  bleEnabled = true;
  chrono.begin();
  // set status LED blue to indicate BLE enabled
  if (ENABLE_LED_STATUS) {
    strip.setPixelColor(0, strip.Color(0,0,255));
    strip.show();
  }

  mode = FACE_EYES;

  // MPU6050 init is now handled by Sectors::MotionSector
  Sectors::Animations::setMood(MS_DEFAULT);  // default mood + LED color

  // seed RNG for jiggle timing (ESP32); fallback if not available would be analog noise
  randomSeed(esp_random());
  // Jiggle scheduling is now handled by Sectors::MotionSector

  // PSRAM check (for future audio/buffers)
  size_t ps = ESP.getPsramSize();
  if (ps > 0) {
    Serial.printf("PSRAM available: %u KB free\n", ESP.getFreePsram()/1024);
  } else {
    Serial.println("PSRAM not detected");
  }
  volume = AUDIO_DEFAULT_VOLUME;
  Sectors::Voice::setMasterVolume(volume);
  // Initialize WiFi for AI features (if enabled by default)
  if (ENABLE_WIFI && ENABLE_GEMINI_AI) {
    wifiEnabled = true; // Start with WiFi enabled
    Serial.println("Connecting to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Wait for connection with timeout
    unsigned long wifiStartTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - wifiStartTime) < WIFI_CONNECT_TIMEOUT_MS) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nWiFi connected!");
      Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
      showToast("WiFi Connected! 📶", 2000);
    } else {
      Serial.println("\nWiFi connection failed!");
      showToast("WiFi Failed 😢", 3000);
      wifiEnabled = false; // Reset flag on failure
    }
  }

  // Initialize Sectors (new modular structure)
  const char* apiKey = nullptr;
  if (ENABLE_GEMINI_AI && GEMINI_API_KEY && strlen(GEMINI_API_KEY) > 0 && WiFi.status() == WL_CONNECTED) {
    apiKey = GEMINI_API_KEY;
  }
  
  Serial.printf("[SETUP DEBUG] Initializing Sectors with API key: %s\n", apiKey ? "provided" : "null");
  Sectors::beginAll(apiKey);
  
  if (apiKey) {
    Serial.println("Sectors initialized with AI enabled - Send messages with 'AI:', '@doge', or 'DogePet:' prefix");
    showToast("AI Ready! 🤖", 2000);
  } else if (ENABLE_GEMINI_AI) {
    Serial.println("[SETUP DEBUG] AI disabled - WiFi not connected or no API key");
    showToast("AI: No WiFi", 3000);
  } else {
    Serial.println("[SETUP DEBUG] Gemini AI disabled - check config.h settings");
  }

  // Defer startup SFX until loop() begins to ensure I2S is fully ready
}

void loop() {
  // Get current time once for the entire loop
  uint32_t currentMs = millis();

  // Update all sectors (new modular structure)
  Sectors::updateAll();

  // BLE
  if (bleEnabled) chrono.loop();

  // BLE toggle UI (non-blocking)
  updateBleToggleUI();

  // Power management (track activity and sleep states)
  updatePowerManagement();

  // Microphone noise detection
  checkMicrophoneNoise();

  // Background AI chatter (now handled by Sectors::updateAll())
  static uint32_t lastDebugMs = 0;
  if (currentMs - lastDebugMs > 30000) { // Debug every 30 seconds
    Serial.printf("[MAIN DEBUG] Loop running, WiFi enabled: %s, currentMs: %lu\n", 
                 wifiEnabled ? "true" : "false", currentMs);
    lastDebugMs = currentMs;
  }

  // Serial CLI diagnostics
  cliPoll();

  // Face switching (hold on TOUCH_PIN to avoid conflict with triple tap)
  {
    static bool touchHolding = false;
    static bool faceHoldConsumed = false;
    static uint32_t touchDownAt = 0;
    const uint16_t FACE_HOLD_MS = 750; // hold duration to change face
    bool touchNow = (digitalRead(TOUCH_PIN) == HIGH);
    if (touchNow && !touchHolding) { touchHolding = true; faceHoldConsumed = false; touchDownAt = millis(); }
    if (!touchNow && touchHolding) { touchHolding = false; }
    if (touchHolding && !faceHoldConsumed) {
      if (millis() - touchDownAt >= FACE_HOLD_MS) {
        mode = (FaceMode)((mode + 1) % FACE_COUNT);
        if (ENABLE_MODE_SFX) {
          if (mode == FACE_NOTIF) {
            Sectors::Voice::sfxNotify();
          } else if (mode == FACE_CLOCK) {
            Sectors::Voice::sfxConfirm();
            Sectors::Voice::playCuteQuestion();
          } else {
            Sectors::Voice::sfxCurious();
          }
        }
        wakeUp(); // Wake up on face switch
        faceHoldConsumed = true;
      }
    }
  }

  // Triple-tap TOUCH_PIN toggles silent/chatty (allowed on all faces)
  if (tripleTap(TOUCH_PIN, /*activeHigh=*/true, /*debounceMs=*/10, /*windowMs=*/650)) {
    wakeUp(); // Wake up on triple-tap
    silentMode = !silentMode;
    if (silentMode) {
      savedVolume = volume;
      volume = 0;
      Sectors::Voice::setMasterVolume(0);
      if (ENABLE_MODE_SFX) Sectors::Voice::sfxError();
      showToast("Silent mode 🤫", 1200);Serial.println("Silent mode");
    } else {
      volume = savedVolume;
      Sectors::Voice::setMasterVolume(volume);
      if (ENABLE_MODE_SFX) Sectors::Voice::sfxConfirm();
      showToast("Chatty mode 🎵", 1200);Serial.println("Chatty mode");
    }
  }

  // Ensure notification attention animation progresses regardless of face
  updateNotificationAnim();

  // Draw active face with throttled updates
  switch (mode) {
    case FACE_EYES:
      // Throttle RoboEyes updates for better performance
      if (currentMs - lastRoboEyesUpdateMs >= ROBOEYES_UPDATE_MS) {
        // Keep eyes fully animated every update; toast will overlay after
        static bool wasBlinking = false;
        static uint32_t lastAutoBlinkSfxMs = 0;

        if (!isLazy) {
          // Note: Direct access to eyeL_open/eyeR_open not available through wrapper
          // TODO: Add isBlinking() method to Display sector if needed
          Sectors::Display::update();
          
          // Simplified blink SFX trigger - could be improved with sector feedback
          if (ENABLE_TAP_SFX && !silentMode && (currentMs - lastAutoBlinkSfxMs > 800)) {
            // Trigger blink sound occasionally during normal animation
            if ((currentMs / 1000) % 5 == 0) { // Every ~5 seconds roughly
              Sectors::Voice::sfxBlink();
              lastAutoBlinkSfxMs = currentMs;
            }
          }
        } else {
          // Lazy mode subtle bobbing: small vertical motion at low rate
          static int dir = 1; static int bob = 0;
          if ((currentMs / 400) % 2 == 0) {
            bob += dir;
            if (bob > 2) { bob = 2; dir = -1; }
            if (bob < -2) { bob = -2; dir = 1; }
            Sectors::Display::setPosition((bob >= 0) ? Sectors::Display::POS_N : Sectors::Display::POS_S);
          }
          // Occasionally play tiny sleep jingle
          if (!silentMode && currentMs >= nextLazyJingleMs) {
            Sectors::Voice::playCuteSleep(0);
            nextLazyJingleMs = currentMs + (uint32_t)(LAZY_JINGLE_MIN_MS + (esp_random() % (LAZY_JINGLE_MAX_MS - LAZY_JINGLE_MIN_MS + 1)));
          }
        }
        lastRoboEyesUpdateMs = currentMs;
      }
      // Suppress random idle SFX during jiggling or non-default moods to keep it calm
      if (!isLazy && ENABLE_IDLE_CHATTER && !silentMode && !jiggling && Sectors::Animations::getCurrentMood() == MS_DEFAULT) {
        // Talkativeness mapping: threshold and cooldown by level
        static uint32_t lastChatterMs = 0;
        uint8_t lvl = talkativeLevel; if (lvl > 5) lvl = 5;
        // levels 0..5
        const uint32_t maskByLvl[6] = {
          0x7FFFFFFF, // 0 never
          0x7FFF,     // 1 ~1/32768
          0x3FFF,     // 2 ~1/16384
          0x1FFF,     // 3 ~1/8192
          0x0FFF,     // 4 ~1/4096
          0x07FF      // 5 ~1/2048
        };
        const uint32_t cooldownByLvl[6] = {
          0xFFFFFFFF, // 0 never
          180000,     // 1 3m
          90000,      // 2 1.5m
          45000,      // 3 45s
          25000,      // 4 25s
          15000       // 5 15s
        };
        uint32_t now = millis();
        if (((esp_random() & maskByLvl[lvl]) == 0) &&
            !Sectors::Voice::isSequencePlaying() &&
            (now - lastChatterMs > cooldownByLvl[lvl])) {
          // Note: AI chatter is now handled by Sectors::Brain in updateAll()
          // Only play local audio/binary when AI is disabled
          if (ENABLE_IDLE_AUDIO_CHATTER && !ENABLE_GEMINI_AI) {
            Sectors::Voice::playCuteChatterFree(450, 1300, 0);
          }
          // Show binary communication when chattering (only if AI is disabled)
          if (ENABLE_BINARY_CHATTER && !ENABLE_GEMINI_AI) {
            showBinaryChatter();
          }
          lastChatterMs = now;
        }
      }
      // Keep toast as the very last draw when visible
      if (toastVisible) {
        // Disable eyes auto-flush and reserve a HUD band for toast
        gEyesAutoFlush = false;
        const int DOCK_H = 16;
        const int TOAST_H = 24;  // Increased from 18 to 24 for 2 rows
        const int TOAST_Y = SCREEN_H - DOCK_H - TOAST_H;
        gEyesViewportYMax = TOAST_Y; // eyes draw only above toast band

        // Draw toast overlay every loop so it wins over any eyes repaint
        drawToastIfAny(display);
        display.display();
        displayNeedsUpdate = false;
        lastDisplayUpdateMs = currentMs;
      } else if (currentMs - lastDisplayUpdateMs >= DISPLAY_UPDATE_MS) {
        // Normal throttled frame when no toast
        drawToastIfAny(display); // no-op when no toast
        if (displayNeedsUpdate) {
          display.display();
          displayNeedsUpdate = false;
        }
        lastDisplayUpdateMs = currentMs;
      }
      break;

    case FACE_CLOCK:
      // Throttle clock display updates
      if (currentMs - lastDisplayUpdateMs >= DISPLAY_UPDATE_MS) {
        drawClock(display, chrono);
        lastDisplayUpdateMs = currentMs;
      }
      break;
    case FACE_NOTIF:
      // Throttle notification display updates
      if (currentMs - lastDisplayUpdateMs >= DISPLAY_UPDATE_MS) {
        drawNotif(display);
        lastDisplayUpdateMs = currentMs;
      }
      break;
  }

  // Final overlay and single flush when a toast is visible (wins over any prior repaint)
  if (toastVisible) {
    // Ensure overlay wins and only we flush
    drawToastIfAny(display);
    display.display();
    displayNeedsUpdate = false;
    lastDisplayUpdateMs = currentMs;
  } else {
    // Restore normal eyes flushing/viewport when no toast
    gEyesAutoFlush = true;
    gEyesViewportYMax = 0;
  }

  // small pacing to avoid excessive I2C spam on non-eyes faces
  if (mode != FACE_EYES) delay(NON_EYES_DELAY_MS);

  // Audio update
  Sectors::Voice::update();
  Sectors::Voice::sequencerUpdate();

  // One-shot startup sound once audio/I2S is live
  {
    static bool playedStartup = false;
    static uint32_t bootT0 = millis();
    if (!playedStartup && millis() - bootT0 > STARTUP_SFX_DELAY) { // slight delay to be safe
      if (ENABLE_STARTUP_SFX) Sectors::Voice::playCuteStartup();
      playedStartup = true;
    }
  }

  // Auto popup handling: show popup for a few seconds, then return to prior face
  static FaceMode prevMode = FACE_EYES;
  if (notifPopupUntil) {
    if (mode != FACE_NOTIF) { prevMode = mode; mode = FACE_NOTIF; }
    if (millis() > notifPopupUntil) { notifPopupUntil = 0; mode = prevMode; }
  }
  
  // Emotion engine (run ~every frame; it’s cheap)
  // Motion/IMU updates are now handled by Sectors::MotionSector

  // Only read when display is also being updated to reduce I2C traffic
  static uint32_t lastVbatLogMs = 0;
  if (currentMs - lastVbatLogMs > VBAT_LOG_MS) {
    vbatVolts = readVBATVolts();
    vbatPercent = voltsToPercent(vbatVolts);
    lastVbatReadMs = currentMs;
    lastVbatLogMs = lastVbatReadMs;
    //Serial.printf("VBAT: %.2f V (%d%%)%s\n", vbatVolts, vbatPercent, batteryCharging?" [charging]":"");
  }

}
