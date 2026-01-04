// Face.cpp - Display and eye animation management
#include "Face.h"

// Define static members
Adafruit_SH1106G Face::display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
roboEyes Face::eyes(Face::display);
DisplayMode Face::currentMode = DisplayMode::Eyes;
DisplayMode Face::previousMode = DisplayMode::Eyes;
uint32_t Face::sleepAnimMs = 0;
char Face::toastText[64] = {0};
uint32_t Face::toastEndMs = 0;

void Face::init() {
    // Note: Wire.begin() should be called in setup() before this
    
    if (!display.begin(SCREEN_ADDR, true)) {
        Serial.println("{\"status\":\"error\",\"msg\":\"OLED not found\"}");
        return;
    }
    
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.display();
    
    // Initialize Eyes (but don't draw yet)
    eyes.begin(SCREEN_W, SCREEN_H, 50);
    eyes.setWidth(EYE_WIDTH, EYE_WIDTH);
    eyes.setHeight(EYE_HEIGHT, EYE_HEIGHT);
    eyes.setBorderradius(EYE_BORDER_RADIUS, EYE_BORDER_RADIUS);
    eyes.setSpacebetween(EYE_SPACING);
    eyes.setAutoblinker(false, 3, 4); // Off during boot
    eyes.setIdleMode(false, 4, 5);    // Off during boot
    
    currentMode = DisplayMode::Custom;  // Start in custom mode for boot sequence
}

void Face::showBootScreen(const char* msg) {
    setMode(DisplayMode::Custom);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("DogeBot");
    display.println("Booting...");
    display.println("");
    display.println(msg);
    display.display();
}

void Face::updateProgressBar(int percent, const char* status) {
    display.fillRect(0, 24, 128, 40, SH110X_BLACK);
    int barW = 100;
    int barH = 10;
    int barX = (128 - barW) / 2;
    int barY = 30;
    display.drawRect(barX, barY, barW, barH, SH110X_WHITE);
    int fillW = (barW - 4) * percent / 100;
    display.fillRect(barX + 2, barY + 2, fillW, barH - 4, SH110X_WHITE);
    display.setCursor(0, 45);
    display.println(status);
    display.display();
}

void Face::playLineAnimation() {
    for (int i = 0; i < 64; i += 2) {
        display.drawLine(0, i, 127, i, SH110X_WHITE);
        display.display();
        delay(5);
        display.drawLine(0, i, 127, i, SH110X_BLACK);
    }
    display.display();
}

// ============================================================================
// Display Mode Control
// ============================================================================

void Face::setMode(DisplayMode mode) {
    if (mode == currentMode) return;
    
    previousMode = currentMode;
    currentMode = mode;
    
    Serial.printf("{\"status\":\"info\",\"msg\":\"Face mode: %d\"}\n", (int)mode);
}

DisplayMode Face::getMode() {
    return currentMode;
}

void Face::enableEyes(bool enable) {
    if (enable) {
        setMode(DisplayMode::Eyes);
    } else {
        setMode(DisplayMode::Custom);
    }
}

bool Face::areEyesEnabled() {
    return currentMode == DisplayMode::Eyes;
}

// ============================================================================
// Main Update - Only draws based on current mode
// ============================================================================

void Face::update() {
    // Handle toast timeout
    if (currentMode == DisplayMode::Toast && millis() >= toastEndMs) {
        currentMode = previousMode;  // Restore previous mode
    }
    
    switch (currentMode) {
        case DisplayMode::Eyes:
            eyes.update();  // RoboEyes handles its own display
            break;
            
        case DisplayMode::Sleep:
            drawSleepFace();
            break;
            
        case DisplayMode::Toast:
            drawToast();
            break;
            
        case DisplayMode::Custom:
            // User controls display directly - do nothing
            break;
            
        case DisplayMode::Off:
            // Display is off - do nothing
            break;
    }
}

// ============================================================================
// Toast System
// ============================================================================

void Face::showToast(const char* text, uint32_t durationMs) {
    strncpy(toastText, text, sizeof(toastText) - 1);
    toastText[sizeof(toastText) - 1] = '\0';
    toastEndMs = millis() + durationMs;
    
    if (currentMode != DisplayMode::Toast) {
        previousMode = currentMode;
    }
    currentMode = DisplayMode::Toast;
}

bool Face::isToastActive() {
    return currentMode == DisplayMode::Toast;
}

void Face::drawToast() {
    display.clearDisplay();
    
    // Draw centered text with border
    int16_t x1, y1;
    uint16_t w, h;
    display.setTextSize(1);
    display.getTextBounds(toastText, 0, 0, &x1, &y1, &w, &h);
    
    int boxPad = 4;
    int boxX = (SCREEN_W - w - boxPad*2) / 2;
    int boxY = (SCREEN_H - h - boxPad*2) / 2;
    
    display.fillRoundRect(boxX, boxY, w + boxPad*2, h + boxPad*2, 3, SH110X_WHITE);
    display.setTextColor(SH110X_BLACK);
    display.setCursor(boxX + boxPad, boxY + boxPad);
    display.print(toastText);
    display.setTextColor(SH110X_WHITE);  // Reset
    
    display.display();
}

// ============================================================================
// Sleep Face
// ============================================================================

void Face::drawSleepFace() {
    display.clearDisplay();
    
    // Draw closed eyes (horizontal lines)
    int eyeY = SCREEN_H / 2;
    int eyeW = EYE_WIDTH;
    int leftX = (SCREEN_W / 2) - EYE_SPACING/2 - eyeW;
    int rightX = (SCREEN_W / 2) + EYE_SPACING/2;
    
    display.drawLine(leftX, eyeY, leftX + eyeW, eyeY, SH110X_WHITE);
    display.drawLine(leftX + 2, eyeY - 1, leftX + eyeW - 2, eyeY - 1, SH110X_WHITE);
    display.drawLine(rightX, eyeY, rightX + eyeW, eyeY, SH110X_WHITE);
    display.drawLine(rightX + 2, eyeY - 1, rightX + eyeW - 2, eyeY - 1, SH110X_WHITE);
    
    // Animated Zzz
    uint32_t elapsed = millis() - sleepAnimMs;
    int zOffset = (elapsed / 500) % 3;
    int zBaseX = SCREEN_W - 25;
    int zBaseY = 10 + zOffset * 2;
    
    display.setTextSize(1);
    display.setCursor(zBaseX, zBaseY);
    display.print("z");
    display.setCursor(zBaseX + 6, zBaseY - 5);
    display.print("Z");
    display.setCursor(zBaseX + 14, zBaseY - 12);
    display.print("Z");
    
    display.display();
}

// ============================================================================
// Power State Faces
// ============================================================================

void Face::showSleepFace() {
    sleepAnimMs = millis();
    eyes.setAutoblinker(false, 3, 4);
    eyes.setIdleMode(false, 4, 5);
    eyes.tired = true;
    eyes.close();
    setMode(DisplayMode::Sleep);
    Serial.println("{\"status\":\"info\",\"msg\":\"Face: Sleep mode\"}");
}

void Face::showDimFace() {
    eyes.tired = true;
    eyes.open();
    eyes.setAutoblinker(true, 6, 3);
    eyes.setIdleMode(false, 4, 5);
    setMode(DisplayMode::Eyes);
    Serial.println("{\"status\":\"info\",\"msg\":\"Face: Dim mode\"}");
}

void Face::showActiveFace() {
    eyes.tired = false;
    eyes.open();
    eyes.setAutoblinker(EYE_AUTO_BLINK, BLINK_INTERVAL, BLINK_VARIATION);
    eyes.setIdleMode(EYE_IDLE_MODE, IDLE_INTERVAL, IDLE_VARIATION);
    setMode(DisplayMode::Eyes);
    Serial.println("{\"status\":\"info\",\"msg\":\"Face: Active mode\"}");
}

// ============================================================================
// Accessors
// ============================================================================

roboEyes* Face::getEyes() { return &eyes; }
Adafruit_SH1106G* Face::getDisplay() { return &display; }

// ============================================================================
// Test Helpers
// ============================================================================

void Face::showTestPattern() {
    setMode(DisplayMode::Custom);
    display.clearDisplay();
    for (int y = 0; y < 64; y += 8) {
        for (int x = 0; x < 128; x += 8) {
            if ((x + y) % 16 == 0) display.fillRect(x, y, 8, 8, SH110X_WHITE);
        }
    }
    display.drawRect(0, 0, 128, 64, SH110X_WHITE);
    display.drawLine(64, 0, 64, 63, SH110X_WHITE);
    display.drawLine(0, 32, 127, 32, SH110X_WHITE);
    display.display();
}

void Face::clear() {
    setMode(DisplayMode::Custom);
    display.clearDisplay();
    display.display();
}

void Face::showText(const char* text) {
    setMode(DisplayMode::Custom);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println(text);
    display.display();
}
