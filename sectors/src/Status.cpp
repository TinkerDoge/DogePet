#include "Status.h"

namespace Sectors { namespace Status {
  static State cur = Idle;

  void begin() { cur = Idle; }
  void update() { /* placeholder for timers and transitions */ }

  void set(State s) { cur = s; }
  State get() { return cur; }

  float imuShakeThreshold() { return (cur >= Angry) ? 0.7f : 1.0f; }
  uint32_t chatterCooldownMs() { return (cur == Chatting) ? 60000u : 45000u; }
}}
