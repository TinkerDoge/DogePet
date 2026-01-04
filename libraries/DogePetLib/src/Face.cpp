// Face.cpp - Display and eye animation management
#include "Face.h"
#include "Settings.h"

// Define static members
Adafruit_SH1106G Face::display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
roboEyes Face::eyes(Face::display);
DisplayMode Face::currentMode = DisplayMode::Eyes;
bool Face::eyesEnabled = true;
uint32_t Face::sleepAnimMs = 0;

void Face::init() {
    // Note: Wire.begin() should be called in setup() before this
    
    if (!display.begin(SCREEN_ADDR, true)) {
        Serial.println("{\"status\":\"error\",\"msg\":\"OLED not found\"}");
        return;
    }
    
    // Apply initial settings from Settings module
    display.setContrast(Settings::face.contrast);
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.display();
    
    // Initialize Eyes with Settings (loaded from NVS or defaults)
    eyes.begin(SCREEN_W, SCREEN_H, 50);
    applySettings();  // Apply face settings
    
    currentMode = DisplayMode::Eyes;
    eyesEnabled = true;
    
    Serial.println("{\"status\":\"info\",\"msg\":\"Face Init\"}");
}

// ============================================================================
// Apply Settings from Settings module (dynamic, no reboot needed)
// ============================================================================

void Face::applySettings() {
    eyes.setWidth(Settings::face.width, Settings::face.width);
    eyes.setHeight(Settings::face.height, Settings::face.height);
    eyes.setBorderradius(Settings::face.radius, Settings::face.radius);
    eyes.setSpacebetween(Settings::face.spacing);
    eyes.setAutoblinker(Settings::face.autoBlink, Settings::face.blinkInterval, BLINK_VARIATION);
    eyes.setIdleMode(Settings::face.idleMode, Settings::face.idleInterval, IDLE_VARIATION);
    display.setContrast(Settings::face.contrast);
    Serial.println("{\"status\":\"info\",\"msg\":\"Face settings applied\"}");
}

// ============================================================================
// Display Mode Control
// ============================================================================

void Face::setMode(DisplayMode mode) {
    currentMode = mode;
    
    // When switching modes, clear display first
    if (mode != DisplayMode::Eyes) {
        display.clearDisplay();
    }
}

DisplayMode Face::getMode() {
    return currentMode;
}

void Face::enableEyes(bool enable) {
    eyesEnabled = enable;
    if (enable) {
        currentMode = DisplayMode::Eyes;
    }
}

bool Face::areEyesEnabled() {
    return eyesEnabled && (currentMode == DisplayMode::Eyes);
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
// Main Update - respects current display mode
// ============================================================================

void Face::update() {
    switch (currentMode) {
        case DisplayMode::Eyes:
            if (eyesEnabled) {
                eyes.update();  // RoboEyes handles its own display
            }
            break;
            
        case DisplayMode::Sleep:
            drawSleepFace();
            break;
            
        case DisplayMode::Toast:
            // Toast mode - don't clear, let toast system handle it
            // Just skip eyes update
            break;
            
        case DisplayMode::Custom:
            // Custom mode - user draws directly, we do nothing
            break;
            
        case DisplayMode::Off:
            // Display off - do nothing
            break;
    }
}

void Face::drawSleepFace() {
    display.clearDisplay();
    
    // Draw closed eyes (horizontal lines) using Settings
    int eyeY = SCREEN_H / 2;
    int eyeW = Settings::face.width;
    int spacing = Settings::face.spacing;
    int leftX = (SCREEN_W / 2) - spacing/2 - eyeW;
    int rightX = (SCREEN_W / 2) + spacing/2;
    
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

void Face::testExpression(const char* name) {
    if (strcmp(name, "curious") == 0) {
        showActiveFace();
        // Trigger curious state in RoboEyes
        eyes.setCuriosity(true);
        // Reset after some time? For now, just set it.
    } else if (strcmp(name, "confused") == 0) {
        showActiveFace();
        eyes.anim_confused();
    } else if (strcmp(name, "laugh") == 0) {
        showActiveFace();
        eyes.anim_laugh();
    } else if (strcmp(name, "sleep") == 0) {
        showSleepFace();
    } else if (strcmp(name, "happy") == 0) {
        showActiveFace();
        eyes.setMood(HAPPY);
    } else if (strcmp(name, "toast") == 0) {
        setMode(DisplayMode::Toast);
        display.clearDisplay();
        display.setCursor(10, 25);
        display.println("Hello World!");
        display.display();
        // Note: No auto-timeout logic here yet, blocks eyes
    } else if (strcmp(name, "off") == 0) {
        setMode(DisplayMode::Off);
        display.clearDisplay();
        display.display();
    } else {
        // Default reset
        showActiveFace();
        eyes.setCuriosity(false);
        eyes.setMood(DEFAULT);
    }
}

void Face::showTestPattern() {
    display.clearDisplay();
    for (int y = 0; y < 64; y += 8) {
        for (int x = 0; x < 128; x += 8) {
            if ((x + y) % 16 == 0) display.fillRect(x, y, 8, 8, SH110X_WHITE);
        }
    }
    display.display();
}

void Face::clear() {
    display.clearDisplay();
    display.display();
}

void Face::showText(const char* text) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(text);
    display.display();
}

void Face::showSleepFace() {
    currentMode = DisplayMode::Sleep;
    sleepAnimMs = millis();
    eyesEnabled = false;
    eyes.setAutoblinker(false);
    eyes.setIdleMode(false);
    eyes.tired = true;
    eyes.close();
    Serial.println("{\"status\":\"info\",\"msg\":\"Face: Sleep mode\"}");
}

void Face::showDimFace() {
    currentMode = DisplayMode::Eyes;
    eyesEnabled = true;
    eyes.tired = true;
    eyes.open();
    eyes.setAutoblinker(true, 8, 4);
    eyes.setIdleMode(false);
    Serial.println("{\"status\":\"info\",\"msg\":\"Face: Dim mode\"}");
}

void Face::showActiveFace() {
    currentMode = DisplayMode::Eyes;
    eyesEnabled = true;
    eyes.tired = false;
    applySettings();  // Apply current face settings from Settings module
    eyes.open();
    Serial.println("{\"status\":\"info\",\"msg\":\"Face: Active mode\"}");
}

// Accessor methods
roboEyes* Face::getEyes() {
    return &eyes;
}

Adafruit_SH1106G* Face::getDisplay() {
    return &display;
}
