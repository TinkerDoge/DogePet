// Motion sector: encapsulates MPU + motion interpretation
#pragma once


#include <Arduino.h>
#include <functional>

namespace MotionSector {
  void begin();
  void update();

  using TiltCb = std::function<void(float)>;
  using ShakeCb = std::function<void()>;

  // Hooks so this sector can be used standalone. Provide an external
  // implementation (adapter) via set*Fn().
  using BeginFn = std::function<void()>;
  using UpdateFn = std::function<void()>;
  void setBeginFn(BeginFn fn);
  void setUpdateFn(UpdateFn fn);

  void onTilt(TiltCb cb);
  void onShake(ShakeCb cb);
}
