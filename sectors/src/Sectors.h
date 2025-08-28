// Aggregator for modular sectors
#pragma once
#include <Arduino.h>
#include "Display.h"
#include "Portal.h"

namespace Sectors {
  // Initialize all sectors (pass API key if Brain should enable AI)
  void beginAll(const char* geminiApiKey = nullptr);
  // Periodic update; call from loop()
  void updateAll();
}
