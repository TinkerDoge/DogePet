// UI sector implementation - thin wrapper around sketch UI helpers

#include "UI.h"

namespace UI {

static ShowToastFn showToastFn = nullptr;

void begin() {}

void update() {}

void showToast(const String& s, uint16_t ms) {
  if (showToastFn) showToastFn(s, ms);
}

void setShowToastFn(ShowToastFn fn) { showToastFn = fn; }

} // namespace UI
