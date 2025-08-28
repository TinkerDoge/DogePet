// Status sector: centralized bot status and tunable parameters
#pragma once
#include <Arduino.h>

namespace Sectors { namespace Status {
  enum State : uint8_t { Idle, Curious, Happy, Angry, Furious, Tired, Notify, Chatting };

  void begin();
  void update();

  // Get/Set current state
  void set(State s);
  State get();

  // Example tunables that other sectors can query later
  float imuShakeThreshold();
  uint32_t chatterCooldownMs();
}}
