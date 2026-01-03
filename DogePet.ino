// DogePet - Robot Face with PC Companion App Support
// Communicates via USB Serial with PC companion app

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "config.h"

// External controls for RoboEyes library
bool gEyesAutoFlush = true;
int gEyesViewportYMax = 0;

#include "FluxGarage_RoboEyes.h"
#include "include/serial_cmd.h"

// =============================================================================
// HARDWARE OBJECTS
// =============================================================================
Adafruit_SH1106G display(SCREEN_W, SCREEN_H, &Wire, OLED_RESET);
roboEyes Eyes(display);

// =============================================================================
// TIMING
// =============================================================================
static uint32_t lastUpdateMs = 0;

// =============================================================================
// SETUP
// =============================================================================
void setup() {
  Serial.begin(115200);
  
  // Wait a moment for Serial to initialize
  delay(100);

  // Load Hardware Config (Pins)
  loadHardwareConfig();
  HardwareConfig* config = getHardwareConfig();

  // Initialize I2C
  Wire.begin(config->i2c_sda, config->i2c_scl, 400000);

  // Initialize display
  if (!display.begin(SCREEN_ADDR, true)) {
    Serial.println("{\"status\":\"error\",\"msg\":\"OLED not found\"}");
    while (1) delay(100);
  }
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("DogePet");
  display.println("Waiting for");
  display.println("companion...");
  display.display();

  // Initialize RoboEyes
  Eyes.begin(SCREEN_W, SCREEN_H, 50);
  Eyes.setWidth(EYE_WIDTH, EYE_WIDTH);
  Eyes.setHeight(EYE_HEIGHT, EYE_HEIGHT);
  Eyes.setBorderradius(EYE_BORDER_RADIUS, EYE_BORDER_RADIUS);
  Eyes.setSpacebetween(EYE_SPACING);
  Eyes.setAutoblinker(true, 3, 4);
  Eyes.setIdleMode(true, 4, 5);

  // Button setup
  pinMode(config->func_btn, INPUT);

  // Initialize Serial command handler
  setupSerialCmd(&Eyes);

  delay(500);
  display.clearDisplay();
}

// =============================================================================
// LOOP
// =============================================================================
void loop() {
  uint32_t now = millis();

  // Process incoming Serial commands from PC companion app
  processSerialCmd();

  // Throttled display update
  if (now - lastUpdateMs >= DISPLAY_UPDATE_MS) {
    Eyes.update();
    lastUpdateMs = now;
  }

  // Simple button check (for testing)
  HardwareConfig* config = getHardwareConfig();
  static bool lastBtn = false;
  bool btn = digitalRead(config->func_btn) == HIGH;
  if (btn && !lastBtn) {
    Eyes.blink();
    Serial.println("{\"event\":\"button\",\"action\":\"pressed\"}");
  }
  lastBtn = btn;
}
