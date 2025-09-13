// Brain sector implementation - thin wrapper for AICompanion

#include "Brain.h"

namespace Brain {

static CmdCb animCb = nullptr;
static CmdCb sfxCb = nullptr;
static CmdCb thoughtCb = nullptr;

static SendMsgFn sendFn = nullptr;
static GetLatestFn getLatestFn = nullptr;

void begin() {
  // sector is standalone; user wires implementation via setSendMessageFn()
}

void update() {
  // if user provided a getter, call it and emit thought
  if (getLatestFn) {
    String s = getLatestFn();
    if (s.length() && thoughtCb) thoughtCb(s);
  }
}

void sendUserMessage(const String& msg) {
  if (!sendFn) return;
  char buf[1024] = {0};
  if (sendFn(msg.c_str(), buf, sizeof(buf))) {
    if (thoughtCb) thoughtCb(String(buf));
  }
}

String getLatestResponse() {
  if (getLatestFn) return getLatestFn();
  return String();
}

void setSendMessageFn(SendMsgFn fn) { sendFn = fn; }
void setGetLatestResponseFn(GetLatestFn fn) { getLatestFn = fn; }

void onAnimationCommand(CmdCb cb) { animCb = cb; }
void onSoundFxCommand(CmdCb cb) { sfxCb = cb; }
void onThought(CmdCb cb) { thoughtCb = cb; }

} // namespace Brain
