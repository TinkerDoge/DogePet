// Brain sector: high-level AI (Gemini) wrapper
#pragma once

#include <Arduino.h>
#include <functional>

namespace Brain {
  void begin();
  void update();
  void sendUserMessage(const String& msg);
  String getLatestResponse();

  // Optional: provide a function hook to perform the low-level send.
  using SendMsgFn = std::function<bool(const char* message, char* response, size_t maxLen)>;
  void setSendMessageFn(SendMsgFn fn);
  using GetLatestFn = std::function<String()>;
  void setGetLatestResponseFn(GetLatestFn fn);

  // simple event callbacks
  using CmdCb = std::function<void(const String&)>;
  void onAnimationCommand(CmdCb cb);
  void onSoundFxCommand(CmdCb cb);
  void onThought(CmdCb cb);
}
