#include "Brain.h"

// Forward declarations to avoid including sketch headers
namespace AICompanion {
  void initialize();
  bool begin(const char* apiKey);
  void handleBackgroundChatter();
  void handleMessage(const char* message);
}

namespace Sectors { namespace Brain {
  static bool inited = false;

  void begin(const char* geminiApiKey) {
    if (inited) return;
    AICompanion::initialize();
    if (geminiApiKey && geminiApiKey[0]) {
      AICompanion::begin(geminiApiKey);
    }
    inited = true;
  }

  void update() {
    if (!inited) return;
    // Let the AI module decide when to chatter based on its cooldown
    AICompanion::handleBackgroundChatter();
  }

  bool sendUserMessage(const char* msg) {
    if (!inited || !msg || !msg[0]) return false;
    AICompanion::handleMessage(msg);
    return true;
  }
}}
