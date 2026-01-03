// DogePet - Robot Face with PC Companion App Support
// Communicates via USB Serial with PC companion app

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "include/config.h"

// External controls for RoboEyes library (Legacy, managed by Face module now)
bool gEyesAutoFlush = true;
int gEyesViewportYMax = 0;

// Modules
#include "include/Face.h"
#include "include/Motion.h"
#include "include/Audio.h"
#include "include/Animation.h"
#include "include/Power.h"
#include "include/Haptics.h"
#include "include/LED.h"
#include "include/Touch.h"
#include "include/serial_cmd.h"
#include "include/Events.h"

// Boot state machine
enum BootState { 
  BOOT_INIT, 
  BOOT_DISPLAY_TEST, 
  BOOT_IMU_TEST, 
  BOOT_AUDIO_TEST, 
  BOOT_HAPTIC_TEST, 
  BOOT_POWER_TEST, 
  BOOT_ANIMATION, 
  BOOT_DONE 
};
static BootState bootState = BOOT_INIT;

// =============================================================================
// SETUP - Pin configuration and peripheral initialization ONLY
// =============================================================================
void setup() {
  // Serial
  Serial.begin(115200);
  delay(500);

  // I2C Bus
  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  delay(100);
  
  // Initialize hardware modules (pin setup + peripheral init only)
  Power::init();
  Haptics::init();
  LED::init();
  Touch::init();
  Face::init();
  Motion::init();
  Audio::init();
  Animation::init();
  Events::init();
  
  // Serial command handler
  setupSerialCmd(Face::getEyes());
}

// =============================================================================
// BOOT SEQUENCE - Hardware tests & startup animation (runs once in loop)
// =============================================================================
void runBootSequence() {
  switch (bootState) {
    case BOOT_INIT:
      // I2C Bus Scan
      Serial.println("{\"status\":\"info\",\"msg\":\"I2C Scan...\"}");
      for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
          Serial.printf("{\"status\":\"info\",\"msg\":\"I2C device at 0x%02X\"}\n", addr);
        }
      }
      Face::showBootScreen("Booting...");
      delay(300);
      bootState = BOOT_DISPLAY_TEST;
      break;
      
    case BOOT_DISPLAY_TEST:
      Face::updateProgressBar(10, "Display Test");
      Face::playLineAnimation();
      bootState = BOOT_IMU_TEST;
      break;
      
    case BOOT_IMU_TEST:
      Face::updateProgressBar(25, "Calibrate IMU...");
      Motion::calibrate();
      delay(200);
      if (Motion::isReady()) {
        Face::updateProgressBar(35, "IMU OK");
        Serial.println("{\"status\":\"info\",\"msg\":\"Motion Sensor OK\"}");
      } else {
        Face::updateProgressBar(35, "IMU Fail");
        Serial.println("{\"status\":\"error\",\"msg\":\"Motion Sensor Fail\"}");
      }
      delay(300);
      bootState = BOOT_AUDIO_TEST;
      break;
      
    case BOOT_AUDIO_TEST:
      Face::updateProgressBar(50, "Mic Test");
      {
        int mic = Audio::readMicDB();
        Serial.printf("{\"status\":\"info\",\"msg\":\"Mic Level: %ddB\"}\n", mic);
        char msg[24];
        snprintf(msg, sizeof(msg), "Mic: %ddB", mic);
        Face::updateProgressBar(55, msg);
      }
      delay(300);
      Face::updateProgressBar(60, "Speaker Test");
      Audio::playTone(440, 150);
      delay(200);
      bootState = BOOT_HAPTIC_TEST;
      break;
      
    case BOOT_HAPTIC_TEST:
      Face::updateProgressBar(70, "Haptic Test");
      // Test basic click
      Haptics::click();
      delay(200);
      // Test purr briefly to verify it works
      Serial.println("{\"status\":\"info\",\"msg\":\"Testing purr...\"}");
      Haptics::startPurr();
      for (int i = 0; i < 30; i++) {  // Run purr for ~300ms
          Haptics::purrTick();
          delay(10);
      }
      Haptics::stopPurr();
      delay(100);
      bootState = BOOT_POWER_TEST;
      break;
      
    case BOOT_POWER_TEST:
      {
        int batt = Power::getPercent();
        char msg[20];
        snprintf(msg, sizeof(msg), "Batt: %d%%", batt);
        Face::updateProgressBar(80, msg);
        Serial.printf("{\"status\":\"info\",\"msg\":\"Batt: %d%%\"}\n", batt);
      }
      delay(300);
      bootState = BOOT_ANIMATION;
      break;
      
    case BOOT_ANIMATION:
      Face::updateProgressBar(90, "Wake Up...");
      Animation::playStartupSequence();
      Face::updateProgressBar(100, "Ready!");
      delay(400);
      
      // Enable eye behaviors
      {
        roboEyes* eyes = Face::getEyes();
        eyes->setAutoblinker(true, 3, 4);
        eyes->setIdleMode(true, 4, 5);
      }
      
      Serial.println("{\"status\":\"info\",\"msg\":\"Boot Complete\"}");
      bootState = BOOT_DONE;
      break;
      
    case BOOT_DONE:
      break;
  }
}

// =============================================================================
// LOOP
// =============================================================================
void loop() {
  // Run boot sequence once (tests run here, safe for all GPIOs)
  if (bootState != BOOT_DONE) {
    runBootSequence();
    return;  // Don't run normal loop until boot is done
  }
  
  // Normal operation
  processSerialCmd();
  
  // Update touch sensors
  Touch::update();
  
  // Handle touch events
  Events::update();
  
  // Update haptics purr (non-blocking)
  Haptics::purrTick();
  
  // Update Motion Sensor (logs data on change)
  Motion::update();
  
  // Update face
  Face::update();
  Animation::tick();
}

// =============================================================================
// TOUCH EVENT HANDLING
// =============================================================================
// Moved to Events.cpp

