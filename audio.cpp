#include <Arduino.h>
#include <stdint.h>  // Add standard integer types
#include "config.h"  // Include configuration constants
#include "audio.h"
#include <driver/i2s.h>
#include <math.h>
#include <algorithm>
#include <esp_system.h>

namespace Audio {

  static constexpr int MAX_VOICES = 4;
  struct Voice {
    bool active = false;
    uint8_t wave = WAVE_SINE;
    uint8_t volume = 180; // 0..255
    float freqStart = 1000.0f;
    float freqEnd = 800.0f;
    uint32_t t0 = 0;
    uint32_t durMs = 0;
    float phase = 0.0f; // 0..1
  
    // NEW ✨
    float vibratoHz = 0.0f;           // LFO rate for vibrato
    float vibratoSemis = 0.0f;        // depth in semitones
    float tremoloHz = 0.0f;           // LFO rate for amplitude
    float tremoloDepth = 0.0f;        // 0..1 additional amplitude modulation
    float lfoPhase = 0.0f;            // shared LFO phase [0..1)
    uint8_t glideShape = 0;           // 0=linear, 1=ease-in, 2=ease-out
    float drive = 0.0f;               // waveshaper drive 0..1 (soft saturation)
  };

  static inline float easeIn(float x){ return x*x; }
  static inline float easeOut(float x){ return 1.0f - (1.0f-x)*(1.0f-x); }

  // lightweight waveshaper (soft tanh-ish)
  static inline float softDrive(float x, float k){
    if (k <= 0.001f) return x;
    // map k[0..1] -> gain & saturate
    float g = 1.0f + 5.0f * k;
    float y = x * g;
    // polynomial soft clip ~tanh without heavy math
    return y - (y*y*y)*0.2f;
  }

  static i2s_port_t I2S_PORT = I2S_NUM_0;
  static uint32_t g_sampleRate = 22050;
  static Voice voices[MAX_VOICES];
  static uint8_t g_masterVol = 240; // 0..255 - increased for better audibility

  // === Microphone Input Support ===
  static bool micEnabled = false;
  static uint32_t micSampleRate = 16000;
  static int16_t micBufferLeft[MIC_BUFFER_SIZE];   // Left channel buffer
  static int16_t micBufferRight[MIC_BUFFER_SIZE];  // Right channel buffer
  static int16_t micBufferStereo[MIC_BUFFER_SIZE * 2]; // Interleaved stereo buffer
  static float currentMicLevelLeft = 0.0f;   // Left channel RMS
  static float currentMicLevelRight = 0.0f;  // Right channel RMS
  static float currentMicLevel = 0.0f;       // Average RMS (backward compatibility)
  static uint32_t lastMicReadMs = 0;

  // Audio feedback prevention
  static bool audioPlaybackActive = false;
  static uint32_t lastAudioPlaybackMs = 0;

  // Microphone gain for sensitivity boost
  static constexpr float MIC_GAIN = 8.0f; // Reduced from 64x to 8x to prevent clipping

  // Forward declaration for audio playback tracking
  static void updateAudioPlaybackState();

  // Debug helper: log SFX name
  static inline void logSfx(const char* name){
    Serial.print("SFX: ");
    Serial.println(name);
  }

  static Voice* allocVoice(){
    for (int i = 0; i < MAX_VOICES; ++i)
      if (!voices[i].active) return &voices[i];
    return nullptr;
  }

  static inline float sineFast(float p){
    // p in [0,1)
    return sinf(6.2831853f * p);
  }

  static inline float noise(){
    return (float) ( (int32_t)esp_random() % 20001 - 10000 ) / 10000.0f;
  }

  // MIDI helper: A4 (69) = 440 Hz
  static inline float hzFromMidi(int midi){
    return 440.0f * powf(2.0f, (midi - 69) / 12.0f);
  }

  // === Microphone Input Functions ===
  static float calculateRMS(int16_t* buffer, size_t size) {
    if (size == 0) return 0.0f;

    float sum = 0.0f;
    for (size_t i = 0; i < size; ++i) {
      float sample = (float)buffer[i] / 32768.0f; // Convert to -1.0 to 1.0 range
      sample *= MIC_GAIN; // Apply microphone gain boost
      // Clamp to prevent overflow
      if (sample > 1.0f) sample = 1.0f;
      if (sample < -1.0f) sample = -1.0f;
      sum += sample * sample;
    }
    return sqrtf(sum / (float)size);
  }

  static void readMicrophoneData() {
    if (!micEnabled) return;

    uint32_t currentMs = millis();

    // Skip microphone reading during audio playback and cooldown period
    if (audioPlaybackActive || (currentMs - lastAudioPlaybackMs < MIC_FEEDBACK_COOLDOWN_MS)) {
      currentMicLevel = 0.0f;
      currentMicLevelLeft = 0.0f;
      currentMicLevelRight = 0.0f;
      return;
    }

    uint32_t readIntervalMs = (1000 * MIC_BUFFER_SIZE) / micSampleRate; // Time for one buffer

    if (currentMs - lastMicReadMs < readIntervalMs) {
      return; // Not time to read yet
    }

    size_t bytesRead = 0;
    // Read stereo interleaved data (L, R, L, R, ...)
    esp_err_t result = i2s_read(I2S_PORT, micBufferStereo, sizeof(micBufferStereo), &bytesRead,
                                pdMS_TO_TICKS(50));

    if (result == ESP_OK && bytesRead > 0) {
      size_t samplesRead = bytesRead / sizeof(int16_t); // Total samples (both channels)
      size_t stereoFrames = samplesRead / 2; // Number of L+R pairs
      
      if (stereoFrames > 0) {
        // De-interleave stereo data into separate left/right buffers
        for (size_t i = 0; i < stereoFrames; i++) {
          micBufferLeft[i] = micBufferStereo[i * 2];      // Left channel (even indices)
          micBufferRight[i] = micBufferStereo[i * 2 + 1]; // Right channel (odd indices)
        }
        
        // Calculate RMS for each channel
        currentMicLevelLeft = calculateRMS(micBufferLeft, stereoFrames);
        currentMicLevelRight = calculateRMS(micBufferRight, stereoFrames);
        
        // Average for backward compatibility
        currentMicLevel = (currentMicLevelLeft + currentMicLevelRight) / 2.0f;
        
        lastMicReadMs = currentMs;

        // Enhanced debug output with stereo levels
        static uint32_t lastSampleDebug = 0;
        if (currentMs - lastSampleDebug > 5000) { // Every 5 seconds
          int16_t minL = 32767, maxL = -32768, minR = 32767, maxR = -32768;
          for (size_t i = 0; i < min(stereoFrames, (size_t)10); i++) {
            if (micBufferLeft[i] < minL) minL = micBufferLeft[i];
            if (micBufferLeft[i] > maxL) maxL = micBufferLeft[i];
            if (micBufferRight[i] < minR) minR = micBufferRight[i];
            if (micBufferRight[i] > maxR) maxR = micBufferRight[i];
          }
          Serial.printf("MIC STEREO: L=%.3f[%d,%d] R=%.3f[%d,%d] Dir=%.2f Active=%s\n",
                       currentMicLevelLeft, minL, maxL,
                       currentMicLevelRight, minR, maxR,
                       getSoundDirection(),
                       (currentMicLevel > MIC_NOISE_THRESHOLD) ? "YES" : "no");
          lastSampleDebug = currentMs;
        }
      }
    } else {
      // Enhanced error reporting
      if (result != ESP_OK) {
        static uint32_t lastErrorTime = 0;
        if (currentMs - lastErrorTime > 3000) { // Report error once per 3 seconds
          const char* errStr = "UNKNOWN";
          switch(result) {
            case ESP_ERR_INVALID_ARG: errStr = "INVALID_ARG"; break;
            case ESP_ERR_TIMEOUT: errStr = "TIMEOUT"; break;
            case ESP_ERR_INVALID_STATE: errStr = "INVALID_STATE"; break;
            case ESP_FAIL: errStr = "FAIL"; break;
          }
          Serial.printf("I2S mic read error: %s (%d), bytes: %d\n", errStr, result, bytesRead);
          lastErrorTime = currentMs;
        }
      }
      currentMicLevel = 0.0f;
      currentMicLevelLeft = 0.0f;
      currentMicLevelRight = 0.0f;
    }
  }

  // Render one sample for a voice given elapsedMs since voice start
  static float renderVoiceSample(Voice &v, uint32_t elapsedMs){
    if (!v.active) return 0.0f;
    if (elapsedMs >= v.durMs) { v.active = false; return 0.0f; }
  
    // normalized time 0..1
    float u = (float)elapsedMs / (float)v.durMs;
  
    // glide curve
    float gu = u;
    if (v.glideShape == 1) gu = easeIn(u);       // ease-in
    else if (v.glideShape == 2) gu = easeOut(u); // ease-out
  
    // base freq with glide
    float f = v.freqStart + (v.freqEnd - v.freqStart) * gu;
  
    // vibrato -> multiply by 2^(semitones/12)
    if (v.vibratoHz > 0.0f && v.vibratoSemis != 0.0f) {
      // advance LFO
      v.lfoPhase += v.vibratoHz / (float)g_sampleRate;
      if (v.lfoPhase >= 1.0f) v.lfoPhase -= (int)v.lfoPhase;
      float vib = sinf(6.2831853f * v.lfoPhase);
      float ratio = powf(2.0f, (v.vibratoSemis * vib) / 12.0f);
      f *= ratio;
    }
  
    // advance osc phase
    v.phase += f / (float)g_sampleRate;
    if (v.phase >= 1.0f) v.phase -= (int)v.phase;
  
    // base waveform
    float s = 0.0f;
    switch (v.wave) {
      case WAVE_SINE:   s = sineFast(v.phase); break;
      case WAVE_SQUARE: s = (v.phase < 0.5f) ? 1.0f : -1.0f; break;
      default:          s = noise(); break;
    }
  
    // soft waveshaper drive
    s = softDrive(s, v.drive);
  
    // amplitude envelope (attack/decay)
    const float attack = 0.012f; // ~12ms
    float envA = u / attack; if (envA > 1.0f) envA = 1.0f;
    float envD = 1.0f - u;
    float env = envA * envD;
  
    // tremolo on top (re-use LFO or separate when vibratoHz=0)
    if (v.tremoloHz > 0.0f && v.tremoloDepth > 0.0f){
      float tPhase = v.lfoPhase;
      if (v.vibratoHz <= 0.0f){ // advance if vibrato not running
        tPhase += v.tremoloHz / (float)g_sampleRate;
        if (tPhase >= 1.0f) tPhase -= (int)tPhase;
        v.lfoPhase = tPhase;
      }
      float trem = 0.5f * (1.0f + sinf(6.2831853f * v.lfoPhase)); // 0..1
      env *= (1.0f - v.tremoloDepth) + v.tremoloDepth * trem;
    }
  
    return s * env * (v.volume / 255.0f);
  }
  
  struct ExTone {
    float f0;
    float f1;
    uint16_t ms;
    uint8_t wave;
    uint8_t vol;
    uint8_t glideShape;   // 0 lin, 1 in, 2 out
    float vibratoHz;
    float vibratoSemis;   // +/- semitones
    float tremoloHz;
    float tremoloDepth;   // 0..1
    float drive;          // 0..1
  };
  // Forward declaration so helpers can call playEx
  static void playEx(const ExTone& p);
  // --- helper: short noise smack for "spit"/attack ---
  static inline void noiseHit_(uint16_t ms=28, uint8_t vol=220, float tremHz=0.0f, float tremAmt=0.0f){
    playEx({ hzFromMidi(80), hzFromMidi(80), ms, WAVE_NOISE, vol, 0, 0.0f, 0.0f, tremHz, tremAmt, 0.00f });
  }

  static void playEx(const ExTone& p){
    Voice* v = allocVoice();
    if (!v) return;
    v->active = true;
    v->wave = p.wave;
    v->volume = p.vol;
    v->freqStart = p.f0;
    v->freqEnd = p.f1;
    v->t0 = millis();
    v->durMs = p.ms;
    v->phase = 0.0f;
    v->vibratoHz = p.vibratoHz;
    v->vibratoSemis = p.vibratoSemis;
    v->tremoloHz = p.tremoloHz;
    v->tremoloDepth = p.tremoloDepth;
    v->lfoPhase = 0.0f;
    v->glideShape = p.glideShape;
    v->drive = p.drive;
  }
  
  void begin(int bclkPin, int lrclkPin, int dataOutPin, uint32_t sampleRate){
    g_sampleRate = sampleRate;
    
    // Critical: Initialize pins as output BEFORE I2S config to prevent damage
    pinMode(bclkPin, OUTPUT);
    pinMode(lrclkPin, OUTPUT);
    pinMode(dataOutPin, OUTPUT);
    pinMode(I2S_DI_RIGHT, INPUT); // Shared stereo microphone input

    // Set pins to safe state (LOW) initially
    digitalWrite(bclkPin, LOW);
    digitalWrite(lrclkPin, LOW);
    digitalWrite(dataOutPin, LOW);
    
    delay(100); // Allow pins to stabilize
    
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX); // Enable both TX and RX
    cfg.sample_rate = (int)g_sampleRate;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT; // Stereo input (both mics on same data pin)
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.dma_buf_count = 8; // Increased from 4 to reduce underruns
    cfg.dma_buf_len = 512; // Increased from 256 to reduce crackling
    cfg.use_apll = true; // Use APLL for better clock stability
    cfg.tx_desc_auto_clear = true;
    cfg.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    
    // Install I2S driver
    esp_err_t result = i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
    if (result != ESP_OK) {
      Serial.printf("I2S driver install failed: %d\n", result);
      return;
    }

    i2s_pin_config_t pins = {};
    pins.bck_io_num = bclkPin;
    pins.ws_io_num = lrclkPin;
    pins.data_out_num = dataOutPin;
    pins.data_in_num = I2S_DI_RIGHT; // Shared stereo microphone data line
    // IMPORTANT: Both INMP441 mics must share the SAME data pin (I2S_DI_RIGHT)
    // Left mic: L/R pin → GND (transmits when WS=LOW)
    // Right mic: L/R pin → VDD (transmits when WS=HIGH)
    
    result = i2s_set_pin(I2S_PORT, &pins);
    if (result != ESP_OK) {
      Serial.printf("I2S pin config failed: %d\n", result);
      return;
    }
    
    // Set proper clock configuration for dual TX/RX stereo mode
    result = i2s_set_clk(I2S_PORT, g_sampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO);
    if (result != ESP_OK) {
      Serial.printf("I2S clock config failed: %d\n", result);
      return;
    }
    
    // Send initial silence to ensure clean startup
    const int N = 256;
    int16_t silence[N];
    memset(silence, 0, sizeof(silence));
    size_t written = 0;
    i2s_write(I2S_PORT, silence, sizeof(silence), &written, pdMS_TO_TICKS(100));
    
    Serial.println("I2S audio initialized successfully");
  }

  void end(){
    // Stop all voices first
    stopAll();
    
    // Give time for audio to finish playing
    delay(50);
    
    // Send silence to clear any residual noise
    const int N = 256;
    int16_t silence[N];
    memset(silence, 0, sizeof(silence));
    size_t written = 0;
    i2s_write(I2S_PORT, silence, sizeof(silence), &written, pdMS_TO_TICKS(100));
    
    // Stop I2S driver
    i2s_driver_uninstall(I2S_PORT);
  }

  void setMasterVolume(uint8_t vol){ g_masterVol = vol; }
  uint8_t getMasterVolume(){ return g_masterVol; }

  // === Public Microphone Functions ===
  bool beginMicrophone(uint32_t sampleRate) {
    if (micEnabled) return true; // Already enabled

    micSampleRate = sampleRate;

    Serial.printf("Initializing stereo microphone at %d Hz on GPIO%d (shared data line)\n", 
                  micSampleRate, I2S_DI_RIGHT);

    // Test microphone data pin
    pinMode(I2S_DI_RIGHT, INPUT);
    int testPin = digitalRead(I2S_DI_RIGHT);
    Serial.printf("Microphone data pin GPIO%d state: %s (idle state, will toggle during I2S operation)\n", 
                  I2S_DI_RIGHT, testPin ? "HIGH" : "LOW");
    
    micEnabled = true;
    lastMicReadMs = 0;
    currentMicLevel = 0.0f;
    currentMicLevelLeft = 0.0f;
    currentMicLevelRight = 0.0f;

    // Give I2S time to settle after enabling RX mode
    delay(200); // Increased delay for better stability

    Serial.printf("Stereo microphone enabled at %d Hz with 8x gain\n", micSampleRate);
    Serial.println("Testing microphones... speak or make noise to test sensitivity");

    // Test reading immediately with better error handling
    delay(500);
    
    // Clear any stale data first
    size_t bytesRead = 0;
    esp_err_t result = i2s_read(I2S_PORT, micBufferStereo, sizeof(micBufferStereo), &bytesRead, pdMS_TO_TICKS(100));
    Serial.printf("Initial mic test - Result: %d, Bytes read: %d\n", result, bytesRead);
    
    readMicrophoneData();
    Serial.printf("Initial mic levels - Left: %.3f, Right: %.3f, Avg: %.3f (threshold: %.3f)\n", 
                 currentMicLevelLeft, currentMicLevelRight, currentMicLevel, MIC_NOISE_THRESHOLD);
    
    if (currentMicLevel == 0.0f) {
      Serial.println("WARNING: Microphones appear to be not working - check wiring");
      return false;
    }

    return true;
  }

  void endMicrophone() {
    micEnabled = false;
    Serial.println("Microphone disabled");
  }

  float getMicrophoneLevel() {
    return currentMicLevel;
  }

  bool isLoudNoiseDetected() {
    return currentMicLevel > MIC_NOISE_THRESHOLD;
  }

  // === Stereo Microphone Functions ===
  
  float getMicrophoneLevelLeft() {
    return currentMicLevelLeft;
  }

  float getMicrophoneLevelRight() {
    return currentMicLevelRight;
  }

  float getSoundDirection() {
    // Calculate normalized direction from -1.0 (left) to +1.0 (right)
    // Returns 0.0 if sound is centered or if no sound detected
    float totalLevel = currentMicLevelLeft + currentMicLevelRight;
    if (totalLevel < MIC_NOISE_THRESHOLD * 0.5f) {
      return 0.0f; // Too quiet to determine direction
    }
    
    // Calculate balance: -1.0 (all left) to +1.0 (all right)
    float balance = (currentMicLevelRight - currentMicLevelLeft) / (totalLevel + 0.001f);
    
    // Clamp to valid range
    if (balance < -1.0f) balance = -1.0f;
    if (balance > 1.0f) balance = 1.0f;
    
    return balance;
  }

  bool isSoundFromLeft() {
    // Sound is from left if left channel is significantly louder
    return (currentMicLevelLeft > currentMicLevelRight * 1.3f) && 
           (currentMicLevelLeft > MIC_NOISE_THRESHOLD);
  }

  bool isSoundFromRight() {
    // Sound is from right if right channel is significantly louder
    return (currentMicLevelRight > currentMicLevelLeft * 1.3f) && 
           (currentMicLevelRight > MIC_NOISE_THRESHOLD);
  }

  // Check if audio is currently playing (for feedback prevention)
  bool isAudioPlaying() {
    return audioPlaybackActive;
  }

  // Check if microphone is in cooldown period after audio playback
  bool isMicrophoneInCooldown() {
    return (millis() - lastAudioPlaybackMs < MIC_FEEDBACK_COOLDOWN_MS);
  }

  // Hardware diagnostics function
  void runDiagnostics() {
    Serial.println("=== AUDIO HARDWARE DIAGNOSTICS ===");
    
    // Test pin configuration
    Serial.printf("I2S Pin Configuration:\n");
    Serial.printf("  BCLK:     GPIO%d\n", I2S_BCLK);
    Serial.printf("  LRC:      GPIO%d\n", I2S_LRC);
    Serial.printf("  DO:       GPIO%d\n", I2S_DO);
    Serial.printf("  DI (Stereo Mics): GPIO%d\n", I2S_DI_RIGHT);
    
    // Test pin states
    Serial.printf("Pin States:\n");
    Serial.printf("  BCLK:     %s\n", digitalRead(I2S_BCLK) ? "HIGH" : "LOW");
    Serial.printf("  LRC:      %s\n", digitalRead(I2S_LRC) ? "HIGH" : "LOW");
    Serial.printf("  DO:       %s\n", digitalRead(I2S_DO) ? "HIGH" : "LOW");
    Serial.printf("  DI (Shared): %s\n", digitalRead(I2S_DI_RIGHT) ? "HIGH" : "LOW");
    
    // Audio system status
    Serial.printf("Audio System:\n");
    Serial.printf("  Master Volume: %d/255\n", g_masterVol);
    Serial.printf("  Sample Rate: %d Hz\n", g_sampleRate);
    Serial.printf("  Mic Enabled: %s\n", micEnabled ? "YES (STEREO)" : "NO");
    Serial.printf("  Active Voices: ");
    int activeCount = 0;
    for (int i = 0; i < MAX_VOICES; i++) {
      if (voices[i].active) activeCount++;
    }
    Serial.printf("%d/%d\n", activeCount, MAX_VOICES);
    
    // Microphone status
    Serial.printf("Microphone:\n");
    Serial.printf("  Enabled: %s\n", micEnabled ? "YES" : "NO");
    Serial.printf("  Sample Rate: %d Hz\n", micSampleRate);
    Serial.printf("  Current Level: %.3f\n", currentMicLevel);
    Serial.printf("  Threshold: %.3f\n", MIC_NOISE_THRESHOLD);
    Serial.printf("  Gain: %.1fx\n", MIC_GAIN);
    Serial.printf("  In Cooldown: %s\n", isMicrophoneInCooldown() ? "YES" : "NO");
    Serial.printf("  Audio Playing: %s\n", audioPlaybackActive ? "YES" : "NO");
    
    Serial.println("=== END DIAGNOSTICS ===");
  }

  void playBeep(float freqHz, uint16_t ms, uint8_t volume, uint8_t wave){
    for (int i=0;i<MAX_VOICES;i++){
      if (!voices[i].active){
        voices[i].active = true;
        voices[i].wave = wave;
        voices[i].volume = volume;
        voices[i].freqStart = freqHz;
        voices[i].freqEnd = freqHz;
        voices[i].t0 = millis();
        voices[i].durMs = ms;
        return;
      }
    }
  }

  void playBleep(const SfxParams& a, const SfxParams& b){
    playBeep(a.baseFreqHz, a.durationMs, a.volume, a.wave);
    // second voice with glide
    for (int i=0;i<MAX_VOICES;i++){
      if (!voices[i].active){
        voices[i].active = true;
        voices[i].wave = b.wave;
        voices[i].volume = b.volume;
        voices[i].freqStart = b.baseFreqHz;
        voices[i].freqEnd = b.glideFreqHz;
        voices[i].t0 = millis();
        voices[i].durMs = b.durationMs;
        return;
      }
    }
  }

  void stopAll(){
    for (int i=0;i<MAX_VOICES;i++) voices[i].active = false;
  }

  void update(){
    // Update audio playback state (for feedback prevention)
    updateAudioPlaybackState();

    // Read microphone data periodically if enabled
    readMicrophoneData();

    // Produce a larger chunk per call to reduce I2S overhead
    const int N = 512; // Increased from 256 to reduce update frequency
    int16_t buffer[N];
    uint32_t frameStartMs = millis();
    const float masterGain = 0.8f * (g_masterVol / 255.0f); // Reduced gain significantly to prevent overheating
    for (int n=0;n<N;n++){
      float mix = 0.0f;
      uint32_t tMsOffset = (uint32_t)((1000.0f * n) / (float)g_sampleRate);
      for (int i=0;i<MAX_VOICES;i++) {
        if (!voices[i].active) continue;
        uint32_t elapsed = (frameStartMs - voices[i].t0) + tMsOffset;
        mix += renderVoiceSample(voices[i], elapsed);
      }
      // Apply power limiting to prevent speaker damage
      mix *= masterGain;
      
      // Hard limiting at lower levels to prevent overheating
      if (mix > 0.7f) mix = 0.7f; 
      if (mix < -0.7f) mix = -0.7f;
      
      // Additional soft compression to reduce peaks
      if (mix > 0.5f) mix = 0.5f + (mix - 0.5f) * 0.3f;
      if (mix < -0.5f) mix = -0.5f + (mix + 0.5f) * 0.3f;
      
      buffer[n] = (int16_t)(mix * 32767.0f);
    }
    size_t written = 0;
    // Use longer timeout to prevent buffer underruns
    esp_err_t result = i2s_write(I2S_PORT, buffer, sizeof(buffer), &written, pdMS_TO_TICKS(20));
    if (result != ESP_OK && result != ESP_ERR_TIMEOUT) {
      static uint32_t lastErrorMs = 0;
      uint32_t now = millis();
      if (now - lastErrorMs > 5000) { // Log error once per 5 seconds
        Serial.printf("I2S write error: %d\n", result);
        lastErrorMs = now;
      }
    }
  }

  // --- Presets ---
  // removed unused: sfxHappy



// ======================= ANGRY v2 =======================
  void sfxAngry(){
    logSfx("Angry v2");

    // 1) consonant "spit" on the attack
    noiseHit_(26, 220, 0.0f, 0.0f);
    delay(6);

    // 2) snarly down-sweep (square) with gentle vibrato + drive
    ExTone lead {
      hzFromMidi(82), hzFromMidi(69), 190, // G#5 -> A4
      WAVE_SQUARE, 205,
      1,              // ease-in fall = sharp start, then drop
      5.5f, 0.22f,    // vibrato
      0.0f, 0.0f,     // tremolo
      0.22f           // drive
    };
    playEx(lead);

    // 3) sub layer an octave-ish below for body (slightly later)
    ExTone sub {
      hzFromMidi(70), hzFromMidi(57), 210, // A#4 -> A3
      WAVE_SQUARE, 165,
      1,
      4.0f, 0.18f,
      0.0f, 0.0f,
      0.28f
    };
    delay(12);
    playEx(sub);

    // 4) grumbly tail (low sine with tremolo wobble)
    ExTone tail {
      hzFromMidi(57), hzFromMidi(55), 220, // A3 -> G3
      WAVE_SINE, 150,
      0,
      3.0f, 0.10f,
      6.0f, 0.45f,    // tremolo Hz, depth
      0.02f
    };
    delay(40);
    playEx(tail);
  }

  // ======================= FURIOUS v2 =======================
  void sfxFurious(){
    logSfx("Furious v2");

    // Constant raspy bed (AM on noise) for ~400 ms, sits under the siren
    playEx({ hzFromMidi(84), hzFromMidi(84), 400, WAVE_NOISE, 160, 0, 0.0f, 0.0f, 16.0f, 0.55f, 0.00f });

    // “Red alert" siren: rapid alternating minor-third with strong tremolo
    // Primary high voice
    ExTone hi {
      hzFromMidi(87), hzFromMidi(87), 100, // D#6
      WAVE_SQUARE, 215,
      0,
      0.0f, 0.0f,
      13.5f, 0.48f,   // tremolo
      0.25f
    };
    // Secondary just below for thickness
    ExTone lo = hi;
    lo.f0 = lo.f1 = hzFromMidi(84); // C6
    lo.vol = 195; lo.drive = 0.22f;

    // Trill run (keeps within MAX_VOICES)
    for (int i=0; i<6; ++i){
      playEx((i & 1) ? hi : lo);        // alternate C6 <-> D#6
      delay(18);
    }

    // Aggressive down-chirp to release tension
    ExTone fall {
      hzFromMidi(88), hzFromMidi(76), 170, // E6 -> E5
      WAVE_SQUARE, 205,
      1,              // ease-in fall
      7.0f, 0.18f,    // a little vibrato shimmer
      0.0f, 0.0f,
      0.22f
    };
    delay(25);
    playEx(fall);

    // Final spit
    delay(18);
    noiseHit_(32, 210, 0.0f, 0.0f);
  }


  void sfxCurious(){
    logSfx("Curious");
    // Vector-inspired curious question: ascending melody with questioning rhythm
    ExTone t1{ hzFromMidi(79), hzFromMidi(79), 100, WAVE_SINE, 180, 0, 8.0f, 0.15f, 0.0f, 0.0f, 0.05f }; // G5
    ExTone t2{ hzFromMidi(82), hzFromMidi(82), 120, WAVE_SINE, 175, 0, 9.0f, 0.12f, 0.0f, 0.0f, 0.04f }; // A#5
    ExTone t3{ hzFromMidi(86), hzFromMidi(86), 140, WAVE_SINE, 170, 0, 10.0f, 0.10f, 0.0f, 0.0f, 0.03f }; // D6
    playEx(t1); delay(60);
    playEx(t2); delay(80);
    playEx(t3);
  }

  void sfxBlink(){
    logSfx("Blink");
    // Vector-inspired gentle blink: soft high tone with subtle vibrato
    ExTone t{ hzFromMidi(91), hzFromMidi(91), 80, WAVE_SINE, 140, 0, 12.0f, 0.08f, 0.0f, 0.0f, 0.02f }; // G6
    playEx(t);
  }

  void sfxNotify(){
    logSfx("Notify");
    // Vector-inspired notification: cheerful melody with personality
    ExTone n1{ hzFromMidi(84), hzFromMidi(84), 120, WAVE_SINE, 200, 0, 8.0f, 0.18f, 0.0f, 0.0f, 0.06f }; // C6
    ExTone n2{ hzFromMidi(86), hzFromMidi(86), 100, WAVE_SINE, 190, 0, 9.0f, 0.16f, 0.0f, 0.0f, 0.05f }; // D6
    ExTone n3{ hzFromMidi(88), hzFromMidi(88), 140, WAVE_SINE, 185, 0, 10.0f, 0.14f, 0.0f, 0.0f, 0.04f }; // E6
    ExTone n4{ hzFromMidi(91), hzFromMidi(91), 160, WAVE_SINE, 180, 0, 11.0f, 0.12f, 0.0f, 0.0f, 0.03f }; // G6
    playEx(n1); delay(30);
    playEx(n2); delay(25);
    playEx(n3); delay(35);
    playEx(n4);
  }

  void sfxConfirm(){
    logSfx("Confirm");
    // Vector-inspired affirmative: ascending perfect fifth with joyful rhythm
    ExTone a{ hzFromMidi(79), hzFromMidi(79), 120, WAVE_SINE, 200, 2, 8.0f, 0.20f, 0.0f, 0.0f, 0.06f }; // G5
    ExTone b{ hzFromMidi(84), hzFromMidi(84), 140, WAVE_SINE, 185, 2, 9.0f, 0.18f, 0.0f, 0.0f, 0.05f }; // C6
    playEx(a); delay(40);
    playEx(b);
  }

  void sfxError(){
    logSfx("Error");
    // Vector-inspired concerned: descending minor third with gentle expression
    ExTone a{ hzFromMidi(81), hzFromMidi(81), 140, WAVE_SINE, 200, 1, 6.0f, 0.15f, 0.0f, 0.0f, 0.08f }; // A5
    ExTone b{ hzFromMidi(78), hzFromMidi(78), 160, WAVE_SINE, 190, 1, 5.5f, 0.12f, 0.0f, 0.0f, 0.06f }; // F#5
    playEx(a); delay(50);
    playEx(b);
  }

  // removed unused: sfxStartup
}

// ===== New Droid-y Presets & Non-blocking Cute Sequencer =====
namespace Audio {
// removed unused: sfxDroidChirp
  static float midiToHz(uint8_t n){
    return 440.0f * powf(2.0f, ((int)n - 69) / 12.0f);
  }

  // Forward decl for random helper used in cute note wrapper
  static inline float frand(float a, float b);

  // --- Expressive sequencer note wrapper (Vector-ish) ---
  static float seqPrevHz = -1.0f; // remember last freq to glide from

  static void playNoteCute_(uint8_t midi, uint16_t ms, uint8_t vol, uint8_t wave){
    if (midi == 0 || ms == 0) { seqPrevHz = -1.0f; return; }
    float f = midiToHz(midi);
    ExTone t{};
    t.f0 = (seqPrevHz > 0.0f) ? seqPrevHz : f; // glide from previous
    t.f1 = f;
    t.ms = ms;
    t.wave = wave;
    t.vol  = vol;
    t.glideShape   = 2;                      // ease-out = cute rise feel
    t.vibratoHz    = 8.5f + frand(-0.8f, 0.8f);
    t.vibratoSemis = 0.26f + frand(-0.06f, 0.06f);
    t.tremoloHz    = 0.0f;
    t.tremoloDepth = 0.0f;
    t.drive        = 0.04f;                  // tiny warmth
    playEx(t);
    seqPrevHz = f;
  }

  struct SeqState {
    const NoteEv* ev = nullptr;
    uint8_t len = 0;
    uint8_t idx = 0;
    bool playing = false;
    uint32_t nextAt = 0;
    uint16_t gapMs = 12;
  } static seq;

  // Audio playback tracking (placed after seq declaration)
  static void updateAudioPlaybackState() {
    bool wasActive = audioPlaybackActive;
    audioPlaybackActive = false;

    // Check if any voices are active
    for (int i = 0; i < MAX_VOICES; ++i) {
      if (voices[i].active) {
        audioPlaybackActive = true;
        break;
      }
    }

    // Also check sequencer
    if (seq.playing) {
      audioPlaybackActive = true;
    }

    // Track when audio playback ends
    if (wasActive && !audioPlaybackActive) {
      lastAudioPlaybackMs = millis();
    }
  }

  void startSequence(const NoteEv* events, uint8_t len, uint16_t startDelayMs){
    Serial.printf("SFX: SeqStart(len=%u)\n", len);
    seq.ev = events; seq.len = len; seq.idx = 0;
    seq.playing = (len > 0);
    seq.nextAt = millis() + startDelayMs;
  }

  bool isSequencePlaying(){ return seq.playing; }

  void sequencerUpdate(){
    if (!seq.playing) return;
    uint32_t now = millis();
    if (now < seq.nextAt) return;

    const NoteEv& e = seq.ev[seq.idx];
    if (e.midi == 0) {
      seqPrevHz = -1.0f;                  // reset glide on rests
      seq.nextAt = now + e.ms;
    } else {
      // tiny randomization for life
      int8_t vj = (int8_t)(esp_random() % 21) - 10; // -10..+10
      int8_t dj = (int8_t)(esp_random() % 11) - 5;  // -5..+5
      uint8_t vol = (uint8_t)max(0, min(255, (int)e.vol + vj));
      uint16_t dur = (uint16_t)max(20, (int)e.ms + dj);
      playNoteCute_(e.midi, dur, vol, e.wave);
      seq.nextAt = now + dur + seq.gapMs;
    }

    seq.idx++;
    if (seq.idx >= seq.len) seq.playing = false;
  }

  #define ARRLEN(x) (uint8_t)(sizeof(x)/sizeof(x[0]))

  // Cute lines (Vector-ish)
  static const NoteEv CUTE_HELLO[] = {
    {76,120,170,WAVE_SINE}, {79,140,180,WAVE_SINE}, {83,160,180,WAVE_SINE},
    { 0, 30,  0, 0}, {88,220,190,WAVE_SINE}
  };
  static const NoteEv CUTE_QUESTION[] = {
    {72,90,160,WAVE_SINE}, {74,90,170,WAVE_SINE}, {76,110,175,WAVE_SINE},
    {79,160,185,WAVE_SINE}
  };
  static const NoteEv CUTE_YES[] = {
    {76,80,170,WAVE_SINE}, {81,90,180,WAVE_SINE}, {88,120,190,WAVE_SINE}
  };
  static const NoteEv CUTE_NO[] = {
    {76,110,170,WAVE_SINE}, {72,130,165,WAVE_SINE}, {69,160,150,WAVE_SINE}
  };
  static const NoteEv CUTE_SLEEP[] = {
    {65,220,130,WAVE_SINE}, {62,260,120,WAVE_SINE}, {57,360,110,WAVE_SINE}
  };
  static const NoteEv CUTE_CHATTER[] = {
    {79,70,165,WAVE_SINE}, {77,70,160,WAVE_SINE}, {81,70,170,WAVE_SINE},
    { 0,25,0,0}, {76,70,165,WAVE_SINE}, {84,90,180,WAVE_SINE}
  };

  // Wrappers
  void playCuteHello(uint16_t d){ startSequence(CUTE_HELLO, ARRLEN(CUTE_HELLO), d); }
  void playCuteQuestion(uint16_t d){ startSequence(CUTE_QUESTION, ARRLEN(CUTE_QUESTION), d); }
  void playCuteYes(uint16_t d){ startSequence(CUTE_YES, ARRLEN(CUTE_YES), d); }
  void playCuteNo(uint16_t d){ startSequence(CUTE_NO, ARRLEN(CUTE_NO), d); }
  void playCuteSleep(uint16_t d){ startSequence(CUTE_SLEEP, ARRLEN(CUTE_SLEEP), d); }
  // Additional short, cute chatter variations (pentatonic-ish)
  static const NoteEv CHATT_1[] = { // up
    {84,80,170,WAVE_SINE}, {86,90,175,WAVE_SINE}, {88,110,180,WAVE_SINE}
  };
  static const NoteEv CHATT_2[] = { // down
    {88,90,175,WAVE_SINE}, {86,80,170,WAVE_SINE}, {84,110,180,WAVE_SINE}
  };
  static const NoteEv CHATT_3[] = { // bounce
    {84,70,165,WAVE_SINE}, {88,90,175,WAVE_SINE}, {86,100,170,WAVE_SINE}
  };
  static const NoteEv CHATT_4[] = { // questiony
    {79,70,165,WAVE_SINE}, {84,80,170,WAVE_SINE}, {86,120,180,WAVE_SINE}
  };

  void playCuteStartup(uint16_t d){ playCuteHello(d); }
  void playCuteFurious(uint16_t d){ startSequence(CUTE_NO, ARRLEN(CUTE_NO), d); }

// removed unused: sfxDroidQuestion

// removed unused: sfxDroidExclaim

void sfxDroidYes(){ // Vector-inspired affirmative: cheerful rising interval
  logSfx("DroidYes");
  playEx({ hzFromMidi(79), hzFromMidi(79), 100, WAVE_SINE, 180, 2, 9.0f, 0.25f, 0.0f, 0.0f, 0.06f }); // G5
  delay(30);
  playEx({ hzFromMidi(84), hzFromMidi(84), 120, WAVE_SINE, 170, 2, 10.0f, 0.20f, 0.0f, 0.0f, 0.05f }); // C6
}

void sfxDroidNo(){ // Vector-inspired negative: gentle descending interval
  logSfx("DroidNo");
  playEx({ hzFromMidi(81), hzFromMidi(81), 120, WAVE_SINE, 180, 1, 8.0f, 0.20f, 0.0f, 0.0f, 0.06f }); // A5
  delay(35);
  playEx({ hzFromMidi(78), hzFromMidi(78), 140, WAVE_SINE, 170, 1, 7.5f, 0.15f, 0.0f, 0.0f, 0.05f }); // F#5
}


// Random helpers
static inline float frand(float a, float b){
  return a + (b - a) * ( (float)(esp_random() & 0xFFFF) / 65535.0f );
}
static inline int irand(int a, int b){ // inclusive
  return a + (int)(esp_random() % (uint32_t)(b - a + 1));
}

// ---- Cute/Vector-style chatter generator ----
static const int PENTA_DEG[] = {0, 2, 4, 7, 9, 12}; // major pentatonic degrees
static inline int pick(const int* arr, int n){ return arr[irand(0, n-1)]; }

static void pushNote(NoteEv* buf, uint8_t& n, uint8_t midi, uint16_t ms, uint8_t vol, uint8_t wave){
  if (n < 32) buf[n++] = { midi, ms, vol, wave };
}
static void pushRest(NoteEv* buf, uint8_t& n, uint16_t ms){
  if (n < 32) buf[n++] = { 0, ms, 0, 0 };
}

// one tiny chance to add a grace note approaching the target
static void pushGraceTo(NoteEv* b, uint8_t& n, uint8_t target, uint8_t vol){
  if (irand(0, 99) < 35) { // ~35% chance
    int8_t step = (irand(0,1) ? +1 : -1);
    uint8_t g = (uint8_t)max(1, min(127, (int)target + step));
    pushNote(b, n, g, (uint16_t)irand(28, 42), (uint8_t)max(50, vol-25), WAVE_SINE);
  }
}

void playCuteChatterV2(uint16_t totalMs, uint16_t startDelayMs){
  // Reduced from 32 to 16 - sufficient for short phrases
  static NoteEv ev[16];
  uint8_t n = 0;

  // pick a pleasant register near Vector's voice
  const int baseChoices[] = {76, 79, 81, 83, 84}; // E5, G5, A5, B5, C6
  int base = baseChoices[irand(0, 4)];
  auto pent = [&](){ return (uint8_t)(base + PENTA_DEG[irand(0, 5)]); };

  // --- Phrase A (up-bounce motif) ---
  uint8_t A1 = pent();
  int tmpA2[3] = {4,7,9};
  uint8_t A2 = (uint8_t)(A1 + tmpA2[irand(0,2)]); // jump M3/P5/M6
  int tmpA3[3] = {-2,-3,-5};
  uint8_t A3 = (uint8_t)(A2 + tmpA3[irand(0,2)]);
  pushGraceTo(ev,n,A1,170); pushNote(ev,n,A1, (uint16_t)irand(70,95), 170, WAVE_SINE);
  pushNote(ev,n,A2, (uint16_t)irand(80,110), 175, WAVE_SINE);
  pushNote(ev,n,A3, (uint16_t)irand(90,120), 175, WAVE_SINE);
  pushRest(ev,n, (uint16_t)irand(25,45)); // breathing gap

  // --- Phrase B (answer, slightly higher, ends with a "question" lift) ---
  uint8_t B1 = pent();
  int tmpB2[3] = {2,4,7};
  uint8_t B2 = (uint8_t)(B1 + tmpB2[irand(0,2)]);
  int tmpB3[3] = {-2,0,2};
  uint8_t B3 = (uint8_t)(B2 + tmpB3[irand(0,2)]);
  int tmpB4[2] = {4,5};
  uint8_t B4 = (uint8_t)(B3 + tmpB4[irand(0,1)]); // little lift
  pushGraceTo(ev,n,B1,170); pushNote(ev,n,B1, (uint16_t)irand(70,95), 170, WAVE_SINE);
  pushNote(ev,n,B2, (uint16_t)irand(80,100), 175, WAVE_SINE);
  pushNote(ev,n,B3, (uint16_t)irand(85,110), 175, WAVE_SINE);
  pushNote(ev,n,B4, (uint16_t)irand(110,140), 185, (irand(0,4)==0)?WAVE_SQUARE:WAVE_SINE); // occasional bright accent

  // Optional tiny tail chirp
  if (irand(0,99) < 60) {
    pushRest(ev,n, (uint16_t)irand(20,35));
    int tmpTail[3] = {2,4,7};
    uint8_t tail = (uint8_t)(B4 + tmpTail[irand(0,2)]);
    pushGraceTo(ev,n,tail,180);
    pushNote(ev,n, tail, (uint16_t)irand(80,120), 185, WAVE_SINE);
  }

  // Trim by totalMs if needed (roughly)
  uint16_t acc=0; uint8_t m=0;
  for (; m<n; ++m){ acc += ev[m].ms; if (acc >= totalMs) { m++; break; } }
  if (m==0) m=n;
  startSequence(ev, m, startDelayMs);
}

} // namespace Audio
// ===== Free-form pentatonic chatter =====
namespace Audio {
void playCuteChatterFree(uint16_t minTotalMs, uint16_t maxTotalMs, uint16_t startDelayMs){
  if (minTotalMs > maxTotalMs) { uint16_t t=minTotalMs; minTotalMs=maxTotalMs; maxTotalMs=t; }
  uint16_t target = (uint16_t)irand(minTotalMs, maxTotalMs);
  // Reduced from 32 to 20 - sufficient for most procedural chatter
  static NoteEv ev[20];
  uint8_t n = 0;

  // base register similar to Vector
  const int baseChoices[] = {74, 76, 79, 81, 83, 84};
  int base = baseChoices[irand(0, (int)(sizeof(baseChoices)/sizeof(baseChoices[0])) - 1)];
  auto pent = [&](){ return (uint8_t)(base + PENTA_DEG[irand(0, 5)]); };

  // Build phrases until target reached
  uint16_t acc=0;
  while (n < 32 && acc < target){
    // choose phrase length 2..5 notes
    int notes = irand(2,5);
    for (int i=0;i<notes && n<32;i++){
      uint8_t nn = pent();
      // occasionally repeat/hold previous note
      if (i>0 && irand(0,99) < 25) nn = ev[n-1].midi;
      uint16_t dur = (uint16_t)irand(60, 160);
      // rare longer held tone
      if (irand(0,99) < 12) dur = (uint16_t)irand(180, 320);
      uint8_t vol = (uint8_t)irand(150, 200);
      uint8_t wave = (irand(0,5)==0) ? WAVE_SQUARE : WAVE_SINE; // rare bright accent
      pushGraceTo(ev, n, nn, vol);
      pushNote(ev, n, nn, dur, vol, wave);
      acc += dur;
      if (acc >= target || n>=32) break;
      // optional short rest between notes
      if (irand(0,99) < 30) { uint16_t r=(uint16_t)irand(20,50); pushRest(ev, n, r); acc += r; }
    }
    // small breathing gap between phrases
    if (n<32 && acc<target) { uint16_t r=(uint16_t)irand(35,80); pushRest(ev, n, r); acc += r; }
  }
  startSequence(ev, n, startDelayMs);
}
}

// ===== Binary-ish chatter: randomized bitstream with holds, variable height =====
namespace Audio {
void binaryTalkRandom(uint16_t minTotalMs, uint16_t maxTotalMs, uint16_t startDelayMs){
  if (minTotalMs > maxTotalMs) { uint16_t t=minTotalMs; minTotalMs=maxTotalMs; maxTotalMs=t; }
  uint16_t target = (uint16_t)irand(minTotalMs, maxTotalMs);

  // Reduced from 64 to 32 - sufficient for binary patterns
  static NoteEv ev[32];
  uint8_t n = 0;
  uint16_t acc = 0;

  const int baseChoices[] = {74, 76, 79, 81, 83, 84, 86, 88};
  const int steps[] = {3,4,5,7};
  int base = baseChoices[irand(0, (int)(sizeof(baseChoices)/sizeof(baseChoices[0]))-1)];
  int step = steps[irand(0,3)];
  int low = base, high = base + step;

  uint8_t level = (uint8_t)irand(0,1);
  int runsToChange = irand(3,6);
  int runCount = 0;

  while (n < 64 && acc < target){
    int bits = irand(1,6);
    uint16_t bitMs = (uint16_t)irand(55,120);
    uint16_t dur = (uint16_t)(bits * bitMs);
    if (irand(0,99) < 18) dur += (uint16_t)irand(40,130);

    uint8_t midi = (uint8_t)( level ? high : low );
    uint8_t vol  = (uint8_t)irand(160, 205);
    uint8_t wave = (irand(0,5)==0) ? WAVE_SQUARE : WAVE_SINE;

    if (dur >= 180 && irand(0,99) < 40 && n <= 62){
      uint16_t d1 = (uint16_t)(dur * frand(0.45f, 0.65f));
      uint16_t d2 = dur - d1;
      pushNote(ev, n, midi, d1, vol, wave);
      uint8_t bend = (uint8_t)max(1, min(127, (int)midi + (irand(0,1) ? +1 : -1)));
      pushNote(ev, n, bend, d2, (uint8_t)max(130, (int)vol-8), wave);
      acc += d1 + d2;
    } else {
      pushNote(ev, n, midi, dur, vol, wave);
      acc += dur;
    }
    if (acc >= target || n >= 64) break;

    if (irand(0,99) < 35) { uint16_t r=(uint16_t)irand(18,50); pushRest(ev,n,r); acc += r; }
    if (irand(0,99) < 70) level ^= 1;
    if (irand(0,99) < 12 && n < 64){
      uint8_t m = (uint8_t)min(127, (int)(level ? high : low) + 12);
      uint16_t d = (uint16_t)irand(40,70);
      pushNote(ev, n, m, d, (uint8_t)irand(175, 210), WAVE_SQUARE);
      acc += d;
    }

    if (++runCount >= runsToChange){
      base = baseChoices[irand(0, (int)(sizeof(baseChoices)/sizeof(baseChoices[0]))-1)];
      step = steps[irand(0,3)];
      low = base; high = base + step;
      runCount = 0;
      runsToChange = irand(3,6);
      uint16_t r=(uint16_t)irand(60,120);
      pushRest(ev,n,r); acc += r;
    }
  }

  startSequence(ev, n, startDelayMs);
}

// ===== Procedural SFX sequence parser =====
namespace Audio {
static uint8_t waveFromChar(char c){
  switch (c) {
    case 'S': case 's': return WAVE_SINE;
    case 'Q': case 'q': return WAVE_SQUARE;
    case 'N': case 'n': return WAVE_NOISE;
    default: return WAVE_SINE;
  }
}

void playSfxSequence(const char* spec){
  if (!spec) return;
  const char* s = strstr(spec, ":");
  if (s) s++; else s = spec;
  while (*s){
    while (*s==' '||*s=='\t'||*s==';') s++;
    if (!*s) break;
    float f0=0, f1=0; unsigned ms=0; char w='S'; unsigned vol=180;
    const char* p = s;
    f0 = strtof(p, (char**)&p);
    if (*p=='>'){ p++; f1 = strtof(p,(char**)&p);} else { f1 = f0; }
    if (*p==',') p++;
    ms = (unsigned)strtoul(p, (char**)&p, 10);
    if (*p==',') p++;
    if (*p){ w = *p; p++; }
    if (*p==',') p++;
    vol = (unsigned)strtoul(p, (char**)&p, 10);

    if (ms > 2000) ms = 2000;
    if (vol > 255) vol = 255;
    uint8_t wave = waveFromChar(w);
    playBeep(f0, (uint16_t)ms, (uint8_t)vol, wave);
    delay(6);
    s = strchr(p, ';');
    if (!s) break; else s++;
  }
}
}
}

// ===== Binary from bytes (MSB first), with holds and register shifts =====
namespace Audio {
void binaryTalkFromBytes(const uint8_t* data, size_t len, uint16_t bitMinMs, uint16_t bitMaxMs, uint16_t startDelayMs){
  if (!data || !len) return;

  // Reduced from 96 to 48 - sufficient for most data patterns
  static NoteEv ev[48];
  uint8_t n = 0;
  uint16_t acc = 0;

  const int baseChoices[] = {74, 76, 79, 81, 83, 84, 86, 88};
  const int steps[] = {3,4,5,7};
  int base = baseChoices[irand(0,7)];
  int step = steps[irand(0,3)];
  int low = base, high = base + step;

  bool haveRun = false;
  uint8_t runMidi = 0;
  uint16_t runDur = 0;

  auto flushRun = [&](){
    if (!haveRun) return;
    if (runDur >= 180 && irand(0,99)<35 && n <= 94){
      uint16_t d1 = (uint16_t)(runDur * frand(0.45f, 0.65f));
      uint16_t d2 = runDur - d1;
      uint8_t vol = (uint8_t)irand(160, 205);
      pushNote(ev, n, runMidi, d1, vol, WAVE_SINE);
      uint8_t bend = (uint8_t)max(1, min(127, (int)runMidi + (irand(0,1)? +1 : -1)));
      pushNote(ev, n, bend, d2, (uint8_t)max(130, (int)vol-8), WAVE_SINE);
    } else {
      pushNote(ev, n, runMidi, runDur, (uint8_t)irand(160, 205), (irand(0,6)==0)? WAVE_SQUARE : WAVE_SINE);
    }
    haveRun = false; runDur = 0;
  };

  int bytesTillShift = irand(2,4);

  for (size_t i=0; i<len && n<96; ++i){
    uint8_t b = data[i];
    uint16_t bitMs = (uint16_t)irand(bitMinMs, bitMaxMs);

    for (int bit=7; bit>=0 && n<96; --bit){
      uint8_t bitVal = (b >> bit) & 1;
      uint8_t midi = (uint8_t)(bitVal ? high : low);

      if (!haveRun){ haveRun = true; runMidi = midi; runDur = bitMs; }
      else if (midi == runMidi){ runDur += bitMs; }
      else { flushRun(); haveRun = true; runMidi = midi; runDur = bitMs; }

      if (irand(0,99) < 6 && n < 96){ uint16_t r=(uint16_t)irand(15,35); pushRest(ev,n,r); }
    }

    if (--bytesTillShift <= 0){
      flushRun();
      if (n < 96){ uint16_t r=(uint16_t)irand(50,110); pushRest(ev,n,r); }
      base = baseChoices[irand(0,7)];
      step = steps[irand(0,3)];
      low = base; high = base + step;
      bytesTillShift = irand(2,4);
    } else {
      flushRun();
      if (n < 96){ uint16_t r=(uint16_t)irand(25,60); pushRest(ev,n,r); }
    }
  }
  flushRun();
  startSequence(ev, n, startDelayMs);
}
}
