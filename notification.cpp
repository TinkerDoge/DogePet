// Notification Face Module - Implementation
#include "notification.h"
#include "config.h"
#include <stdint.h>

// Ensure stdint types are available
#if !defined(__STDINT_H) && !defined(_STDINT_H_)
typedef unsigned long uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef long int32_t;
typedef short int16_t;
typedef signed char int8_t;
#endif

// Font includes
#include "include/ShopeeRegular12.h"
#include "include/ToastRenderer.h"

// Frame constants (from main)
static const int FRAME_X = 0;
static const int FRAME_Y = 0;  
static const int FRAME_W = SCREEN_W - 0;
static const int FRAME_H = 64;
static const uint8_t FRAME_RADIUS = 8;
static const uint8_t FRAME_THICKNESS = 2;

// Scatter drawing state (stable per character)
static int16_t scatterX[200];
static int16_t scatterY[250];

// UI helper functions
static inline void drawSharedFrame(Adafruit_SH1106G& display, int fx, int fy, int fw, int fh) {
  for (uint8_t t=0; t<FRAME_THICKNESS; ++t) {
    display.drawRoundRect(fx + t, fy + t, fw - 2*t, fh - 2*t, FRAME_RADIUS, SH110X_WHITE);
  }
}

// returns whether we have any cached notification text
bool hasNotif() {
  return lastNotifTitle[0] != '\0' || lastNotifBody[0] != '\0';
}

// Toast overlay (top banner or scatter) - optimized to reduce flicker with typewriter support
void drawToastIfAny(Adafruit_SH1106G& display) {
  uint32_t currentMs = millis();
  bool shouldBeVisible = (currentMs <= toastUntil && toastFullText[0] != '\0');

  // Handle typewriter effect update
  if (shouldBeVisible && toastTypewriter && toastTypePos < strlen(toastFullText) && 
      (currentMs - lastToastTypeMs >= toastTypeSpeed)) {
    // Add next character
    toastText[toastTypePos] = toastFullText[toastTypePos];
    toastText[toastTypePos + 1] = '\0'; // Null terminate
    // If scatter mode, precompute a stable random position for this character index
    if (toastScatter) {
      int idx = (int)toastTypePos;
      // constrain to visible area (avoid top 0..1 to not clip)
      scatterX[idx] = (int16_t)(esp_random() % (SCREEN_W - 6));
      scatterY[idx] = (int16_t)(2 + (esp_random() % (SCREEN_H - 10)));
    }
    toastTypePos++;
    lastToastTypeMs = currentMs;
  }

  // Handle toast timeout - clear the toast when it expires
  if (toastVisible && !shouldBeVisible) {
    toastVisible = false;
    toastTypewriter = false;
    toastUntil = 0; // Reset timeout
    toastText[0] = '\0';
    toastFullText[0] = '\0';
    return;
  }

  // Only draw if toast should be visible
  if (shouldBeVisible) {
    display.setTextSize(1);

    if (toastScatter) {
      // Draw each typed character at a stable random position without any frame
      for (uint8_t i = 0; i < toastTypePos && toastFullText[i] != '\0'; ++i) {
        char c = toastFullText[i];
        if (c == ' ') continue; // skip spaces for nicer scatter
        int rx = scatterX[i];
        int ry = scatterY[i];
        
        // Use ToastRenderer for emoji support even in scatter mode
        char singleChar[2] = {c, '\0'};
        ToastRenderer::drawToastText(display, rx, ry + ToastFont12::Ascent, String(singleChar), SH110X_WHITE);
      }
      // No cursor in scatter mode to avoid jitter
    } else {
      // Fixed top banner area (full width) to avoid re-centering jitter
      const int by = 0;               // top row
      const int bh = 14;              // banner height
      const int bx = 0;               // full width
      const int bw = SCREEN_W;

      // Clear banner area only (avoid touching other content)
      display.fillRect(bx, by, bw, bh, SH110X_BLACK);
      if (!toastNoFrame) display.drawRect(bx, by, bw, bh, SH110X_WHITE);

      // Measure current text width for cursor placement
      int16_t x1, y1; uint16_t w, h;
      ToastRenderer::getToastTextBounds(String(toastText), x1, y1, w, h);

      // Left padding inside banner
      const int padX = 4;
      const int padY = 2;
      
      // Use ToastRenderer for emoji support
      int textY = by + padY + ToastFont12::Ascent; // Baseline position
      ToastRenderer::drawToastText(display, bx + padX, textY, String(toastText), SH110X_WHITE);

      // Add cursor for typewriter effect
      if (toastTypewriter && toastTypePos < strlen(toastFullText)) {
        if ((currentMs / 150) % 2 == 0) { // fast blink
          ToastRenderer::drawToastText(display, bx + padX + (int)w, textY, "_", SH110X_WHITE);
        }
      }
    }
  }
}

// Face: Notification popup (framed like clock)
void drawNotif(Adafruit_SH1106G& display) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE); display.setTextSize(1);
  const int fx = FRAME_X, fy = FRAME_Y;     // outer margins (left/top)
  const int fw = FRAME_W, fh = FRAME_H;     // frame size (width/height)
  // Draw frame with configurable thickness
  drawSharedFrame(display, fx, fy, fw, fh);

  if (lastNotifTitle[0]=='\0' && lastNotifBody[0]=='\0') {
    // placeholder
    display.setCursor(fx+4, fy+2); display.print("No notifications");
  } else {
    // Title (row 1): app name/logo using size-12 font
    int16_t titleBaseY = fy + 4 + ShopeeRegular12::Ascent;
    ShopeeRegular12::drawTextUTF8(display, fx+4, titleBaseY, lastNotifTitle, SH110X_WHITE);

    // Body text (rows 2-4)
    char body[sizeof(lastNotifBody)];
    strcpy(body, lastNotifBody);
    for (char* p = body; *p; ++p) if (*p == '\n') *p = ' ';
    const int viewW = fw - 8;
    // Compute as many message rows as fit below the title (usually 3; may allow a 4th if space)
    int16_t rowY[6];
    int rows = 0;
    {
      int16_t y = titleBaseY + ShopeeRegular12::YAdvance;
      int16_t maxY = fy + fh - 2; // bottom padding inside frame
      while (y - ShopeeRegular12::Descent <= maxY && rows < 6) {
        rowY[rows++] = y;
        y += ShopeeRegular12::YAdvance;
      }
      if (rows == 0) rows = 1; // safety
    }

    // Measure a single space advance once
    int spaceAdv = 0; {
      int16_t gi = ShopeeRegular12::findGlyph(' ');
      if (gi < 0) gi = ShopeeRegular12::findGlyph('?');
      spaceAdv = (int8_t)pgm_read_byte(&ShopeeRegular12::Glyphs[gi].xAdvance);
    }
    auto measureAdv = [&](const char* s){
      int adv = 0; for (int j=0; s[j]; ++j){ uint8_t c=(uint8_t)s[j]; int16_t gi=ShopeeRegular12::findGlyph(c); if (gi<0) gi=ShopeeRegular12::findGlyph('?'); adv += (int8_t)pgm_read_byte(&ShopeeRegular12::Glyphs[gi].xAdvance);} return adv; };

    // Word-wrap into available rows; if doesn't fit, cut and append "..."
    bool hasMore = false;
    char linesBuf[6][128];
    for (int r=0;r<6;r++) linesBuf[r][0] = '\0';
    char* lines[6] = { linesBuf[0], linesBuf[1], linesBuf[2], linesBuf[3], linesBuf[4], linesBuf[5] };
    int   lineCount = 0;

    int i = 0;
    while (body[i] && lineCount < rows) {
      int lineW = 0;
      bool firstWord = true;
      int out = 0;
      char* lb = lines[lineCount];

      while (body[i]) {
        while (body[i] == ' ') i++;
        if (!body[i]) break;
        int wstart = i; int wwidth = 0; int wlen = 0;
        while (body[i] && body[i] != ' ') {
          uint8_t c = (uint8_t)body[i++]; wlen++;
          int16_t gi = ShopeeRegular12::findGlyph(c);
          if (gi < 0) gi = ShopeeRegular12::findGlyph('?');
          wwidth += (int8_t)pgm_read_byte(&ShopeeRegular12::Glyphs[gi].xAdvance);
        }
        int add = wwidth + (firstWord ? 0 : spaceAdv);
        if (lineW + add <= viewW) {
          if (!firstWord) { lb[out++] = ' '; }
          for (int k=0; k<wlen; ++k) lb[out++] = body[wstart + k];
          lineW += add;
          firstWord = false;
        } else {
          if (lineW == 0) { hasMore = true; }
          i = wstart; // start next line with this word
          break;
        }
      }
      lb[out] = '\0';
      if (out == 0) break;
      lineCount++;
    }
    for (; body[i]; ++i) { if (body[i] != ' ') { hasMore = true; break; } }

    // If overflow, truncate the last visible line and add ellipsis
    if (hasMore && lineCount > 0) {
      int lastIdx = (lineCount > rows) ? (rows - 1) : (lineCount - 1);
      char* last = lines[lastIdx];
      // Compute width of ellipsis
      int ellAdv = 0; { int16_t gi = ShopeeRegular12::findGlyph('.'); if (gi < 0) gi = ShopeeRegular12::findGlyph('?'); int a=(int8_t)pgm_read_byte(&ShopeeRegular12::Glyphs[gi].xAdvance); ellAdv = a*3; }
      // Trim from end until it fits with ellipsis
      int curW = measureAdv(last);
      int len = (int)strlen(last);
      while (len > 0 && curW + ellAdv > viewW) { last[--len] = '\0'; curW = measureAdv(last); }
      // Append ellipsis if it fits
      if (ellAdv <= viewW) {
        int remain = (int)sizeof(linesBuf[0]) - 1 - len;
        if (remain >= 3) { last[len++]='.'; last[len++]='.'; last[len++]='.'; last[len]='\0'; }
      }
      lineCount = min(lineCount, rows);
    }

    // Draw wrapped (possibly truncated) lines
    for (int r=0; r<lineCount && r<rows; ++r) {
      ShopeeRegular12::drawTextUTF8(display, fx+4, rowY[r], lines[r], SH110X_WHITE);
    }
  }

  drawToastIfAny(display);
  display.display();
}
