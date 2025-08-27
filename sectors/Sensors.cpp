// Sensors sector implementation
#include "Sensors.h"
#include "mpu6050.h"

namespace Sensors {

static TouchCb touchCb = nullptr;

void begin() {
  // initialize MPU6050
  mpuInit();
}

void update() {
  // placeholder for polling touch or other sensors
}

void onTouch(TouchCb cb) { touchCb = cb; }

} // namespace Sensors
