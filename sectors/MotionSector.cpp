// Motion sector implementation - uses existing motion module
#include "MotionSector.h"

namespace MotionSector {

static TiltCb tiltCb = nullptr;
static ShakeCb shakeCb = nullptr;

static BeginFn beginFn = nullptr;
static UpdateFn updateFn = nullptr;

void begin() {
  if (beginFn) beginFn();
}

void update() {
  if (updateFn) updateFn();
}

void setBeginFn(BeginFn fn) { beginFn = fn; }
void setUpdateFn(UpdateFn fn) { updateFn = fn; }

void onTilt(TiltCb cb) { tiltCb = cb; }
void onShake(ShakeCb cb) { shakeCb = cb; }

} // namespace MotionSector
