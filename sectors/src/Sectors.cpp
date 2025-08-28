#include "Sectors.h"
#include "Brain.h"
#include "MotionSector.h"
#include "UI.h"
#include "Sensors.h"
#include "Status.h"
#include "Vitals.h"
#include "Voice.h"
#include "Display.h"
#include "Portal.h"
// Use project configuration constants for pins and screen size
#include "../../config.h"

extern Adafruit_SH1106G* oled_display; // Global display reference

namespace Sectors {
  void beginAll(const char* geminiApiKey) {
    Status::begin();
    Sensors::begin();
    MotionSector::begin();
    Brain::begin(geminiApiKey);
    Vitals::begin();
  Portal::begin();
    
  // Initialize audio with I2S pins from config.h
  Voice::begin(I2S_BCLK, I2S_LRC, I2S_DO, AUDIO_SAMPLE_RATE);
    
    // Initialize Display/RoboEyes using screen constants
    if (oled_display) {
      Display::begin(SCREEN_W, SCREEN_H, 50); // eye distance default = 50
    }
    
    // UI has no global begin for now; uses existing sketch/display init
  }

  void updateAll() {
    Status::update();
    Sensors::update();
    MotionSector::update();
    Brain::update();
    Vitals::update();
    Voice::update();
    Display::update(); // Drive RoboEyes animations
  Portal::update();  // Serve config portal if active
    // UI::update(...) is driven by the sketch during rendering
  }
}
