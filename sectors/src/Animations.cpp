#include "Animations.h"

// Forward declarations from legacy modules
enum MoodState : uint8_t; // match sketch
void setEyesMood(MoodState m);
void Eyes_Blink();

// Stub implementations for removed AnimationEngine functions
namespace AnimationEngine {
  void lookDirection(String direction) {
    // Placeholder - could be implemented with Eyes.setPosition() calls
    Serial.printf("Animation: look %s\n", direction.c_str());
  }
  void adjustEyeSize(String adjustment) {
    // Placeholder - could be implemented with Eyes.setWidth/setHeight calls  
    Serial.printf("Animation: eye size %s\n", adjustment.c_str());
  }
  void executeSoundFX(String soundFx) {
    // Placeholder - could trigger Audio:: calls
    Serial.printf("Animation: SFX %s\n", soundFx.c_str());
  }
}

// Track current mood in this sector
extern MoodState curMood; // Reference to global variable in main sketch

namespace Sectors { namespace Animations {
  void setMood(MoodState mood) { 
    ::setEyesMood(mood); 
    curMood = mood; // Update global mood tracker
  }
  MoodState getCurrentMood() { return curMood; }
  void blink() { ::Eyes_Blink(); }
  void look(const String& direction) { AnimationEngine::lookDirection(direction); }
  void eyeSize(const String& adj) { AnimationEngine::adjustEyeSize(adj); }
  void playFx(const String& sfx) { AnimationEngine::executeSoundFX(sfx); }
}}
