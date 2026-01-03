#include "include/Face.h"

// Define static members
Adafruit_SH1106G Face::display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
roboEyes Face::eyes(Face::display);
bool Face::isSleeping = false;
uint32_t Face::sleepAnimMs = 0;

void Face::init() {
    // Initialize Display
    // Note: Wire should be begun in main setup or here? 
    // We assume Wire.begin() is called in setup() before Module init
    
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
}

void Face::showBootScreen(const char* msg) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("DogeBot");
    display.println("Booting...");
    display.println("");
    display.println(msg);
    display.display();
}

void Face::updateProgressBar(int percent, const char* status) {
    // Keep top text
    display.fillRect(0, 24, 128, 40, SH110X_BLACK); // Clear bottom part
    
    // Draw Bar
    int barW = 100;
    int barH = 10;
    int barX = (128 - barW) / 2;
    int barY = 30;
    
    display.drawRect(barX, barY, barW, barH, SH110X_WHITE);
    int fillW = (barW - 4) * percent / 100;
    display.fillRect(barX + 2, barY + 2, fillW, barH - 4, SH110X_WHITE);
    
    // Status text
    display.setCursor(0, 45);
    display.println(status);
    display.display();
}

void Face::playLineAnimation() {
    // Simple scanner effect
    for (int i = 0; i < 64; i += 2) {
        display.drawLine(0, i, 127, i, SH110X_WHITE);
        display.display();
        delay(5);
        display.drawLine(0, i, 127, i, SH110X_BLACK);
    }
    display.display();
}

void Face::update() {
    if (isSleeping) {
        // Draw sleeping face with animated Zzz
        display.clearDisplay();
        
        // Draw closed eyes (horizontal lines)
        int eyeY = SCREEN_H / 2;
        int eyeW = EYE_WIDTH;
        int leftX = (SCREEN_W / 2) - EYE_SPACING/2 - eyeW;
        int rightX = (SCREEN_W / 2) + EYE_SPACING/2;
        
        // Closed eye lines (slightly curved look with 3 lines)
        display.drawLine(leftX, eyeY, leftX + eyeW, eyeY, SH110X_WHITE);
        display.drawLine(leftX + 2, eyeY - 1, leftX + eyeW - 2, eyeY - 1, SH110X_WHITE);
        display.drawLine(rightX, eyeY, rightX + eyeW, eyeY, SH110X_WHITE);
        display.drawLine(rightX + 2, eyeY - 1, rightX + eyeW - 2, eyeY - 1, SH110X_WHITE);
        
        // Animated Zzz - float up and down
        uint32_t elapsed = millis() - sleepAnimMs;
        int zOffset = (elapsed / 500) % 3;  // 0, 1, 2 cycle
        int zBaseX = SCREEN_W - 25;
        int zBaseY = 10 + zOffset * 2;  // Gentle float
        
        display.setTextSize(1);
        display.setTextColor(SH110X_WHITE);
        display.setCursor(zBaseX, zBaseY);
        display.print("z");
        display.setCursor(zBaseX + 6, zBaseY - 5);
        display.print("Z");
        display.setCursor(zBaseX + 14, zBaseY - 12);
        display.print("Z");
        
        display.display();
    } else {
        // Normal RoboEyes update
        eyes.update();
    }
}

roboEyes* Face::getEyes() {
    return &eyes;
}

Adafruit_SH1106G* Face::getDisplay() {
    return &display;
}

void Face::showTestPattern() {
    display.clearDisplay();
    
    // Draw checkerboard pattern
    for (int y = 0; y < 64; y += 8) {
        for (int x = 0; x < 128; x += 8) {
            if ((x + y) % 16 == 0) {
                display.fillRect(x, y, 8, 8, SH110X_WHITE);
            }
        }
    }
    
    // Draw border
    display.drawRect(0, 0, 128, 64, SH110X_WHITE);
    
    // Draw crosshairs
    display.drawLine(64, 0, 64, 63, SH110X_WHITE);
    display.drawLine(0, 32, 127, 32, SH110X_WHITE);
    
    display.display();
}

void Face::clear() {
    display.clearDisplay();
    display.display();
}

void Face::showText(const char* text) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.println(text);
    display.display();
}

// ============================================================================
// Power State Faces
// ============================================================================

void Face::showSleepFace() {
    isSleeping = true;
    sleepAnimMs = millis();
    
    // Disable auto animations
    eyes.setAutoblinker(false, 3, 4);
    eyes.setIdleMode(false, 4, 5);
    
    // Set tired expression and close eyes
    eyes.tired = true;
    eyes.close();
    
    Serial.println("{\"status\":\"info\",\"msg\":\"Face: Sleep mode\"}");
}

void Face::showDimFace() {
    isSleeping = false;
    
    // Keep eyes open but tired/droopy
    eyes.tired = true;
    eyes.open();
    
    // Slow down animations
    eyes.setAutoblinker(true, 6, 3);  // Slower blinks
    eyes.setIdleMode(false, 4, 5);    // No idle movement
    
    Serial.println("{\"status\":\"info\",\"msg\":\"Face: Dim mode\"}");
}

void Face::showActiveFace() {
    isSleeping = false;
    
    // Normal happy eyes
    eyes.tired = false;
    eyes.open();
    
    // Re-enable normal animations
    eyes.setAutoblinker(EYE_AUTO_BLINK, BLINK_INTERVAL, BLINK_VARIATION);
    eyes.setIdleMode(EYE_IDLE_MODE, IDLE_INTERVAL, IDLE_VARIATION);
    
    Serial.println("{\"status\":\"info\",\"msg\":\"Face: Active mode\"}");
}
