// Vitals sector: battery, power and connectivity monitoring
#pragma once

#include <Arduino.h>

namespace Vitals {
  void begin();
  void update();
  float getVbatVolts();
  int getVbatPercent();
}
