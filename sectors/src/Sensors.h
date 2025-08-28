// Sensors sector: touch/button hooks and MPU helpers
#pragma once
#include <Arduino.h>

namespace Sectors { namespace Sensors {
  void begin();
  void update();
  // Future: callbacks for touch/button
}}
