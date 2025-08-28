// Voice sector: audio generation and speech synthesis
#pragma once
#include <Arduino.h>
#include <stdint.h>

namespace Sectors { namespace Voice {
  // Audio system initialization
  void begin(int bclkPin, int lrclkPin, int dataOutPin, uint32_t sampleRate = 22050);
  void end();
  void update(); // Call regularly from loop() to fill I2S with mixed audio
  
  // Master volume control
  void setMasterVolume(uint8_t vol);
  uint8_t getMasterVolume();
  
  // Basic audio generation
  void playBeep(float freqHz, uint16_t ms, uint8_t volume = 180);
  void stopAll();
  
  // Microphone input support
  bool beginMicrophone(uint32_t sampleRate = 16000);
  void endMicrophone();
  float getMicrophoneLevel();
  bool isLoudNoiseDetected();
  bool isAudioPlaying();
  bool isMicrophoneInCooldown();
  
  // Expressive sound effects
  void sfxAngry();
  void sfxFurious();
  void sfxCurious();
  void sfxBlink();
  void sfxNotify();
  void sfxConfirm();
  void sfxError();
  void sfxDroidYes();
  void sfxDroidNo();
  
  // Cute robot sounds
  void playCuteYes(uint16_t duration = 120);
  void playCuteNo(uint16_t duration = 200);
  void playCuteQuestion();
  void playCuteQuestion(uint16_t duration);
  void playCuteFurious();
  void playCuteStartup();
  void playCuteSleep(uint16_t duration = 0);
  void playCuteChatterFree(float minFreq = 450, float maxFreq = 1300, uint16_t duration = 0);
  
  // Audio sequencer
  bool isSequencePlaying();
  void sequencerUpdate();
}}
