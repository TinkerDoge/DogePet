// PetBot v0.1 — Visual Core (non-blocking BLE toggle, debounced touch)
// Faces: RoboEyes (default), Clock, Notifications

#include <Wire.h>
#include <esp_system.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#include <Adafruit_NeoPixel.h>
// Forward-declare FaceMode so Arduino's auto-generated prototypes can refer to it
enum FaceMode : uint8_t;
// Forward-declare MoodState to avoid Arduino prototype generation issues
enum MoodState : uint8_t;
// icons are moved to a separate header for easier updates
#include "icons.h"
#include <FluxGarage_RoboEyes.h>
#include "mpu6050.h"
#include <ChronosESP32.h>

// === toast overlay (non-blocking) ===
String  toastText;
uint32_t toastUntil = 0;

inline void showToast(const String& s, uint16_t ms=1200) {
  toastText = s; toastUntil = millis() + ms;
}

// ---------- Pins ----------
#define I2C_SDA   9
#define I2C_SCL   8
#define TOUCH_PIN 13   // HIGH when touched (TPS223)
#define FUNC_BTN  1    // hold to toggle BLE

#define I2S_LRC  10   // WS/LRCLK
#define I2S_BCLK 11   // BCLK
#define I2S_DO   12   // SD out -> MAX98357A DIN
#define I2S_DI    2   // (unused)

#define LED_PIN   48    // status LED (WS2812 data pin)
#define LED_BRIGHTNESS 60


Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
// ---------- OLED ----------
#define SCREEN_W 128
#define SCREEN_H 64
#define OLED_RESET -1
#define SCREEN_ADDR 0x3C
Adafruit_SH1106G display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET); // must exist BEFORE RoboEyes include

// ---------- RoboEyes ----------

roboEyes Eyes;
void setupRoboEyes() {
  Eyes.begin(SCREEN_W, SCREEN_H, 60);
  Eyes.setWidth(40, 40);
  Eyes.setHeight(26, 26);
  Eyes.setBorderradius(5, 5);
  Eyes.setSpacebetween(10);
  // CHANGE: give the internal idle engine room to roam
  Eyes.setAutoblinker(true, 3, 4);  // avg ~3s with ±4s jitter
  Eyes.setIdleMode(true, 4, 5);     // moderate wander frequency & randomness
  Eyes.setMood(DEFAULT);
  Eyes.setCuriosity(true);
}

// MPU moved to separate module


// define extern offsets declared in mpu6050.h
int32_t mpu_accel_off_x=0, mpu_accel_off_y=0, mpu_accel_off_z=0;
int32_t mpu_gyro_off_x=0, mpu_gyro_off_y=0, mpu_gyro_off_z=0;

// ===== Emotion engine (from IMU) =====
enum MoodState : uint8_t { MS_DEFAULT, MS_HAPPY, MS_ANGRY, MS_TIRED };
MoodState curMood = MS_DEFAULT;

// Note: keep the status LED behavior separate; do not change it when mood changes.
void setEyesMood(MoodState m) {
  curMood = m;
  switch (m) {
    case MS_DEFAULT: Eyes.setMood(DEFAULT); break;
    case MS_HAPPY:   Eyes.setMood(HAPPY);   break;
    case MS_ANGRY:   Eyes.setMood(ANGRY);   break;
    case MS_TIRED:   Eyes.setMood(TIRED);   break;
  }
}

// Thresholds (tune to taste)
const float  TILT_HAPPY_DEG    = 20.0f;    // gentle tilt ⇒ happy
const float  SHAKE_ANGRY_DPS   = 140.0f;   // quick gyro spike ⇒ angry
const uint16_t SHAKE_MS        = 120;      // sustained spike window
const float  STILL_G_THRESH    = 0.06f;    // |ax|,|ay| below this and az≈+1g → flat
const float  AZ_1G_TOL         = 0.12f;    // az within 1g±tol
// const uint32_t SLEEP_AFTER_MS  = 7000;     // removed: do not auto-sleep when flat
const uint32_t MOOD_HOLD_MS    = 2500;     // how long to keep happy/angry

// State for detection
uint32_t lastMoveMs = 0;        // when significant motion last happened
uint32_t moodUntil  = 0;        // temporary mood timeout
uint32_t shakeStart = 0;        // for sustained shake
bool     shaking    = false;

// tiny smoothing
float lpf_ax=0, lpf_ay=0, lpf_az=0, lpf_g=0;
inline float lpf(float prev, float x, float a=0.2f) { return prev*(1.0f-a) + x*a; }

// Compute tilt (approx) from accel
inline void accelToAngles(float ax, float ay, float az, float& pitchDeg, float& rollDeg) {
  // pitch around X, roll around Y
  pitchDeg = atan2f(-ax, sqrtf(ay*ay + az*az)) * 57.2958f;
  rollDeg  = atan2f( ay, sqrtf(ax*ax + az*az)) * 57.2958f;
}

void updateEmotionsFromIMU() {
  // read IMU (already calibrated in your setup)
  float ax, ay, az, gx, gy, gz;
  mpuRead(ax, ay, az, gx, gy, gz);

  // low-pass accel & gyro magnitude
  float gmag = sqrtf(gx*gx + gy*gy + gz*gz);
  lpf_ax = lpf(lpf_ax, ax);
  lpf_ay = lpf(lpf_ay, ay);
  lpf_az = lpf(lpf_az, az);
  lpf_g  = lpf(lpf_g , gmag, 0.3f);

  // detect motion vs stillness
  bool moving = (fabsf(ax) > STILL_G_THRESH) || (fabsf(ay) > STILL_G_THRESH) || (fabsf(az-1.0f) > AZ_1G_TOL) || (lpf_g > 40.0f);
  if (moving) lastMoveMs = millis();

  // quick SHAKE → ANGRY for a short period
  if (lpf_g > SHAKE_ANGRY_DPS) {
    if (!shaking) { shaking = true; shakeStart = millis(); }
    if (millis() - shakeStart > SHAKE_MS) {
      setEyesMood(MS_ANGRY);
      moodUntil = millis() + MOOD_HOLD_MS;
      showToast("grrr!");
    }
  } else {
    shaking = false;
  }

  // TAP/BUMP detection: quick accel spike -> blink both eyes once
  float tap_mag = fabsf(ax) + fabsf(ay) + fabsf(az - 1.0f);
  if (tap_mag > 0.5f) {
    // quick blink
    Eyes.blink();
  }

  // gentle TILT → HAPPY briefly (if not angry)
  float pitch, roll;
  accelToAngles(lpf_ax, lpf_ay, lpf_az, pitch, roll);
  if (curMood != MS_ANGRY) {
    if (fabsf(pitch) > TILT_HAPPY_DEG || fabsf(roll) > TILT_HAPPY_DEG) {
      setEyesMood(MS_HAPPY);
      moodUntil = millis() + MOOD_HOLD_MS;
    }
  }

  // NOTE: removed per-frame follow-gravity nudges so the library's idle/curiosity
  // engine can roam naturally. We will use rare, throttled nudges elsewhere.

  // removed auto-sleep behavior: keep the bot awake even when lying flat

  // Expire temporary moods (happy/angry) back to default unless we’re sleeping
  if (curMood != MS_TIRED && (curMood == MS_HAPPY || curMood == MS_ANGRY)) {
    if (millis() > moodUntil) {
      setEyesMood(MS_DEFAULT);
    }
  }

  // (Optional) subtle brightness cue per mood via LED is already in setEyesMood()
}

// ===== Liveliness updater (~25Hz) =====

// ---------- MPU Debug Printing ----------
// last reported values
static float last_ax=0, last_ay=0, last_az=0;
static float last_gx=0, last_gy=0, last_gz=0;

// thresholds to avoid spamming small jitter
const float ACCEL_DELTA = 0.05f;   // g-units
const float GYRO_DELTA  = 5.0f;    // deg/s

void debugPrintIMUIfChanged() {
  float ax,ay,az,gx,gy,gz;
  mpuRead(ax,ay,az,gx,gy,gz);

  bool changed = false;
  if (fabs(ax-last_ax) > ACCEL_DELTA || fabs(ay-last_ay) > ACCEL_DELTA || fabs(az-last_az) > ACCEL_DELTA ||
      fabs(gx-last_gx) > GYRO_DELTA  || fabs(gy-last_gy) > GYRO_DELTA  || fabs(gz-last_gz) > GYRO_DELTA) {
    changed = true;
  }

  if (changed) {
    Serial.printf("MPU ax=%.2f ay=%.2f az=%.2f gx=%.1f gy=%.1f gz=%.1f\n",
      ax, ay, az, gx, gy, gz);
    last_ax=ax; last_ay=ay; last_az=az;
    last_gx=gx; last_gy=gy; last_gz=gz;
  }
}

// ===== Liveliness / IMU reaction =====
static uint32_t imuTickMs = 0;
// (lpf_ax, lpf_ay, lpf_az and lpf_g exist in the emotion engine; reuse them)

// micro-jiggle timer
static uint32_t jiggleNextMs = 0;
static uint32_t jiggleEndMs  = 0;
static bool     jiggling     = false;

// direction nudge cooldown + deadzone
static uint32_t lastDirNudgeMs = 0;
const uint16_t  DIR_NUDGE_COOLDOWN_MS = 700;   // throttle direction nudges
const float     DIR_DEADZONE_DEG      = 12.0f; // ignore tiny tilts (let idle roam)

// thresholds (tweak to taste)
const float TILT_DEG_POSITIONS = 15.0f;   // sector change threshold
const float TAP_SPIKE_DPS      = 140.0f;  // quick rotation spike => blink
const uint32_t TAP_MIN_GAP_MS  = 600;     // min time between blinks
static uint32_t lastBlinkMs = 0;

// --- gentle LFO follow-gravity (non-pinning)
static uint32_t lfoLastMs = 0;
const uint32_t LFO_PERIOD_MS = 8000; // 8s slow cycle
const int LFO_AMPLITUDE_PX = 2;      // ± pixels to nudge when LFO active

// Notification attention animation (non-blocking state machine)
static uint8_t notifAnimState = 0; // 0=idle,1=widen,2=look,3=blink,4=restore
static uint32_t notifAnimT0 = 0;
int notif_orig_wL=0, notif_orig_wR=0, notif_orig_hL=0, notif_orig_hR=0;
MoodState notif_prevMood = MS_DEFAULT;

// helpers: trigger attention animation (called from notification callback)
inline void triggerNotificationAttention() {
  // save original sizes
  notif_orig_wL = Eyes.eyeLwidthNext; notif_orig_wR = Eyes.eyeRwidthNext;
  notif_orig_hL = Eyes.eyeLheightNext; notif_orig_hR = Eyes.eyeRheightNext;
  notif_prevMood = curMood;
  // start animation
  notifAnimState = 1; notifAnimT0 = millis();
  // quick widen
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
        setEyesMood(MS_HAPPY); moodUntil = millis() + 800;
        notifAnimState = 3; notifAnimT0 = now;
      }
      break;
    case 3: // restore sizes and mood
      if (now - notifAnimT0 > 180) {
        Eyes.eyeLwidthNext = notif_orig_wL; Eyes.eyeRwidthNext = notif_orig_wR;
        Eyes.eyeLheightNext = notif_orig_hL; Eyes.eyeRheightNext = notif_orig_hR;
        setEyesMood(notif_prevMood);
        notifAnimState = 0;
      }
      break;
  }
}

// small micro-expression helpers
inline void doWinkLeft(){ Eyes.blink(true, false); }
inline void doWinkRight(){ Eyes.blink(false, true); }
inline void doSmile(uint16_t ms=700){ setEyesMood(MS_HAPPY); moodUntil = millis() + ms; }

inline void updateLivelinessFromIMU() {
  float ax,ay,az,gx,gy,gz;
  mpuRead(ax,ay,az,gx,gy,gz);
  lpf_ax = lpf(lpf_ax, ax);
  lpf_ay = lpf(lpf_ay, ay);
  lpf_az = lpf(lpf_az, az);
  float gmag = sqrtf(gx*gx + gy*gy + gz*gz);
  lpf_g = lpf(lpf_g, gmag, 0.3f);

  // 1) Blink on tap
  if (lpf_g > TAP_SPIKE_DPS && (millis() - lastBlinkMs > TAP_MIN_GAP_MS)) {
    Eyes.blink();
    lastBlinkMs = millis();
  }

  // 2) Rarely nudge look direction IF tilt is clearly pointing somewhere
  float pitch, roll;
  accelToAngles(lpf_ax, lpf_ay, lpf_az, pitch, roll);
  bool strongTilt = (fabsf(pitch) > DIR_DEADZONE_DEG) || (fabsf(roll) > DIR_DEADZONE_DEG);

  if (strongTilt && (millis() - lastDirNudgeMs >= DIR_NUDGE_COOLDOWN_MS)) {
    uint8_t d = dirFromAngles(pitch, roll);
    if (d != DEFAULT) {
      Eyes.setPosition(d);          // brief hint
      lastDirNudgeMs = millis();
    }
    // If d == DEFAULT, do nothing—let idle wander naturally.
  }

  // optional: rare random curiosity nudge when flat
  if (!strongTilt && (millis() - lastDirNudgeMs > 2500)) {
    if (random(0,100) < 8) { // ~8% chance every ~2.5s
      static const uint8_t picks[] = {N, NE, E, SE, S, SW, W, NW};
      Eyes.setPosition(picks[random(0, 8)]);
      lastDirNudgeMs = millis();
    }
  }

  // 3) micro-jiggle (unchanged)
  uint32_t now = millis();
  if (!jiggling && now >= jiggleNextMs) {
    jiggling = true;
    jiggleEndMs = now + 120 + (uint32_t)random(0, 120);
    Eyes.setHFlicker(true, 1);
    if (random(0,3)==0) Eyes.setVFlicker(true, 1);
  }
  if (jiggling && now >= jiggleEndMs) {
    jiggling = false;
    Eyes.setHFlicker(false, 0);
    Eyes.setVFlicker(false, 0);
    scheduleNextJiggle();
  }

}

// map pitch/roll to cardinal direction (RoboEyes::setPosition expects N, NE, ... DEFAULT)
inline uint8_t dirFromAngles(float pitchDeg, float rollDeg) {
  float x = rollDeg;   // left(-) / right(+)
  float y = -pitchDeg; // up(+)/ down(-)
  if (fabsf(x) < TILT_DEG_POSITIONS && fabsf(y) < TILT_DEG_POSITIONS) return DEFAULT;
  float a = atan2f(y, x) * 57.2958f; // -180..180
  if (a >= -22.5f && a < 22.5f)   return E;
  if (a >= 22.5f  && a < 67.5f)   return NE;
  if (a >= 67.5f  && a < 112.5f)  return N;
  if (a >= 112.5f && a < 157.5f)  return NW;
  if (a >= -67.5f && a < -22.5f)  return SE;
  if (a >= -112.5f&& a < -67.5f)  return S;
  if (a >= -157.5f&& a < -112.5f) return SW;
  return W;
}

// schedule a tiny flicker burst every so often
inline void scheduleNextJiggle() {
  jiggleNextMs = millis() + 800 + (uint32_t)random(0, 2200);  // 0.8–3.0s
}


// ---------- Faces ----------
enum FaceMode : uint8_t { FACE_EYES=0, FACE_CLOCK=1, FACE_NOTIF=2, FACE_COUNT };
FaceMode mode = FACE_EYES;

// ---------- BLE / Chronos ----------

ChronosESP32 chrono("PetBot");
bool bleEnabled = true;

// notif cache
String lastNotifTitle, lastNotifBody;

// helpers
static String zeroPad2(int v){ char b[3]; snprintf(b, sizeof(b), "%02d", v); return String(b); }




// ---------- Input handling (debounce + state machine) ----------
static bool edgeRising(uint8_t pin, bool activeHigh=true, uint16_t debounceMs=30) {
  static uint32_t t0=0; static bool lastRaw=false, stable=false;
  bool raw = activeHigh ? (digitalRead(pin)==HIGH) : (digitalRead(pin)==LOW);
  if (raw != lastRaw) { lastRaw = raw; t0 = millis(); }
  if (millis()-t0 > debounceMs) {
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

// ---------- Drawing helpers ----------
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
  // clear the band
  display.fillRect(0, 0, SCREEN_W, 12, SH110X_BLACK);

  // BLE icon (left)
  drawIcon(0, 0, ICON_BLE_12);
  if (!bleEnabled || !chrono.isConnected()) {
    // strike-through if disabled/disconnected
    display.drawLine(0, 11, 11, 0, SH110X_WHITE);
  }

  // play/pause hint (center-left)
  if (playingHint) {
    drawIcon(16, 0, ICON_PLAY_12);
  }

  // Notif bell (right)
  drawIcon(SCREEN_W - 12, 0, ICON_BELL_12);
  if (hasNotif()) {
    // tiny badge dot
    display.fillCircle(SCREEN_W - 3, 2, 2, SH110X_WHITE);
  }
}

// Bottom dock (16 px tall): Eyes | Clock | Notif, highlight current
void drawModeDock(FaceMode m) {
  const int y = SCREEN_H - 16;
  display.fillRect(0, y, SCREEN_W, 16, SH110X_BLACK);
  // grid positions
  int x1 = 20, x2 = SCREEN_W/2 - 6, x3 = SCREEN_W - 32;

  drawIcon(x1, y+2, ICON_EYES_12);
  drawIcon(x2, y+2, ICON_CLOCK_12);
  drawIcon(x3, y+2, ICON_BELL_12);

  // underline active
  int uW = 18;
  int ux = (m==FACE_EYES) ? (x1-3) : (m==FACE_CLOCK ? (x2-3) : (x3-3));
  display.fillRect(ux, y+14, uW, 2, SH110X_WHITE);
}

// Toast overlay (centered)
void drawToastIfAny() {
  if (millis() > toastUntil || toastText.length()==0) return;
  // semi-opaque box (just invert band)
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

  String hhmmss, dateStr;

  if (chrono.isConnected()) {
    hhmmss  = chrono.getHourZ() + ":" + zeroPad2(chrono.getMinute()) + ":" + zeroPad2(chrono.getSecond());
    dateStr = zeroPad2(chrono.getDay()) + "/" + zeroPad2(chrono.getMonth());
  } else {
    time_t t = time(NULL); struct tm tmnow; localtime_r(&t, &tmnow);
    char tbuf[9]; snprintf(tbuf,sizeof(tbuf),"%02d:%02d:%02d", tmnow.tm_hour, tmnow.tm_min, tmnow.tm_sec);
    char dbuf[6]; snprintf(dbuf,sizeof(dbuf),"%02d/%02d", tmnow.tm_mday, tmnow.tm_mon+1);
    hhmmss = tbuf; dateStr = dbuf;
  }

  // date top-left
  display.setTextSize(1);
  display.setCursor(0,0); display.print(dateStr);

  // time centered
  int16_t x1,y1; uint16_t w,h;
  display.setTextSize(3);
  display.getTextBounds(hhmmss,0,0,&x1,&y1,&w,&h);
  display.setCursor((SCREEN_W - w)/2, (SCREEN_H - h)/2);
  display.print(hhmmss);

  // BLE status bottom-left
  display.setTextSize(1);
  display.setCursor(0, SCREEN_H-10);
  if (bleEnabled) display.print(chrono.isConnected() ? "BLE: Chronos" : "BLE: -");
  else            display.print("BLE: OFF");

  // HUD
  drawStatusBar(true);         // show play icon as a hint if you later add audio
  drawModeDock(FACE_CLOCK);
  drawToastIfAny();
  display.display();

}

// Face: Notification
void drawNotif() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE); display.setTextSize(1);

  if (lastNotifTitle.length()==0 && lastNotifBody.length()==0) {
    drawCenterText("No notifications");
    return;
  }
  display.setCursor(0,0);  display.println("Last notif:");
  display.setCursor(0,12); display.println(lastNotifTitle.substring(0,21));
  display.setCursor(0,24); display.println(lastNotifBody.substring(0, (SCREEN_W/6)*3)); // quick wrap
  drawStatusBar(false);
  drawModeDock(FACE_NOTIF);
  drawToastIfAny();
  display.display();

}

// ---------- Arduino setup/loop ----------
void setup() {
  Serial.begin(115200);
  pinMode(TOUCH_PIN, INPUT_PULLDOWN);
  pinMode(FUNC_BTN,  INPUT_PULLUP);

  Wire.begin(I2C_SDA, I2C_SCL, 400000);
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

  // Draw active face
  switch (mode) {
    case FACE_EYES:
      // Keep the face clean: RoboEyes draws into the buffer. Do not draw HUD
      // or dock here so the face remains uncluttered and stable.
      Eyes.update();
      // push whatever the face drew (RoboEyes no longer calls display.display())
      display.display();
      break;

    case FACE_CLOCK:  drawClock();   break;
    case FACE_NOTIF:  drawNotif();   break;
  }

  // small pacing to avoid excessive I2C spam on non-eyes faces
  if (mode != FACE_EYES) delay(120);

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
