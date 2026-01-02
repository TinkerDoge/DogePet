// DogePet - Minimal Barebones Sketch
// Just displays eyes on OLED - ready for web configuration integration

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

// External controls for RoboEyes library
bool gEyesAutoFlush = true;
int gEyesViewportYMax = 0;

#include "FluxGarage_RoboEyes.h"
#include "include/web_config.h"

// =============================================================================
// HARDWARE OBJECTS
// =============================================================================
Adafruit_SH1106G display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
roboEyes Eyes;

// =============================================================================
// TIMING
// =============================================================================
static uint32_t lastUpdateMs = 0;

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== DogePet Barebones ===");

  // Initialize I2C
  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  Serial.printf("I2C: SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);

  // Initialize display
  if (!display.begin(SCREEN_ADDR, true)) {
    Serial.println("ERROR: OLED not found!");
    while (1) delay(100);
  }
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("DogePet");
  display.display();
  Serial.println("OLED: OK");

  // Initialize RoboEyes
  Eyes.begin(SCREEN_W, SCREEN_H, 50);
  Eyes.setWidth(EYE_WIDTH, EYE_WIDTH);
  Eyes.setHeight(EYE_HEIGHT, EYE_HEIGHT);
  Eyes.setBorderradius(EYE_BORDER_RADIUS, EYE_BORDER_RADIUS);
  Eyes.setSpacebetween(EYE_SPACING);
  Eyes.setAutoblinker(true, 3, 4);
  Eyes.setIdleMode(true, 4, 5);
  Serial.println("Eyes: OK");

  // Button setup
  pinMode(FUNC_BTN, INPUT);
  Serial.printf("Button: GPIO%d (Active HIGH)\n", FUNC_BTN);

  // Initialize Web Configuration
  setupWebConfig(&Eyes);

  delay(500);
  display.clearDisplay();
  Serial.println("Setup complete!\n");
}

// =============================================================================
// LOOP
// =============================================================================
void loop() {
  uint32_t now = millis();

  // Handle web server requests
  loopWebConfig();

  // Throttled display update
  if (now - lastUpdateMs >= DISPLAY_UPDATE_MS) {
    Eyes.update();
    lastUpdateMs = now;
  }

  // Simple button check (for testing)
  static bool lastBtn = false;
  bool btn = digitalRead(FUNC_BTN) == HIGH;
  if (btn && !lastBtn) {
    Serial.println("Button pressed!");
    Eyes.blink();
  }
  lastBtn = btn;
}

