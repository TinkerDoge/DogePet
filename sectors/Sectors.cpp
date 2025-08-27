// Sectors registry implementation
#include "Sectors.h"
#include "Brain.h"
#include "MotionSector.h"
#include "Animations.h"
#include "UI.h"
#include "Sensors.h"
#include "Vitals.h"

namespace Sectors {

void beginAll() {
  Sensors::begin();
  Vitals::begin();
  Animations::begin();
  MotionSector::begin();
  Brain::begin();
  UI::begin();
}

void updateAll() {
  Sensors::update();
  MotionSector::update();
  Brain::update();
  UI::update();
  Vitals::update();
}

} // namespace Sectors
