#include "Voice.h"

// Forward declarations to existing Audio namespace functions
namespace Audio {
  void begin(int bclkPin, int lrclkPin, int dataOutPin, uint32_t sampleRate);
  void end();
  void update();
  void setMasterVolume(uint8_t vol);
  uint8_t getMasterVolume();
  void playBeep(float freqHz, uint16_t ms, uint8_t volume, uint8_t wave);
  void stopAll();
  
  bool beginMicrophone(uint32_t sampleRate);
  void endMicrophone();
  float getMicrophoneLevel();
  bool isLoudNoiseDetected();
  bool isAudioPlaying();
  bool isMicrophoneInCooldown();
  
  void sfxAngry();
  void sfxFurious();
  void sfxCurious();
  void sfxBlink();
  void sfxNotify();
  void sfxConfirm();
  void sfxError();
  void sfxDroidYes();
  void sfxDroidNo();
  
  void playCuteYes(uint16_t duration);
  void playCuteNo(uint16_t duration);
  void playCuteQuestion();
  void playCuteFurious();
  void playCuteChatterFree(float minFreq, float maxFreq, uint16_t duration);
}

namespace Sectors { namespace Voice {
  void begin(int bclkPin, int lrclkPin, int dataOutPin, uint32_t sampleRate) {
    Audio::begin(bclkPin, lrclkPin, dataOutPin, sampleRate);
  }
  
  void end() { Audio::end(); }
  void update() { Audio::update(); }
  
  void setMasterVolume(uint8_t vol) { Audio::setMasterVolume(vol); }
  uint8_t getMasterVolume() { return Audio::getMasterVolume(); }
  
  void playBeep(float freqHz, uint16_t ms, uint8_t volume) {
    Audio::playBeep(freqHz, ms, volume, 0); // WAVE_SINE = 0
  }
  void stopAll() { Audio::stopAll(); }
  
  bool beginMicrophone(uint32_t sampleRate) { return Audio::beginMicrophone(sampleRate); }
  void endMicrophone() { Audio::endMicrophone(); }
  float getMicrophoneLevel() { return Audio::getMicrophoneLevel(); }
  bool isLoudNoiseDetected() { return Audio::isLoudNoiseDetected(); }
  bool isAudioPlaying() { return Audio::isAudioPlaying(); }
  bool isMicrophoneInCooldown() { return Audio::isMicrophoneInCooldown(); }
  
  void sfxAngry() { Audio::sfxAngry(); }
  void sfxFurious() { Audio::sfxFurious(); }
  void sfxCurious() { Audio::sfxCurious(); }
  void sfxBlink() { Audio::sfxBlink(); }
  void sfxNotify() { Audio::sfxNotify(); }
  void sfxConfirm() { Audio::sfxConfirm(); }
  void sfxError() { Audio::sfxError(); }
  void sfxDroidYes() { Audio::sfxDroidYes(); }
  void sfxDroidNo() { Audio::sfxDroidNo(); }
  
  void playCuteYes(uint16_t duration) { Audio::playCuteYes(duration); }
  void playCuteNo(uint16_t duration) { Audio::playCuteNo(duration); }
  void playCuteQuestion() { Audio::playCuteQuestion(); }
  void playCuteQuestion(uint16_t duration) { Audio::playCuteQuestion(duration); }
  void playCuteFurious() { Audio::playCuteFurious(); }
  void playCuteStartup() { Audio::playCuteStartup(); }
  void playCuteSleep(uint16_t duration) { Audio::playCuteSleep(duration); }
  void playCuteChatterFree(float minFreq, float maxFreq, uint16_t duration) {
    Audio::playCuteChatterFree(minFreq, maxFreq, duration);
  }
  
  bool isSequencePlaying() { return Audio::isSequencePlaying(); }
  void sequencerUpdate() { Audio::sequencerUpdate(); }
}}
