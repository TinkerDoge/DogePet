// Sensors sector: handles low-level sensors and inputs
#pragma once

#include <Arduino.h>
#include <functional>

namespace Sensors {
  void begin();
  void update();

  // callbacks
  using TouchCb = std::function<void()>;
  void onTouch(TouchCb cb);
}
