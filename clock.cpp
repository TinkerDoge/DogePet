// Clock Face Module - Implementation
#include "clock.h"
#include "config.h"
#include <stdint.h>

// Font includes  
#include "include/TechTime37.h"
#include "include/Pixeboy20.h"

// Helper function for zero-padding
static String zeroPad2(int v) { 
  char b[3]; 
  snprintf(b, sizeof(b), "%02d", v); 
  return String(b); 
}

// Frame constants (from main)
static const int FRAME_X = 0;
static const int FRAME_Y = 0;  
static const int FRAME_W = SCREEN_W - 0;
static const int FRAME_H = 64;
static const uint8_t FRAME_RADIUS = 8;
static const uint8_t FRAME_THICKNESS = 2;

// UI helper functions
static inline void drawSharedFrame(Adafruit_SH1106G& display, int fx, int fy, int fw, int fh) {
  for (uint8_t t=0; t<FRAME_THICKNESS; ++t) {
    display.drawRoundRect(fx + t, fy + t, fw - 2*t, fh - 2*t, FRAME_RADIUS, SH110X_WHITE);
  }
}

static inline int measureAdvance_Pixeboy(const String &s) {
  int adv = 0;
  for (size_t i=0;i<s.length();++i) {
    uint8_t cp = (uint8_t)s[i];
    int16_t gi = Pixeboy20::findGlyph(cp);
    if (gi < 0) gi = Pixeboy20::findGlyph('0');
    adv += (int8_t)pgm_read_byte(&Pixeboy20::Glyphs[gi].xAdvance);
  }
  return adv;
}

static inline int headerBaselineY(int fy) {
  return fy + 2 + Pixeboy20::Ascent;
}

static inline int headerBottomY(int fy) {
  return headerBaselineY(fy) - Pixeboy20::Descent; // Descent is negative
}

static inline void drawHeaderDateBattery(Adafruit_SH1106G& display, int fx, int fy, int fw, const String &dateStr, const String &batStr) {
  int16_t baseY = headerBaselineY(fy);
  String bat = batteryCharging ? String("CHG") : batStr; // show CHG when charging
  // Left side: date or SLNT + date
  if (silentMode) {
    String sl = "SL";
    Pixeboy20::drawTextUTF8(display, fx + 4, baseY, sl.c_str(), SH110X_WHITE);
    int dW = measureAdvance_Pixeboy(sl) + 6;
    Pixeboy20::drawTextUTF8(display, fx + 4 + dW, baseY, dateStr.c_str(), SH110X_WHITE);
  } else {
    Pixeboy20::drawTextUTF8(display, fx + 4, baseY, dateStr.c_str(), SH110X_WHITE);
  }
  // Right side: battery percent (+ charging marker)
  int bw = measureAdvance_Pixeboy(bat);
  int16_t bx = fx + fw - 4 - bw;
  Pixeboy20::drawTextUTF8(display, bx, baseY, bat.c_str(), SH110X_WHITE);
}

// Battery reading function
float readVBATVolts() {
  // Average a few samples
  uint32_t mvAcc = 0;
  for (uint8_t i=0;i<VBAT_SAMPLES;i++) mvAcc += analogReadMilliVolts(VBAT_PIN);
  float mvAvg = (float)mvAcc / (float)VBAT_SAMPLES;
  float v_pin = mvAvg / 1000.0f; // calibrated millivolts -> volts
  // Divider is 1:2 → battery voltage is 2x pin voltage
  return v_pin * 2.0f * VBAT_CAL;
}

int voltsToPercent(float v) {
  if (v <= VBAT_MIN_V) return 0;
  if (v >= VBAT_MAX_V) return 100;
  float pct = (v - VBAT_MIN_V) / (VBAT_MAX_V - VBAT_MIN_V) * 100.0f;
  if (pct < 0) pct = 0; if (pct > 100) pct = 100;
  return (int)(pct + 0.5f);
}

// Main clock drawing function
void drawClock(Adafruit_SH1106G& display, ChronosESP32& chrono) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  // Build strings (with seconds)
  String timeStr, dateStr, batStr;
  String hhStr, mmStr, ssStr;
  if (chrono.isConnected()) {
    hhStr = chrono.getHourZ();
    mmStr = zeroPad2(chrono.getMinute());
    ssStr = zeroPad2(chrono.getSecond());
    timeStr = hhStr + ":" + mmStr + ":" + ssStr;
    dateStr = zeroPad2(chrono.getDay()) + "/" + zeroPad2(chrono.getMonth());
    // local device battery
    if (millis() - lastVbatReadMs > VBAT_CACHE_MS || vbatPercent < 0) {
      vbatVolts = readVBATVolts();
      vbatPercent = voltsToPercent(vbatVolts);
      lastVbatReadMs = millis();
      // charging detect with hysteresis and consecutive confirmation
      if (vbatVolts >= VBAT_CHARGE_V) {
        if (vbatChargeCount < 255) vbatChargeCount++;
        if (vbatChargeCount >= VBAT_CHARGE_MIN_COUNT) batteryCharging = true;
      } else if (vbatVolts < (VBAT_CHARGE_V - VBAT_CHARGE_HYST_V)) {
        vbatChargeCount = 0;
        batteryCharging = false;
      }
    }
    batStr  = String(vbatPercent) + "%";
  } else {
    time_t t = time(NULL); struct tm tmnow; localtime_r(&t, &tmnow);
    char tbuf[9]; snprintf(tbuf,sizeof(tbuf),"%02d:%02d:%02d", tmnow.tm_hour, tmnow.tm_min, tmnow.tm_sec);
    char dbuf[6]; snprintf(dbuf,sizeof(dbuf),"%02d/%02d", tmnow.tm_mday, tmnow.tm_mon+1);
    timeStr = tbuf; dateStr = dbuf;
    char hbuf[3]; snprintf(hbuf,sizeof(hbuf),"%02d", tmnow.tm_hour); hhStr = hbuf;
    char mbuf[3]; snprintf(mbuf,sizeof(mbuf),"%02d", tmnow.tm_min);  mmStr = mbuf;
    char sbuf[3]; snprintf(sbuf,sizeof(sbuf),"%02d", tmnow.tm_sec);  ssStr = sbuf;
    if (millis() - lastVbatReadMs > VBAT_CACHE_MS || vbatPercent < 0) {
      vbatVolts = readVBATVolts();
      vbatPercent = voltsToPercent(vbatVolts);
      lastVbatReadMs = millis();
      // charging detect with hysteresis and consecutive confirmation
      if (vbatVolts >= VBAT_CHARGE_V) {
        if (vbatChargeCount < 255) vbatChargeCount++;
        if (vbatChargeCount >= VBAT_CHARGE_MIN_COUNT) batteryCharging = true;
      } else if (vbatVolts < (VBAT_CHARGE_V - VBAT_CHARGE_HYST_V)) {
        vbatChargeCount = 0;
        batteryCharging = false;
      }
    }
    batStr = String(vbatPercent >= 0 ? vbatPercent : 0) + "%";
  }

  // Frame (shared layout)
  const int fx = FRAME_X, fy = FRAME_Y;
  const int fw = FRAME_W, fh = FRAME_H;
  drawSharedFrame(display, fx, fy, fw, fh);

  // Header row inside frame using Pixeboy20 (date left, battery right)
  drawHeaderDateBattery(display, fx, fy, fw, dateStr, batStr);
  {
    // Compute vertical baseline centered in the space below the header row
    int16_t headerBottom = headerBottomY(fy);
    int16_t contentTop = headerBottom + 10;  // small gap below header
    int16_t contentBottom = fy + fh - 2;    // bottom padding
    int16_t availCenter = (contentTop + contentBottom) / 2;
    int16_t glyphHalf = (TechTime37::Ascent - TechTime37::Descent) / 2;
    int16_t baseY = availCenter + glyphHalf;
    // Measure fixed widths using reference strings
    auto measureTech = [](const String &s){
      int adv=0; for (size_t i=0;i<s.length();++i){ uint8_t cp=(uint8_t)s[i]; int16_t gi=TechTime37::findGlyph(cp); if (gi<0) gi=TechTime37::findGlyph('0'); adv += (int8_t)pgm_read_byte(&TechTime37::Glyphs[gi].xAdvance);} return adv; };
    int wHH = measureTech("00");
    int wMM = measureTech("00");
    int wSS = measureTech("00");
    int wColon = measureTech(":");
    int totalRef = wHH + wColon + wMM + wColon + wSS;
    int x0 = fx + (fw - totalRef)/2; // center the fixed layout
    int xHH = x0;
    int xC1 = xHH + wHH;
    int xMM = xC1 + wColon;
    int xC2 = xMM + wMM;
    int xSS = xC2 + wColon;
    TechTime37::drawTextUTF8(display, xHH, baseY, hhStr.c_str(), SH110X_WHITE);
    TechTime37::drawTextUTF8(display, xC1, baseY, ":", SH110X_WHITE);
    TechTime37::drawTextUTF8(display, xMM, baseY, mmStr.c_str(), SH110X_WHITE);
    TechTime37::drawTextUTF8(display, xC2, baseY, ":", SH110X_WHITE);
    TechTime37::drawTextUTF8(display, xSS, baseY, ssStr.c_str(), SH110X_WHITE);
  }

  display.display();
}
