#include "Vitals.h"

// Forward to legacy helpers (declared in clock.h and notification.h in sketch)
float readVBATVolts();
int voltsToPercent(float v);
class Adafruit_SH1106G; class ChronosESP32;
void drawClock(Adafruit_SH1106G& display, ChronosESP32& chrono);
void drawNotif(Adafruit_SH1106G& display);
bool hasNotif();

namespace Sectors { namespace Vitals {
  float readVBATVolts() { return ::readVBATVolts(); }
  int voltsToPercent(float v) { return ::voltsToPercent(v); }
  
  void drawClock(Adafruit_SH1106G& display, ChronosESP32& chrono) { 
    ::drawClock(display, chrono); 
  }
  
  void drawNotification(Adafruit_SH1106G& display) { 
    ::drawNotif(display); 
  }
  
  bool hasNotification() { 
    return ::hasNotif(); 
  }
}}
