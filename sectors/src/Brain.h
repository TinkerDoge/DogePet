// Brain sector: thin wrapper over AI module
#pragma once
#include <Arduino.h>

namespace Sectors { namespace Brain {
  // Initialize AI brain (pass nullptr to keep AI disabled)
  void begin(const char* geminiApiKey);
  // Drive background chatter / maintenance
  void update();
  // Send a user message to AI
  bool sendUserMessage(const char* msg);
}}
