// UI sector: thin wrappers for toasts and drawing helpers
#pragma once
#include <Arduino.h>
class Adafruit_SH1106G; // fwd decl to avoid heavy include here

namespace Sectors { namespace UI {
  void showToast(const String& text, uint16_t ms);
  void drawToastIfAny(Adafruit_SH1106G& display);
}}
