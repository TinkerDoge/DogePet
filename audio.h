// I2S Audio Engine (ESP32-S3)
// Simple software mixer with a few voices to generate layered "droid"-style bleeps
// Usage:
//   #include "audio.h"
//   Audio::begin(I2S_BCLK, I2S_LRC, I2S_DO, 22050);
//   In loop(): Audio::update();
//   Trigger sounds: Audio::playBeep(1200, 120, 200); or layered Audio::playBleep(...)

#pragma once
#include <Arduino.h>
#include <stdint.h>  // Add standard integer types
#include "config.h"  // Include configuration constants

namespace Audio {

  enum Waveform : uint8_t { WAVE_SINE=0, WAVE_SQUARE=1, WAVE_NOISE=2 };

  struct SfxParams {
    float baseFreqHz = 1000.0f;   // start frequency
    float glideFreqHz = 800.0f;   // end frequency (linear glide)
    uint16_t durationMs = 120;    // total duration
    uint8_t wave = WAVE_SINE;     // waveform
    uint8_t volume = 180;         // 0..255
  };

  // Initialize I2S and internal mixer
  void begin(int bclkPin, int lrclkPin, int dataOutPin, uint32_t sampleRate=22050);
  void end();

  // Call regularly from loop() to fill I2S with mixed audio
  void update();

  // Master volume 0..255 (default ~200)
  void setMasterVolume(uint8_t vol);
  uint8_t getMasterVolume();

  // Simple one-voice beep
  void playBeep(float freqHz, uint16_t ms, uint8_t volume=180, uint8_t wave=WAVE_SINE);

  // Layered bleep (2 short overlapping voices for "droid" feel)
  void playBleep(const SfxParams& a, const SfxParams& b);

  // === Microphone Input Support ===
  bool beginMicrophone(uint32_t sampleRate = MIC_SAMPLE_RATE);
  void endMicrophone();
  float getMicrophoneLevel();  // Returns current RMS level
  bool isLoudNoiseDetected();  // Returns true if noise above threshold

  // Audio feedback prevention
  bool isAudioPlaying();       // Returns true if any audio is currently playing
  bool isMicrophoneInCooldown(); // Returns true if mic is in cooldown after audio

  // Stop all voices immediately
  void stopAll();

  // Expressive presets (layered, with variation)
  void sfxAngry();        // buzzy square bursts
  void sfxFurious();      // alternating alarm
  void sfxCurious();      // short inquisitive bleep
  void sfxBlink();        // tiny tick
  void sfxNotify();       // message arrival
  void sfxConfirm();      // UI confirm up
  void sfxError();        // UI error down

  // Droid-style extended presets
  void sfxDroidYes();
  void sfxDroidNo();
  // removed unused: sfxDroidChirp/Question/Exclaim/Thinking/Chatter

  // removed unused: droidSpeak

  // ===== Non-blocking cute sequencer =====
  struct NoteEv {
    uint8_t midi;     // 0 = rest, else MIDI note number (A4=69)
    uint16_t ms;      // duration
    uint8_t vol;      // 0..255
    uint8_t wave;     // WAVE_SINE / WAVE_SQUARE / WAVE_NOISE
  };

  void startSequence(const NoteEv* events, uint8_t len, uint16_t startDelayMs=0);
  bool isSequencePlaying();
  void sequencerUpdate();

  // Convenience wrappers for built-in cute lines
  void playCuteHello(uint16_t startDelayMs=0);
  void playCuteQuestion(uint16_t startDelayMs=0);
  void playCuteYes(uint16_t startDelayMs=0);
  void playCuteNo(uint16_t startDelayMs=0);
  void playCuteSleep(uint16_t startDelayMs=0);
  // removed unused: playCuteChatter, playCuteChatterV2
  // Variable-length procedural chatter: generates a fresh melody each call
  void playCuteChatterFree(uint16_t minTotalMs=450, uint16_t maxTotalMs=1300, uint16_t startDelayMs=0);
  // Binary-ish chatter APIs (non-blocking via sequencer)
  void binaryTalkRandom(uint16_t minTotalMs=550, uint16_t maxTotalMs=1200, uint16_t startDelayMs=0);
  void binaryTalkFromBytes(const uint8_t* data, size_t len, uint16_t bitMinMs=60, uint16_t bitMaxMs=110, uint16_t startDelayMs=0);
  void playCuteStartup(uint16_t startDelayMs=0);
  void playCuteFurious(uint16_t startDelayMs=0);
  
}


