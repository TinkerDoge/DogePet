// PetBot v0.1 — Visual Core (non-blocking BLE toggle, debounced touch)
// Faces: RoboEyes (default), Clock, Notifications
// Author: [Doge De Shibe]
// Description: ESP32-S3 Super Mini companion bot with animated eyes, motion-reactive emotions

// =============================================================================
// INCLUDES & DEPENDENCIES
// =============================================================================
#include <Wire.h>
#include <esp_system.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_NeoPixel.h>
#include <ChronosESP32.h>


// Custom headers
#include "icons.h"
#include "mpu6050.h"

#include "util.h"
#include "motion.h"

// =============================================================================
// GLOBAL VARIABLES & CONSTANTS
// =============================================================================
// Forward-declare enums to avoid Arduino prototype generation issues
enum FaceMode : uint8_t;
enum MoodState : uint8_t;
// === Toast overlay (non-blocking notifications) ===
String  toastText;
uint32_t toastUntil = 0;

inline void showToast(const String& s, uint16_t ms=1200) {
  toastText = s; toastUntil = millis() + ms;
}

// =============================================================================
// HARDWARE CONFIGURATION
// =============================================================================

// Pin definitions
#define I2C_SDA         9
#define I2C_SCL         8
#define TOUCH_PIN       13   // HIGH when touched (TPS223)
#define FUNC_BTN        1    // Hold to toggle BLE
#define I2S_LRC         10   // WS/LRCLK
#define I2S_BCLK        11   // BCLK
#define I2S_DO          12   // SD out -> MAX98357A DIN
#define I2S_DI          2    // (unused)
#define LED_PIN         48   // Status LED (WS2812 data pin)
#define LED_BRIGHTNESS  60
#define VBAT_PIN        7    // Battery voltage divider
#define VBAT_PIN        7    // Battery voltage divider


// Hardware object instances
Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

// OLED Display configuration
#define SCREEN_W        128
#define SCREEN_H        64
#define OLED_RESET      -1
#define SCREEN_ADDR     0x3C
Adafruit_SH1106G display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET); // Must exist BEFORE RoboEyes include

// =============================================================================
// GLOBAL CONSTANTS & CONFIGURATION
// =============================================================================

// === Emotion & Motion Detection Thresholds ===
const float  TILT_HAPPY_DEG    = 20.0f;    // gentle tilt ⇒ happy
const float  SHAKE_ANGRY_DPS   = 140.0f;   // quick gyro spike ⇒ angry
const float  SHAKE_FURIOUS_DPS = 200.0f;   // higher gyro spike ⇒ furious
const uint16_t SHAKE_MS        = 120;      // sustained spike window
const uint16_t FURIOUS_MS      = 200;      // longer sustained spike for furious
const float  STILL_G_THRESH    = 0.06f;    // |ax|,|ay| below this and az≈+1g → flat
const float  AZ_1G_TOL         = 0.12f;    // az within 1g±tol

// === Timing Constants ===
const uint32_t MOOD_HOLD_MS    = 2500;     // how long to keep happy/angry
const uint32_t FURIOUS_HOLD_MS = 4000;     // longer duration for furious
// const uint32_t SLEEP_AFTER_MS  = 7000;     // removed: do not auto-sleep when flat

// === Tap Detection Constants ===
const float TAP_SPIKE_DPS      = 140.0f;  // quick rotation spike => blink
const uint32_t TAP_MIN_GAP_MS  = 600;     // min time between blinks
const uint32_t TAP_COOLDOWN_MS = 200;     // cooldown after tap detection to prevent continuous blinking

// === Jiggle System Constants ===
const uint32_t DIR_NUDGE_COOLDOWN_MS = 700;   // throttle direction nudges
const float     DIR_DEADZONE_DEG      = 12.0f; // ignore tiny tilts (let idle roam)
const float TILT_DEG_POSITIONS = 15.0f;   // sector change threshold

// === Debug System Constants ===
const float ACCEL_DELTA = 0.05f;   // g-units
const float GYRO_DELTA  = 5.0f;    // deg/s

// === Battery Sensing ===
static constexpr float VBAT_MIN_V = 3.30f;  // 0%
static constexpr float VBAT_MAX_V = 4.20f;  // 100%
static constexpr uint8_t VBAT_SAMPLES = 12; // simple averaging

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

// === Emotion & Motion State ===
uint32_t lastMoveMs = 0;        // when significant motion last happened
uint32_t moodUntil  = 0;        // temporary mood timeout
uint32_t shakeStart = 0;        // for sustained shake
uint32_t furiousStart = 0;      // for furious shake detection
bool     shaking    = false;
bool     furiousShaking = false;

// === Timing Variables ===
static uint32_t lastBlinkMs = 0;
static uint32_t lastTapCheckMs = 0;       // tracks last tap detection check

// === Furious Jiggle System ===
uint32_t furiousJiggleEndMs = 0;
bool     furiousJiggling     = false;

// === Regular Jiggle System ===
uint32_t jiggleNextMs = 0;
uint32_t jiggleEndMs  = 0;
bool     jiggling     = false;

// === Low Pass Filtering ===
float lpf_ax = 0, lpf_ay = 0, lpf_az = 0, lpf_g = 0;

// === Battery sense cache ===
static uint32_t lastVbatReadMs = 0;
static float    vbatVolts = 0.0f;
static int      vbatPercent = -1; // -1 when unknown

// === Debug System Variables ===
static float last_ax = 0, last_ay = 0, last_az = 0;
static float last_gx = 0, last_gy = 0, last_gz = 0;


// =============================================================================
// PSRAM-BACKED DOUBLE BUFFER (optional non-blocking flush)
// =============================================================================


// (double-buffering removed)

// =============================================================================
// ROBOEYES ANIMATION SYSTEM
// =============================================================================

// Important: include RoboEyes AFTER the global 'display' is instantiated
#include <FluxGarage_RoboEyes.h>

roboEyes Eyes;

// Thin wrappers so motion.cpp can control RoboEyes without including its header
void Eyes_SetMood(uint8_t mood) { Eyes.setMood(mood); }
void Eyes_SetHFlicker(bool on, int level) { Eyes.setHFlicker(on, level); }
void Eyes_SetVFlicker(bool on, int level) { Eyes.setVFlicker(on, level); }
void Eyes_Blink() { Eyes.blink(); }

// REMOVED: Furious jiggle variables moved to GLOBAL VARIABLES section above

void setupRoboEyes() {
  Eyes.begin(SCREEN_W, SCREEN_H, 60);
  Eyes.setWidth(40, 40);
  Eyes.setHeight(26, 26);
  Eyes.setBorderradius(5, 5);
  Eyes.setSpacebetween(10);
  // Give the internal idle engine room to roam
  Eyes.setAutoblinker(true, 3, 4);  // avg ~3s with ±4s jitter
  Eyes.setIdleMode(true, 4, 5);     // moderate wander frequency & randomness
  Eyes.setMood(DEFAULT);
  Eyes.setCuriosity(true);
}

// =============================================================================
// MOTION PROCESSING & EMOTION SYSTEM
// =============================================================================

// IMU calibration offsets (extern declarations from mpu6050.h)
int32_t mpu_accel_off_x=0, mpu_accel_off_y=0, mpu_accel_off_z=0;
int32_t mpu_gyro_off_x=0, mpu_gyro_off_y=0, mpu_gyro_off_z=0;

// Emotion system (type declared in motion.h)
extern MoodState curMood;

// Note: keep the status LED behavior separate; do not change it when mood changes.
// Implemented in motion.cpp
// Furious jiggle functionality is implemented in motion.cpp
void updateEmotionsFromIMU();

// ===== Liveliness updater (~25Hz) =====

// ---------- MPU Debug Printing ----------
void debugPrintIMUIfChanged();

// ===== Liveliness / IMU reaction =====
static uint32_t imuTickMs = 0;
// (lpf_ax, lpf_ay, lpf_az and lpf_g exist in the emotion engine; reuse them)
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
  notif_prevMood = curMood;
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
        setEyesMood(MS_HAPPY); moodUntil = now + 800;
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
        setEyesMood(notif_prevMood);
        notifAnimState = 0;
      }
      break;
  }
}

// small micro-expression helpers
inline void doWinkLeft(){ Eyes.blink(true, false); }
inline void doWinkRight(){ Eyes.blink(false, true); }
inline void doSmile(uint16_t ms=700){ setEyesMood(MS_HAPPY); moodUntil = millis() + ms; } // Single call - acceptable

// Implemented in motion.cpp
inline void updateLivelinessFromIMU() {
  // Cache current time to avoid multiple millis() calls
  uint32_t currentMs = millis();

  float ax,ay,az,gx,gy,gz;
  mpuRead(ax,ay,az,gx,gy,gz);
  lpf_ax = lpf(lpf_ax, ax);
  lpf_ay = lpf(lpf_ay, ay);
  lpf_az = lpf(lpf_az, az);
  float gmag = sqrtf(gx*gx + gy*gy + gz*gz);
  lpf_g = lpf(lpf_g, gmag, 0.3f);

  // 1) Blink on tap (but not when angry or furious) - with cooldown to prevent continuous blinking
  if (lpf_g > TAP_SPIKE_DPS && (currentMs - lastBlinkMs > TAP_MIN_GAP_MS) &&
      (currentMs - lastTapCheckMs > TAP_COOLDOWN_MS) &&
      curMood != MS_ANGRY && curMood != MS_FURIOUS) {
    Eyes.blink();
    lastBlinkMs = currentMs;
    lastTapCheckMs = currentMs;  // Add cooldown after successful tap detection
  }
  // 2) Regular micro-jiggle (only when not furious - furious has its own jiggle)
  if (!jiggling && currentMs >= jiggleNextMs && curMood != MS_FURIOUS) {
    jiggling = true;
    jiggleEndMs = currentMs + 120 + (uint32_t)random(0, 120);
    Eyes.setHFlicker(true, 1);
    if (random(0,3)==0) Eyes.setVFlicker(true, 1);
  }
  if (jiggling && currentMs >= jiggleEndMs) {
    jiggling = false;
    Eyes.setHFlicker(false, 0);
    Eyes.setVFlicker(false, 0);
    scheduleNextJiggle();
  }

  // 3) Furious jiggle update (separate system)
  if (furiousJiggling && currentMs >= furiousJiggleEndMs) {
    furiousJiggling = false;
    Eyes.setHFlicker(false, 0);
    Eyes.setVFlicker(false, 0);
  }

}

// schedule a tiny flicker burst every so often
inline void scheduleNextJiggle() {
  uint32_t currentMs = millis();  // Cache current time
  jiggleNextMs = currentMs + 800 + (uint32_t)random(0, 2200);  // 0.8–3.0s
}


// =============================================================================
// FACE MODES & BLE SYSTEM
// =============================================================================

// Available face modes
enum FaceMode : uint8_t { FACE_EYES=0, FACE_CLOCK=1, FACE_NOTIF=2, FACE_COUNT };
FaceMode mode = FACE_EYES;

// BLE / Chronos time synchronization
ChronosESP32 chrono("PetBot");
bool bleEnabled = true;

// notif cache
String lastNotifTitle, lastNotifBody;

// Utility functions
static String zeroPad2(int v){ char b[3]; snprintf(b, sizeof(b), "%02d", v); return String(b); }

// Battery helpers
static float readVBATVolts() {
  // Average a few samples
  uint32_t acc = 0;
  for (uint8_t i=0;i<VBAT_SAMPLES;i++) acc += analogRead(VBAT_PIN);
  float avg = (float)acc / (float)VBAT_SAMPLES;
  // 12-bit raw -> pin voltage (assuming 11dB attn factory calibration)
  // On ESP32-S3 Arduino core, analogRead returns 0..4095 for 0..~3.6V with 11dB.
  float v_pin = (avg / 4095.0f) * 3.60f; // approximate; good enough for percentage
  // Divider is 1:2 → battery voltage is 2x pin voltage
  return v_pin * 2.0f;
}

static int voltsToPercent(float v) {
  if (v <= VBAT_MIN_V) return 0;
  if (v >= VBAT_MAX_V) return 100;
  float pct = (v - VBAT_MIN_V) / (VBAT_MAX_V - VBAT_MIN_V) * 100.0f;
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  return (int)(pct + 0.5f);
}

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

// FUNC button hold → BLE toggle (non-blocking)
enum BleUiState { BLEUI_IDLE, BLEUI_PROMPT, BLEUI_ENABLING, BLEUI_DISABLING, BLEUI_DONE };
BleUiState bleUi = BLEUI_IDLE;
uint32_t   bleUiT0 = 0;
uint32_t   btnDownT0 = 0;
const uint32_t HOLD_TIME_MS = 2000;

void updateBleToggleUI() {
  bool btn = (digitalRead(FUNC_BTN)==LOW);

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
        } else {
          bleEnabled = false;
          chrono.stop(true);
          Serial.println("BLE: DISABLING");
          strip.setPixelColor(0, 0);
          strip.show();
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
          bleEnabled = true; bleUi = BLEUI_ENABLING;  bleUiT0 = millis(); chrono.begin(); showToast("BLE: ENABLING");
          strip.setPixelColor(0, strip.Color(0,0,255)); strip.show();
        }
        else {
          bleEnabled = false; bleUi = BLEUI_DISABLING; bleUiT0 = millis(); chrono.stop(true); showToast("BLE: DISABLING");
          strip.setPixelColor(0, 0); strip.show();
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
      showToast(bleEnabled ? "BLE: ENABLED" : "BLE: DISABLED");
      display.display();
      if (millis()-bleUiT0 > 900) bleUi = BLEUI_IDLE;
      break;
  }
}

// =============================================================================
// DISPLAY RENDERING FUNCTIONS
// =============================================================================

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
// returns whether we have any cached notification text
inline bool hasNotif() {
  return lastNotifTitle.length() || lastNotifBody.length();
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
  drawIcon(x1, y+2, ICON_EYES_12);
  drawIcon(x2, y+2, ICON_CLOCK_12);
  drawIcon(x3, y+2, ICON_BELL_12);
  int uW = 18;
  int ux = (m==FACE_EYES) ? (x1-3) : (m==FACE_CLOCK ? (x2-3) : (x3-3));
  display.fillRect(ux, y+14, uW, 2, SH110X_WHITE);
}

// Toast overlay (centered)
void drawToastIfAny() {
  if (millis() > toastUntil || toastText.length()==0) return;
  int16_t x1,y1; uint16_t w,h;
  display.setTextSize(1);
  display.getTextBounds(toastText, 0,0, &x1,&y1, &w,&h);
  int bx = (SCREEN_W - (int)w - 8)/2;
  int by = (SCREEN_H - 12)/2;
  display.fillRect(bx, by, w+8, 12, SH110X_BLACK);
  display.drawRect(bx, by, w+8, 12, SH110X_WHITE);
  display.setCursor(bx+4, by+2);
  display.print(toastText);
}

// Face: Clock
void drawClock() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  // Build strings (with seconds)
  String timeStr, dateStr, batStr;
  if (chrono.isConnected()) {
    timeStr = chrono.getHourZ() + ":" + zeroPad2(chrono.getMinute()) + ":" + zeroPad2(chrono.getSecond());
    dateStr = zeroPad2(chrono.getDay()) + "/" + zeroPad2(chrono.getMonth());
    // local device battery
    if (millis() - lastVbatReadMs > 3000 || vbatPercent < 0) {
      vbatVolts = readVBATVolts();
      vbatPercent = voltsToPercent(vbatVolts);
      lastVbatReadMs = millis();
    }
    batStr  = String(vbatPercent) + "%";
  } else {
    time_t t = time(NULL); struct tm tmnow; localtime_r(&t, &tmnow);
    char tbuf[9]; snprintf(tbuf,sizeof(tbuf),"%02d:%02d:%02d", tmnow.tm_hour, tmnow.tm_min, tmnow.tm_sec);
    char dbuf[6]; snprintf(dbuf,sizeof(dbuf),"%02d/%02d", tmnow.tm_mday, tmnow.tm_mon+1);
    timeStr = tbuf; dateStr = dbuf;
    if (millis() - lastVbatReadMs > 3000 || vbatPercent < 0) {
      vbatVolts = readVBATVolts();
      vbatPercent = voltsToPercent(vbatVolts);
      lastVbatReadMs = millis();
    }
    batStr = String(vbatPercent >= 0 ? vbatPercent : 0) + "%";
  }

  // Frame
  const int fx = 6, fy = 8;
  const int fw = SCREEN_W - 12, fh = 48;
  display.drawRoundRect(fx, fy, fw, fh, 6, SH110X_WHITE);

  // Header row inside frame
  display.setTextSize(1);
  display.setCursor(fx + 4, fy + 2); display.print(dateStr);
  int16_t bx1, by1; uint16_t bw, bh;
  display.getTextBounds(batStr, 0,0, &bx1,&by1,&bw,&bh);
  display.setCursor(fx + fw - bw - 4, fy + 2); display.print(batStr);

  // Big HH:MM:SS at bottom row inside frame; scale to fit
  int textSize = 3;
  int16_t tx1, ty1; uint16_t tw, th;
  for (; textSize >= 1; --textSize) {
    display.setTextSize(textSize);
    display.getTextBounds(timeStr, 0,0, &tx1,&ty1,&tw,&th);
    if (tw <= (uint16_t)(fw - 8) && th <= (uint16_t)(fh - 14)) break; // leave margins
  }
  int tx = fx + (fw - tw)/2;
  int ty = fy + fh - th - 3; // bottom row with small padding
  display.setCursor(tx, ty); display.print(timeStr);

  // no HUD
  drawToastIfAny();
  display.display();
}

// Face: Notification popup (framed like clock)
static uint32_t notifPopupUntil = 0;
void drawNotif() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE); display.setTextSize(1);

  // Frame same as clock
  const int fx = 6, fy = 8;
  const int fw = SCREEN_W - 12, fh = 48;
  display.drawRoundRect(fx, fy, fw, fh, 6, SH110X_WHITE);

  if (lastNotifTitle.length()==0 && lastNotifBody.length()==0) {
    // placeholder
    display.setCursor(fx+4, fy+2); display.print("No notifications");
  } else {
    // Title at top
    display.setCursor(fx+4, fy+2);
    display.println(lastNotifTitle.substring(0, (fw/6)));
    // Body in remaining area (simple wrap)
    String body = lastNotifBody;
    body.replace('\n',' ');
    display.setCursor(fx+4, fy+14);
    display.println(body.substring(0, (fw/6)*2));
  }

  drawToastIfAny();
  display.display();
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

  setupRoboEyes();
  // init WS2812 status LED
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.show();

  chrono.setNotificationCallback([](Notification n){
    lastNotifTitle = n.title;
    lastNotifBody = n.message;
    triggerNotificationAttention();
    // Popup overlay for 4 seconds
    extern uint32_t notifPopupUntil; // forward
    notifPopupUntil = millis() + 4000;
  });
  chrono.setConnectionCallback([](bool c){ Serial.printf("Chronos %s\n", c?"connected":"disconnected"); });
  // Force BLE enabled on startup
  bleEnabled = true;
  chrono.begin();
  // set status LED blue to indicate BLE enabled
  strip.setPixelColor(0, strip.Color(0,0,255));
  strip.show();

  mode = FACE_EYES;

  // MPU6050 init & quick calibration (call while stationary)
  mpuInit();
  delay(100);
  mpuCalibrate(150);
  setEyesMood(MS_DEFAULT);  // default mood + LED color

  // seed RNG for jiggle timing (ESP32); fallback if not available would be analog noise
  randomSeed(esp_random());
  scheduleNextJiggle();

  // PSRAM check (for future audio/buffers)
  size_t ps = ESP.getPsramSize();
  if (ps > 0) {
    Serial.printf("PSRAM available: %u KB free\n", ESP.getFreePsram()/1024);
  } else {
    Serial.println("PSRAM not detected");
  }

}

void loop() {
  // BLE
  if (bleEnabled) chrono.loop();

  // BLE toggle UI (non-blocking)
  updateBleToggleUI();

  // Face switching
  if (edgeRising(TOUCH_PIN, /*activeHigh=*/true, /*debounceMs=*/30)) {
    mode = (FaceMode)((mode + 1) % FACE_COUNT);
  }

  // Ensure notification attention animation progresses regardless of face
  updateNotificationAnim();

  // Draw active face
  switch (mode) {
    case FACE_EYES:
      // Keep the face clean: RoboEyes draws into the buffer. Do not draw HUD
      // or dock here so the face remains uncluttered and stable.
      Eyes.update();
      // Overlay toast if any, then flush
      drawToastIfAny();
      display.display();
      break;

    case FACE_CLOCK:  drawClock();   break;
    case FACE_NOTIF:  drawNotif();   break;
  }

  // small pacing to avoid excessive I2C spam on non-eyes faces
  if (mode != FACE_EYES) delay(120);

  // Auto popup handling: show popup for a few seconds, then return to prior face
  static FaceMode prevMode = FACE_EYES;
  if (notifPopupUntil) {
    if (mode != FACE_NOTIF) { prevMode = mode; mode = FACE_NOTIF; }
    if (millis() > notifPopupUntil) { notifPopupUntil = 0; mode = prevMode; }
  }

  // (old MPU debug prints removed)
  
  // Emotion engine (run ~every frame; it’s cheap)
  static uint32_t emoT0 = 0;
  if (millis() - emoT0 > 40) {  // ~25 Hz
    emoT0 = millis();
    updateEmotionsFromIMU();
  }

  // Liveliness: ~25 Hz
  if (millis() - imuTickMs > 40) {
    imuTickMs = millis();
    updateLivelinessFromIMU();
    // Debug print MPU only if significant change
    debugPrintIMUIfChanged();

  }

}
