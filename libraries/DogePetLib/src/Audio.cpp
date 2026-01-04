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

void Audio::playTone(uint32_t freq, uint32_t durationMs) {
    if (!isI2SReady) return;
    
    size_t bytes_written;
    int samples = (I2S_SAMPLE_RATE * durationMs) / 1000;
    int32_t buffer[64]; 
    
    float increment = (2.0f * M_PI * freq) / I2S_SAMPLE_RATE;
    float phase = 0;
    
    for (int i = 0; i < samples; i += 32) {
        for (int j = 0; j < 64; j+=2) {
            int16_t amplitude = (int16_t)((volume / 100.0f) * 32000);
            int16_t sample = (int16_t)(sin(phase) * amplitude);
            phase += increment;
            if (phase > 2.0f * M_PI) phase -= 2.0f * M_PI;
            
            int32_t s32 = sample << 16; 
            buffer[j] = s32;
            buffer[j+1] = s32;
        }
        i2s_write(I2S_PORT, buffer, sizeof(buffer), &bytes_written, portMAX_DELAY);
    }
}

void Audio::playMelody() {
    playTone(523, 100); delay(50);
    playTone(659, 100); delay(50);
    playTone(784, 100); delay(50);
    playTone(1047, 300);
}

void Audio::chirp() {
    playTone(800, 40); delay(20);
    playTone(1200, 60);
}

void Audio::purrrSound() {
    playTone(600, 80); delay(30);
    playTone(500, 80); delay(30);
    playTone(400, 100);
}

void Audio::surpriseBeep() {
    playTone(400, 50); delay(30);
    playTone(600, 50); delay(30);
    playTone(900, 50); delay(30);
    playTone(1200, 80);
}

void Audio::yawn() {
    playTone(800, 100); delay(40);
    playTone(600, 120); delay(40);
    playTone(400, 150); delay(40);
    playTone(300, 200);
}

void Audio::tapSound() {
    // Gentle tap sound - short, pleasant beep
    playTone(600, 30); delay(10);
    playTone(800, 40);
}

void Audio::happySound() {
    // Happy/content sound when petting starts - ascending pleasant tones
    playTone(500, 50); delay(20);
    playTone(600, 50); delay(20);
    playTone(700, 60);
}

void Audio::contentSound() {
    // Satisfied sound when petting ends - gentle descending sigh
    playTone(600, 60); delay(30);
    playTone(500, 80); delay(30);
    playTone(400, 100);
}

void Audio::satisfiedSound() {
    // Gentle satisfied sound for chin scratch end - soft content purr
    playTone(450, 50); delay(25);
    playTone(400, 70);
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
