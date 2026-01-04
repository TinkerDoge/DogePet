// DogePet - Robot Face with PC Companion App Support
// Communicates via USB Serial with PC companion app

#include <Wire.h>
#include <ArduinoJson.h>
#include <DogePetLib.h>

// External controls for RoboEyes library
bool gEyesAutoFlush = true;
int gEyesViewportYMax = 0;

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
  // Increase RX buffer for large JSON payloads
  Serial.setRxBufferSize(1024);
  Serial.begin(115200);
  delay(500);

  // Load runtime settings from NVS (must be before Wire.begin)
  Settings::begin();

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
  
  // Set up power state callbacks
  Power::onSleepCallback = []() {
    Face::showSleepFace();  // Turn display completely off
    Haptics::stop();  // Stop all haptics
    Haptics::stopPurr();  // Ensure purr is stopped
    LED::off();  // Turn off status LED
  };
  
  Power::onWakeCallback = []() {
    Face::showActiveFace();  // Return to normal eyes
    // LED will be controlled by other modules
  };
  
  Power::onDimCallback = []() {
    Face::showDimFace();  // Show closed eyes with animated zZZ
    LED::setBrightness(DIM_BRIGHTNESS);  // Dim LED
  };
  
  // Serial command handler
  // setupSerialCmd(Face::getEyes());
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
      //Face::playLineAnimation();
      bootState = BOOT_IMU_TEST;
      break;
      
    case BOOT_IMU_TEST:
      Face::updateProgressBar(25, "Check IMU...");
      // Note: Calibration already happens in Motion::init()
      // Just check if DMP is ready
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
  
  // Check if sleeping - skip most operations to save power
  if (Power::isSleeping()) {
    // Minimal operations when sleeping
    Power::update();  // Still update power (with reduced frequency)
    
    // Check for wake conditions (touch, motion, etc.)
    Touch::update();
    if (Touch::getHeadEvent() != TouchEvent::NONE || Touch::isHeadPressed()) {
      Power::onActivity();  // Wake on touch
    }
    
    // Check for motion wake (minimal polling - every 500ms)
    static uint32_t lastMotionCheckMs = 0;
    if (millis() - lastMotionCheckMs >= 500) {
      Motion::Event motionEvent = Motion::update();
      if (motionEvent != Motion::Event::None && motionEvent != Motion::Event::Still) {
        Power::onMotion();  // Wake on motion
      }
      lastMotionCheckMs = millis();
    }
    
    // Small delay to reduce CPU usage when sleeping
    delay(SLEEP_LOOP_DELAY_MS);
    return;  // Skip all other updates (face animations, haptics, etc.)
  }
  
  // Normal operation (ACTIVE or DIM mode)
  // Serial Command Handling
  processSerialCmd();
  
  // Update Modules
  Motion::update();
  Power::update();
  
  // Update touch sensors FIRST (must be before Events::update)
  Touch::update();
  
  // Handle touch events (reads events generated by Touch::update)
  Events::update();
  
  // Update haptics purr (non-blocking)
  Haptics::purrTick();
  
  // Update face
  Face::update();
  Animation::tick();
}

// =============================================================================
// SERIAL COMMAND HANDLING
// =============================================================================
void processSerialCmd() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;
    
    // Simple command parsing
    if (line.indexOf("get_config") >= 0) {
      Serial.print("{\"type\":\"config\",\"data\":");
      Serial.print(Settings::toJson());
      Serial.println("}");
    }
    else if (line.indexOf("set_config") >= 0) {
      if (!Settings::fromJson(line)) {
        // Error already printed by fromJson
      }
    }
    else if (line.indexOf("test_face") >= 0) {
        // format: {"cmd":"test_face","val":"curious"}
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, line);
        if (!error && doc.containsKey("val")) {
            const char* val = doc["val"];
            Face::testExpression(val);
            Serial.println("{\"status\":\"ok\",\"msg\":\"Expression applied\"}");
        } else {
             Serial.println("{\"status\":\"error\",\"msg\":\"Invalid JSON or missing 'val'\"}");
        }
    }
    else if (line.indexOf("reboot") >= 0) {
      Serial.println("{\"status\":\"info\",\"msg\":\"Rebooting...\"}");
      delay(500);
      ESP.restart();
    }
    else if (line.indexOf("factory_reset") >= 0) {
      Serial.println("{\"status\":\"info\",\"msg\":\"Resetting to defaults...\"}");
      Settings::resetDefaults();
      delay(500);
      ESP.restart();
    }
  }
}

