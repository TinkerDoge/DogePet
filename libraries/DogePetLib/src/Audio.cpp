// Audio.cpp - I2S audio for mic input and speaker output
#include "Audio.h"
#include <driver/i2s.h>
#include <math.h>

// I2S Configuration
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE 22050
#define I2S_BUFFER_LEN 64

bool Audio::isI2SReady = false;
uint8_t Audio::volume = DEFAULT_AUDIO_VOLUME;

// Mic change detection state
bool Audio::micLogEnabled = true;
int Audio::lastMicDB = 0;
uint32_t Audio::lastMicLogMs = 0;

void Audio::init() {
    initI2S();
    // Set initial volume from config.h
    volume = DEFAULT_AUDIO_VOLUME;
    
    Serial.println("{\"status\":\"info\",\"msg\":\"Audio: I2S OK\"}");
}

void Audio::initI2S() {
    if (isI2SReady) return;
    
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = I2S_BUFFER_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DO,
        .data_in_num = I2S_DI
    };
    
    if (i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL) == ESP_OK) {
        i2s_set_pin(I2S_PORT, &pin_config);
        isI2SReady = true;
    } else {
        Serial.println("{\"status\":\"error\",\"msg\":\"I2S Init Failed\"}");
    }
}

int Audio::readMicDB() {
    if (!isI2SReady) return 0;
    
    int32_t samples[I2S_BUFFER_LEN * 2];
    size_t bytes_read = 0;
    
    i2s_read(I2S_PORT, &samples, sizeof(samples), &bytes_read, 0);
    
    if (bytes_read > 0) {
        int64_t sum = 0;
        int count = 0;
        
        for (int i = 0; i < (int)(bytes_read / 4); i += 2) {
            int32_t valL = samples[i] >> 8; 
            int32_t valR = samples[i+1] >> 8;
            sum += ((int64_t)valL * valL) + ((int64_t)valR * valR);
            count += 2;
        }
        
        if (count == 0) return 0;
        
        float rms = sqrt(sum / count);
        if (rms < 1) rms = 1;
        float db = 20 * log10(rms);
        return (int)db;
    }
    return 0;
}

void Audio::testMicLog() {
    int db = readMicDB();
    Serial.printf("{\"status\":\"data\",\"type\":\"mic\",\"db\":%d}\n", db);
}

void Audio::setMicLogEnabled(bool enabled) {
    micLogEnabled = enabled;
}

void Audio::update() {
    if (!isI2SReady) return;
    
    int db = readMicDB();
    uint32_t now = millis();
    
    bool changed = abs(db - lastMicDB) >= MIC_CHANGE_THRESHOLD_DB;
    bool canLog = (now - lastMicLogMs) >= MIN_MIC_LOG_INTERVAL_MS;
    
    if (micLogEnabled && changed && canLog) {
        Serial.printf("{\"type\":\"mic\",\"db\":%d}\n", db);
        lastMicDB = db;
        lastMicLogMs = now;
    }
}

void Audio::playTone(uint32_t freq, uint32_t durationMs, WaveType type, uint32_t modFreq, WaveEffect effect) {
    if (!isI2SReady) return;
    
    size_t bytes_written;
    int samples = (I2S_SAMPLE_RATE * durationMs) / 1000;
    int32_t buffer[64]; 
    
    // Oscillator 1 (Main)
    uint32_t period1 = I2S_SAMPLE_RATE / (freq > 0 ? freq : 1);
    uint32_t halfPeriod1 = period1 / 2;
    static uint32_t counter1 = 0;

    // Oscillator 2 (Modulator)
    uint32_t period2 = (modFreq > 0) ? (I2S_SAMPLE_RATE / modFreq) : 0;
    uint32_t halfPeriod2 = period2 / 2;
    static uint32_t counter2 = 0;

    int16_t maxAmp = 20000; // Base amplitude

    for (int i = 0; i < samples; i += 32) {
        for (int j = 0; j < 64; j+=2) {
            int16_t sample1 = 0;
            int16_t sample2 = 0;
            int16_t finalSample = 0;

            // --- OSCILLATOR 1 ---
            if (type == WAVE_SQUARE) {
                int16_t amp = (int16_t)((volume / 100.0f) * maxAmp);
                sample1 = ((counter1 % period1) < halfPeriod1) ? amp : -amp;
            } else if (type == WAVE_NOISE) {
                int16_t amp = (int16_t)((volume / 100.0f) * (maxAmp * 0.75));
                sample1 = (rand() % (2 * amp)) - amp;
            }

            // --- OSCILLATOR 2 (Only if needed) ---
            if (modFreq > 0 && period2 > 0) {
                int16_t amp = (int16_t)((volume / 100.0f) * maxAmp);
                // Osc 2 is always Square for now (simple modulator)
                sample2 = ((counter2 % period2) < halfPeriod2) ? amp : -amp;
            }

            // --- MIXING / EFFECTS ---
            if (modFreq > 0 && effect == EFFECT_MIX) {
                // Additive: Average the two signals
                finalSample = (sample1 + sample2) / 2;
            } else if (modFreq > 0 && effect == EFFECT_RING) {
                // Ring Mod: Multiply (Simulates XOR logic behavior of retro hardware ring mods roughly)
                // Mathematical Ring Mod: s1 * s2 -> output amplitude needs scaling
                // Logic XOR (1-bit style): if signs differ, output high? 
                // Let's stick to multiplication logic but keep ranges safely within 16-bit
                
                // Scale down before multiply to avoid 32-bit overflow when casting back to 16
                // sample1 is +/- 20000. s1*s2 is +/- 400,000,000. Fits in int32.
                int32_t product = (int32_t)sample1 * sample2; 
                // Normalize back to amplitude range. maxAmp * maxAmp = 400M. We want output maxAmp aka 20000.
                // Divide by maxAmp.
                finalSample = (int16_t)(product / maxAmp); 
            } else {
                // No effect or no modulator
                finalSample = sample1;
            }

            counter1++;
            if (modFreq > 0) counter2++;
            
            int32_t s32 = finalSample << 16; 
            buffer[j] = s32;
            buffer[j+1] = s32;
        }
        i2s_write(I2S_PORT, buffer, sizeof(buffer), &bytes_written, portMAX_DELAY);
    }
}

void Audio::playSizzle() {
    playTone(0, 150, WAVE_NOISE);
}

void Audio::speak(Mood mood) {
    int count = 0;
    
    switch(mood) {
        case MOOD_HAPPY:
            // Fast, rising, HARMONIOUS (Major chords)
            count = 4 + (rand() % 4); 
            for(int i=0; i<count; i++) {
                int f = 800 + (rand() % 800) + (i*150);
                int d = 40 + (rand() % 40);
                
                // Add 5th (x1.5) or Octave (x2) harmony
                int modRatio = (rand() % 2 == 0) ? 2 : 3; // 2=octave(2/1), 3=fifth(3/2 approx) - wait integer math
                // Simple ratios: 1.5 is difficult with int. Let's do: f*3/2
                int f2 = (modRatio == 2) ? f*2 : (f*3)/2; 
                
                playTone(f, d, WAVE_SQUARE, f2, EFFECT_MIX);
                delay(10 + (rand() % 20));
            }
            break;
            
        case MOOD_SAD:
            // Slow, descending, simple tones (no aggressive mod)
            count = 2 + (rand() % 2); 
            for(int i=0; i<count; i++) {
                int f = 600 - (i * 150) - (rand() % 100);
                if (f < 100) f = 100;
                int d = 150 + (rand() % 100); 
                playTone(f, d, WAVE_SQUARE); // Pure tone for sadness
                delay(50);
            }
            break;
            
        case MOOD_CURIOUS:
            // "Whistle" slide up
            count = 2 + (rand() % 3);
            for(int i=0; i<count; i++) {
                int startF = 800 + (rand() % 400);
                int endF = startF + 400 + (rand() % 400);
                int steps = 10;
                int stepDur = 5;
                for(int s=0; s<steps; s++) {
                    int currF = startF + ((endF - startF) * s / steps);
                    // Add slight detuned phasing for sci-fi whistle texture
                    playTone(currF, stepDur, WAVE_SQUARE, currF + 5, EFFECT_MIX);
                }
                delay(20);
            }
            break;
            
        case MOOD_ANGRY:
            // Fast, RING MODULATED (Metallic/Harsh)
            count = 5 + (rand() % 5);
            for(int i=0; i<count; i++) {
                if (rand() % 3 == 0) {
                    playTone(0, 40, WAVE_NOISE); 
                } else {
                    int f = 1000 + (rand() % 1000);
                    // Ring mod with discordant frequency (e.g. f * 1.33)
                    int f2 = f + 333; 
                    playTone(f, 30, WAVE_SQUARE, f2, EFFECT_RING);
                }
                delay(5);
            }
            break;
            
        case MOOD_NEUTRAL:
        default:
            count = 3 + (rand() % 3);
            for(int i=0; i<count; i++) {
                int f = 500 + (rand() % 1000);
                int d = 40 + (rand() % 60);
                // Slight Mix for body
                playTone(f, d, WAVE_SQUARE, f/2, EFFECT_MIX);
                delay(30);
            }
            break;
    }
}

void Audio::playMelody() {
    // Startup "Power On" Arpeggio
    playTone(880, 80, WAVE_SQUARE);  // A5
    playTone(1108, 80, WAVE_SQUARE); // C#6
    playTone(1318, 80, WAVE_SQUARE); // E6
    playTone(1760, 120, WAVE_SQUARE); // A6
}

void Audio::chirp() {
    // Quick rising slide
    for(int f = 800; f < 1600; f+=200) {
        playTone(f, 20, WAVE_SQUARE); 
    }
}

void Audio::purrrSound() {
    // Low rumble with noise bursts
    for(int i=0; i<3; i++) {
        playTone(60, 60, WAVE_SQUARE);
        playTone(0, 30, WAVE_NOISE);
    }
}

void Audio::surpriseBeep() {
    // "!" Alert Sound - Rapid high arpeggio
    playTone(1800, 50, WAVE_SQUARE);
    playTone(2200, 50, WAVE_SQUARE);
    playTone(1800, 50, WAVE_SQUARE);
}

void Audio::yawn() {
    // Pitch down slide
    for(int f = 800; f > 200; f-=100) {
        playTone(f, 60, WAVE_SQUARE);
    }
}

void Audio::tapSound() {
    // Short high blip
    playTone(1200, 30, WAVE_SQUARE);
}

void Audio::happySound() {
    // "Coin" / 1-Up style sound
    playTone(1047, 50, WAVE_SQUARE); // C6
    playTone(1318, 50, WAVE_SQUARE); // E6
    playTone(1568, 50, WAVE_SQUARE); // G6
    playTone(2093, 80, WAVE_SQUARE); // C7
}

void Audio::contentSound() {
    // Gentle 2-tone descent
    playTone(1500, 60, WAVE_SQUARE);
    playTone(1000, 80, WAVE_SQUARE);
}

void Audio::satisfiedSound() {
    // Short purr burst
    playTone(60, 40, WAVE_SQUARE);
    playTone(0, 40, WAVE_NOISE);
    playTone(60, 40, WAVE_SQUARE);
}

void Audio::stop() {
    // Silence
}

void Audio::setVolume(uint8_t vol) {
    volume = vol > 100 ? 100 : vol;
}

uint8_t Audio::getVolume() {
    return volume;
}
