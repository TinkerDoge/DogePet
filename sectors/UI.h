// UI sector: display composition and toast APIs
#pragma once


#include <Arduino.h>
#include <functional>

namespace UI {
  void begin();
  void update();
  void showToast(const String& s, uint16_t ms=1200);

  using ShowToastFn = std::function<void(const String&, uint16_t)>;
  void setShowToastFn(ShowToastFn fn);
}
