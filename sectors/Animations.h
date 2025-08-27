// Animations sector: wrapper around AnimationEngine and eyes
#pragma once


#include <Arduino.h>
#include <functional>

namespace Animations {
  void begin();
  void update();
  void playAnimation(const String& name);
  void playSfx(const String& name);

  // Hooks so this sector can be used without linking legacy code.
  using PlayAnimFn = std::function<void(const String&)>;
  using PlaySfxFn = std::function<void(const String&)>;
  void setPlayAnimFn(PlayAnimFn fn);
  void setPlaySfxFn(PlaySfxFn fn);
}
