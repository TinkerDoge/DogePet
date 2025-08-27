// Animations sector implementation - thin wrapper

#include "Animations.h"

namespace Animations {

static PlayAnimFn playAnimFn = nullptr;
static PlaySfxFn playSfxFn = nullptr;

void begin() {
}

void update() {
}

void playAnimation(const String& name) {
  if (playAnimFn) playAnimFn(name);
}

void playSfx(const String& name) {
  if (playSfxFn) playSfxFn(name);
}

void setPlayAnimFn(PlayAnimFn fn) { playAnimFn = fn; }
void setPlaySfxFn(PlaySfxFn fn) { playSfxFn = fn; }

} // namespace Animations
