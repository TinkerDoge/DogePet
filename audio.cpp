#include <Arduino.h>
#include "audio.h"
#include <driver/i2s.h>

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
    i2s_config_t cfg = {};
    cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    cfg.sample_rate = (int)g_sampleRate;
    cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.dma_buf_count = 4;
    cfg.dma_buf_len = 256;
    cfg.use_apll = false;
    cfg.tx_desc_auto_clear = true;
    cfg.intr_alloc_flags = 0;
    i2s_driver_install(I2S_PORT, &cfg, 0, NULL);

    i2s_pin_config_t pins = {};
    pins.bck_io_num = bclkPin;
    pins.ws_io_num = lrclkPin;
    pins.data_out_num = dataOutPin;
    pins.data_in_num = I2S_PIN_NO_CHANGE;
    i2s_set_pin(I2S_PORT, &pins);
  }

  void end(){
    i2s_driver_uninstall(I2S_PORT);
  }

  void setMasterVolume(uint8_t vol){ g_masterVol = vol; }
  uint8_t getMasterVolume(){ return g_masterVol; }

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
    // Produce a small chunk per call
    const int N = 256;
    int16_t buffer[N];
    uint32_t frameStartMs = millis();
    const float masterGain = 1.6f * (g_masterVol / 200.0f); // scale by master volume
    for (int n=0;n<N;n++){
      float mix = 0.0f;
      uint32_t tMsOffset = (uint32_t)((1000.0f * n) / (float)g_sampleRate);
      for (int i=0;i<MAX_VOICES;i++) {
        if (!voices[i].active) continue;
        uint32_t elapsed = (frameStartMs - voices[i].t0) + tMsOffset;
        mix += renderVoiceSample(voices[i], elapsed);
      }
      // soft clip
      mix *= masterGain;
      if (mix > 0.97f) mix = 0.97f; if (mix < -0.97f) mix = -0.97f;
      buffer[n] = (int16_t)(mix * 32767.0f);
    }
    size_t written = 0;
    i2s_write(I2S_PORT, buffer, sizeof(buffer), &written, 0);
  }

  // --- Presets ---
  void sfxHappy(){
    logSfx("Happy");
    // Vector-inspired happy melody: C6-E6-G6 arpeggio with vibrato and personality
    ExTone t1{ hzFromMidi(84), hzFromMidi(84), 120, WAVE_SINE, 200, 0, 7.0f, 0.25f, 0.0f, 0.0f, 0.08f }; // C6
    ExTone t2{ hzFromMidi(88), hzFromMidi(88), 140, WAVE_SINE, 190, 0, 8.0f, 0.22f, 0.0f, 0.0f, 0.06f }; // E6
    ExTone t3{ hzFromMidi(91), hzFromMidi(91), 160, WAVE_SINE, 180, 0, 9.0f, 0.20f, 0.0f, 0.0f, 0.05f }; // G6
    playEx(t1); delay(35);
    playEx(t2); delay(40);
    playEx(t3);
  }



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

  void sfxStartup(){
    logSfx("Startup");
    // Vector-inspired cheerful awakening: major scale melody with personality
    const int notes[] = {72, 74, 76, 79, 81, 83, 84, 86}; // C5-D5-E5-G5-A5-B5-C6-D6
    const int durations[] = {100, 90, 110, 130, 120, 100, 140, 160};
    for (int i=0;i<8;i++) {
      ExTone n{ hzFromMidi(notes[i]), hzFromMidi(notes[i]), durations[i], WAVE_SINE, 185, 0, 8.0f, 0.15f, 0.0f, 0.0f, 0.05f };
      playEx(n);
      delay(25 + i*2); // slight acceleration for excitement
    }
  }
}

// ===== New Droid-y Presets & Non-blocking Cute Sequencer =====
namespace Audio {
void sfxDroidChirp(){ // Vector-inspired curious chirp: melodic ascending with personality
  logSfx("DroidChirp");
  playEx({ hzFromMidi(84), hzFromMidi(84), 100, WAVE_SINE, 180, 2, 9.0f, 0.25f, 0.0f, 0.0f, 0.08f }); // C6
  delay(30);
  playEx({ hzFromMidi(88), hzFromMidi(88), 120, WAVE_SINE, 170, 2, 10.0f, 0.20f, 0.0f, 0.0f, 0.06f }); // E6
}
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
  void playCuteChatter(uint16_t d){
    int r = (int)(esp_random() % 4);
    switch (r){
      case 0: startSequence(CHATT_1, ARRLEN(CHATT_1), d); break;
      case 1: startSequence(CHATT_2, ARRLEN(CHATT_2), d); break;
      case 2: startSequence(CHATT_3, ARRLEN(CHATT_3), d); break;
      default:startSequence(CHATT_4, ARRLEN(CHATT_4), d); break;
    }
  }
  void playCuteStartup(uint16_t d){ playCuteHello(d); }
  void playCuteFurious(uint16_t d){ startSequence(CUTE_NO, ARRLEN(CUTE_NO), d); }

void sfxDroidQuestion(){ // Vector-inspired questioning: melodic rising phrase
  logSfx("DroidQuestion");
  playEx({ hzFromMidi(79), hzFromMidi(79), 140, WAVE_SINE, 170, 1, 8.0f, 0.25f, 0.0f, 0.0f, 0.06f }); // G5
  delay(40);
  playEx({ hzFromMidi(83), hzFromMidi(83), 160, WAVE_SINE, 160, 1, 9.0f, 0.20f, 0.0f, 0.0f, 0.05f }); // B5
  delay(50);
  playEx({ hzFromMidi(86), hzFromMidi(86), 180, WAVE_SINE, 150, 1, 10.0f, 0.15f, 0.0f, 0.0f, 0.04f }); // D6
}

void sfxDroidExclaim(){ // Vector-inspired excited: joyful descending arpeggio
  logSfx("DroidExclaim");
  playEx({ hzFromMidi(91), hzFromMidi(91), 120, WAVE_SINE, 200, 2, 10.0f, 0.30f, 0.0f, 0.0f, 0.08f }); // G6
  delay(25);
  playEx({ hzFromMidi(88), hzFromMidi(88), 100, WAVE_SINE, 190, 2, 9.0f, 0.25f, 0.0f, 0.0f, 0.06f }); // E6
  delay(20);
  playEx({ hzFromMidi(84), hzFromMidi(84), 140, WAVE_SINE, 180, 2, 8.0f, 0.20f, 0.0f, 0.0f, 0.05f }); // C6
}

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

void sfxDroidThinking(){ // Vector-inspired thinking: gentle pulsing with subtle vibrato
  logSfx("DroidThinking");
  playEx({ hzFromMidi(69), hzFromMidi(71), 600, WAVE_SINE, 140, 0, 4.0f, 0.12f, 3.0f, 0.35f, 0.02f }); // A4 to B4
}

// removed: old chatter in favor of playCuteChatterV2

// Random helpers
static inline float frand(float a, float b){
  return a + (b - a) * ( (float)(esp_random() & 0xFFFF) / 65535.0f );
}
static inline int irand(int a, int b){ // inclusive
  return a + (int)(esp_random() % (uint32_t)(b - a + 1));
}

// ---- Cute/Vector-style chatter generator ----
static const int PENTA_DEG[] = {0, 2, 4, 7, 9, 12}; // major pentatonic degrees
static inline int pick(const int* arr, int n){ return irand(0, n-1) + 0, arr[irand(0, n-1)]; }

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
  static NoteEv ev[32];
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

// removed: old droidSpeak generator
} // namespace Audio
