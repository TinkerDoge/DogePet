// Adapters implementation - isolate legacy includes here and wire them into sectors
#include "Adapters.h"

// Legacy headers
#include "ai_companion.h"
#include "animation_engine.h"
#include "mpu6050.h"
#include "DogePet.ino" // for showToast
#include "motion.h"
#include "clock.h"

#include "Brain.h"
#include "Animations.h"
#include "MotionSector.h"
#include "Sensors.h"
#include "UI.h"
#include "Vitals.h"

void wireLegacyAdapters() {
  // Brain adapter: use AICompanion send/get
  Brain::setSendMessageFn([](const char* msg, char* resp, size_t maxLen)->bool {
    bool ok = AICompanion::sendMessage(msg, resp, maxLen);
    return ok;
  });
  Brain::setGetLatestResponseFn([]()->String {
    return String(AICompanion::aiResponse);
  });

  // Animations adapter
  Animations::setPlayAnimFn([](const String& s){ AnimationEngine::executeEyeAnimation(s); });
  Animations::setPlaySfxFn([](const String& s){ AnimationEngine::executeSoundFX(s); });

  // Motion adapters: call existing functions
  MotionSector::setBeginFn([](){ /* no-op: motion uses global init */ });
  MotionSector::setUpdateFn([](){ updateEmotionsFromIMU(); });

  // Sensors: wire MPU init
  Sensors::onTouch(nullptr); // leave touch unbound

  // UI: wire showToast to sketch's global
  UI::setShowToastFn([](const String& s, uint16_t ms){ ::showToast(s, ms); });

  // Vitals: nothing to wire; its implementation uses clock helpers already
}
