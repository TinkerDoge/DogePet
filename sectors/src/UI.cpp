#include "UI.h"

// Pull minimal declarations from legacy modules
class Adafruit_SH1106G;
void drawToastIfAny(Adafruit_SH1106G& display);
void showToast(const String& s, uint16_t ms);

namespace Sectors { namespace UI {
  void showToast(const String& text, uint16_t ms) { ::showToast(text, ms); }
  void drawToastIfAny(Adafruit_SH1106G& display) { ::drawToastIfAny(display); }
}}
