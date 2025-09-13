// Vitals sector implementation
#include "Vitals.h"
#include "clock.h"

namespace Vitals {

void begin() {
  // nothing to init
}

void update() {
  // could cache readings periodically
}

float getVbatVolts() {
  return readVBATVolts();
}

int getVbatPercent() {
  return voltsToPercent(getVbatVolts());
}

} // namespace Vitals
