 # DogePet Hardware Setup

## Overview

DogePet is an ESP32-S3 based companion robot with animated eyes, audio feedback, haptic response, and motion sensing capabilities.

---

## Microcontroller

| Parameter | Value |
|-----------|-------|
| **MCU** | ESP32-S3 Super Mini |
| **Flash** | 4MB |
| **PSRAM** | QSPI (2MB) |
| **USB** | Native USB-C |
| **Voltage** | 3.3V logic |

---

## Pin Configuration

### GPIO Pinout Table

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| 5 | I2C_SCL | Output | 400kHz Fast Mode |
| 6 | I2C_SDA | Bidirectional | Shared bus (OLED + MPU6050) |
| 2 | MPU_INT | Input | MPU6050 interrupt (optional) |
| 11 | I2S_DI | Input | Microphone data (INMP441) |
| 15 | VBAT_PIN | Input | Battery voltage ADC |
| 16 | I2S_LRC | Output | Audio Word Select (LRCLK) |
| 17 | I2S_BCLK | Output | Audio Bit Clock |
| 33 | I2S_DO | Output | Audio Data Out (MAX98357A) |
| 41 | FUNC_BTN | Input | Head touch sensor (Active HIGH, TPP223) |
| 1 | TOUCH_CHIN | Input | Chin touch sensor (optional, TPP223) |
| 48 | LED_PIN | Output | WS2812 Status LED |
| 3 | VIBRO_RIGHT | Output | Right vibration motor (PWM) ⚠️ Strapping pin |
| 4 | VIBRO_LEFT | Output | Left vibration motor (PWM) |

**Notes:**
- GPIO 3 is a strapping pin - handle during boot sequence
- Touch sensors (TPP223) are Active HIGH - use `INPUT_PULLDOWN`

---

## Peripherals

### Display - OLED SH1106G

| Parameter | Value |
|-----------|-------|
| **Type** | SH1106G OLED |
| **Resolution** | 128 x 64 pixels |
| **Interface** | I2C |
| **Address** | 0x3C |
| **Voltage** | 3.3V |

**Wiring:**
```
OLED        ESP32-S3
────        ────────
VCC    →    3.3V
GND    →    GND
SCL    →    GPIO 5
SDA    →    GPIO 6
```

---

### IMU - MPU6050 / MPU6500

| Parameter | Value |
|-----------|-------|
| **Type** | MPU6050 or MPU6500 6-axis |
| **Interface** | I2C (shared bus) |
| **Address** | 0x68 |
| **Device ID** | 0x34 (MPU6050) or 0x38/0x39 (MPU6500) |
| **Features** | Accelerometer ±2g, Gyroscope ±250°/s |
| **Polling Rate** | ~25Hz (software polling) |

**Firmware Approach:**
- Uses simple I2C register reads (no DMP library)
- Noise-gated low-pass filtering for stability
- Event detection: Tilt, Shake, Tap, Still
- Calibration at boot (device must be stationary)

**Wiring:**
```
MPU6050     ESP32-S3
───────     ────────
VCC    →    3.3V
GND    →    GND
SCL    →    GPIO 5
SDA    →    GPIO 6
AD0    →    GND (address 0x68)
INT    →    GPIO 2 (optional)
```

**Note:** Some modules are actually MPU6500 variants that report device ID 0x38 instead of 0x34. The firmware handles both.

---

### Audio Output - MAX98357A I2S Amplifier

| Parameter | Value |
|-----------|-------|
| **Type** | MAX98357A I2S DAC + Amplifier |
| **Sample Rate** | 22050 Hz |
| **Bit Depth** | 16-bit |
| **Output** | 3W Class D |

**Wiring:**
```
MAX98357A   ESP32-S3
─────────   ────────
VIN    →    5V (or 3.3V)
GND    →    GND
BCLK   →    GPIO 17
LRC    →    GPIO 16
DIN    →    GPIO 33
GAIN   →    (see gain table below)
SD     →    3.3V (always on) or GPIO for mute
```

**Gain Settings:**
| GAIN Pin | Gain |
|----------|------|
| GND | +9dB |
| Floating | +12dB |
| 3.3V | +15dB |

---

### Microphone - INMP441 (Dual-Channel Hardware)

| Parameter | Value |
|-----------|-------|
| **Type** | INMP441 MEMS Microphone |
| **Configuration** | Dual-channel (hardware stereo) |
| **Sample Rate** | 22050 Hz |
| **Interface** | I2S (single data line) |

**Wiring (Stereo Pair):**
```
INMP441 Left    INMP441 Right   ESP32-S3
────────────    ─────────────   ────────
VDD        →    VDD        →    3.3V
GND        →    GND        →    GND
SD         ────┬──────────────→ GPIO 11 (shared)
WS         →    WS         →    GPIO 16 (shared with audio LRC)
SCK        →    SCK        →    GPIO 17 (shared with audio BCLK)
L/R        →    GND             (Left channel select)
            L/R        →    3.3V (Right channel select)
```

**Note:** Both microphones share the same I2S bus with the audio output. The L/R pin determines which channel each mic outputs on.

---

### Status LED - WS2812

| Parameter | Value |
|-----------|-------|
| **Type** | WS2812 (NeoPixel) |
| **Count** | 1 LED |
| **Brightness** | 60 (0-255) |

**Wiring:**
```
WS2812      ESP32-S3
──────      ────────
VCC    →    3.3V or 5V
GND    →    GND
DIN    →    GPIO 48
```

---

### Vibration Motors

| Parameter | Value |
|-----------|-------|
| **Type** | Coin/ERM vibration motor |
| **Count** | 2 (Left + Right) |
| **Control** | PWM via LEDC (analogWrite) |
| **GPIO Left** | 4 |
| **GPIO Right** | 3 (strapping pin - handle with care) |

**Wiring (with N-channel MOSFET or transistor):**
```
Motor Driver    ESP32-S3
────────────    ────────
Left Gate  →    GPIO 4
Right Gate →    GPIO 3
```

**Note:** Do NOT connect motors directly to GPIO! Use a transistor (2N2222, 2N7000) or motor driver (DRV8833) with flyback diode.

**Haptic Patterns (Non-Blocking, 80-100% PWM Range: 204-255):**
- **TAP (click)**: Single pulse with ramp-up (50ms) for snappy feedback - 80-100% intensity
- **DOUBLE-TAP (doubleClick)**: Heartbeat rhythm pattern
  - LUB: 60ms @ 95% intensity (PWM 242)
  - Gap: 50ms silence
  - DUB: 90ms @ 100% intensity (PWM 255)
  - Total: ~200ms for romantic heartbeat feel
- **ALARM (alarm)**: Alternating left-right urgent pattern
  - 6 pulses (left-right-left-right-left-right) × 80ms = 480ms total
  - 100% intensity (PWM 255) for urgency
- **PURR**: Continuous 5-phase rhythmic cycle (non-blocking)
  - Phase 0: 120ms @ 85% (PWM 217)
  - Phase 1: 100ms @ 100% (PWM 255)
  - Phase 2: 110ms @ 85% (PWM 217)
  - Phase 3: 90ms @ 80% (PWM 204)
  - Phase 4: 130ms @ 85% (PWM 217)
  - Loops continuously while petting for natural cat-like feel

**Performance:**
- All patterns use phase-based state machine (no blocking delays)
- Main loop overhead: ~1ms per iteration
- Intensity controlled dynamically via `Settings::haptic.intensity` (0-255 → 204-255 PWM)

---

### Button / Touch Sensor

| Parameter | Value |
|-----------|-------|
| **Type** | TPP223 Capacitive Touch or Momentary Button |
| **Logic** | Active HIGH (HIGH when touched/pressed) |
| **GPIO** | 41 |
| **Debounce** | 30ms (software) |
| **Features** | Tap detection, Hold/Petting detection |

**Wiring:**
```
TPP223      ESP32-S3
──────      ────────
VCC    →    3.3V
GND    →    GND
I/O    →    GPIO 41
```

**Touch Events (All with Haptic + Sound Feedback):**
- **Tap** (< 300ms): Eye blink + haptic TAP (50ms ramp-up) + tap sound
- **Hold** (> 400ms): Triggers "petting" mode with happy eyes + haptic PURR (80-100% intensity) + happy sound
- **Hold End**: Haptic purr stops + content sound

**Optional Chin Touch:**
A second touch sensor can be added on GPIO 1. Enable with `#define TOUCH_CHIN_ENABLED` in `config.h`.

**Chin Behaviors (All with Haptic + Sound Feedback):**
- **Chin Tap**: Playful wink + haptic TAP (50ms ramp-up) + chirp sound
- **Chin Hold (scratches)**: Blissfully closes eyes + haptic PURR (80-100% intensity) + purr sound
- **Chin Hold End**: Eyes open + haptic purr stops + satisfied sound

**Combo Expression (Head + Chin simultaneously):**
- **Both Held Together**: **CONFUSED FACE** 😕
  - Eyes show confused mood
  - Horizontal eye flicker animation
  - Haptic DOUBLE-TAP heartbeat rhythm (200ms)
  - Surprise beep sound effect

---

### Battery Monitoring

| Parameter | Value |
|-----------|-------|
| **Type** | Voltage divider to ADC |
| **ADC Pin** | GPIO 15 |
| **Range** | 3.2V - 4.05V (Li-Po) |
| **Calibration** | 1.0518 multiplier (`VBAT_CAL`) |
| **ADC Resolution** | 12-bit (0-4095) |
| **ADC Attenuation** | 11dB (up to ~3.6V on pin before divider) |
| **Sampling** | Averaged over `VBAT_SAMPLES` (12 samples) |
| **Read Interval** | Every `VBAT_READ_INTERVAL_MS` (1 second) |
| **Log Interval** | Every `VBAT_LOG_INTERVAL_MS` (30 seconds) |
| **Warnings** | Low battery at `LOW_BATT_WARN`% (15%), Critical at `CRIT_BATT_WARN`% (5%) |

**Voltage Divider (for 4.2V Li-Po):**
```
VBAT ──┬── R1 (10kΩ) ──┬── R2 (10kΩ) ── GND
       │                │
       │                └── GPIO 15 (ADC)
       └── To charge controller
```

**Calculation:**
```
V_battery = (ADC_raw / 4095) × 3.3V × 2.0 × VBAT_CAL
```

**Percentage Calculation:**
```
percent = ((V_battery - VBAT_MIN_V) / (VBAT_MAX_V - VBAT_MIN_V)) × 100
```

**Features:**
- **Cached Readings**: Voltage and percentage are cached and updated every 1 second (`VBAT_READ_INTERVAL_MS`) to avoid excessive ADC reads
- **Automatic Logging**: Battery status is automatically logged to serial every 30 seconds (`VBAT_LOG_INTERVAL_MS`) in JSON format:
  ```json
  {"status":"data","type":"power","voltage":3.85,"percent":72,"state":0}
  ```
- **Efficient**: Uses averaging (12 samples per reading) for stable readings with minimal delay
- **Configurable**: All thresholds, intervals, and calibration values defined in `config.h`:
  - `VBAT_MIN_V`: Minimum battery voltage (3.2V default)
  - `VBAT_MAX_V`: Maximum battery voltage (4.05V default)
  - `VBAT_CAL`: Calibration multiplier (1.0518 default)
  - `VBAT_SAMPLES`: Number of ADC samples to average (12 default)
  - `VBAT_READ_INTERVAL_MS`: Battery reading update interval (1000ms default)
  - `VBAT_LOG_INTERVAL_MS`: Battery status logging interval (30000ms default)

---

## I2C Bus Devices

| Address | Device |
|---------|--------|
| 0x3C | SH1106G OLED Display |
| 0x68 | MPU6050 IMU |

---

## Power Requirements

| Component | Current (typical) | Current (peak) |
|-----------|------------------|----------------|
| ESP32-S3 | 80mA | 350mA (WiFi TX) |
| OLED Display | 10mA | 20mA |
| MAX98357A | 2mA (idle) | 300mA (max volume) |
| INMP441 x2 | 2mA | 2mA |
| WS2812 LED | 1mA | 60mA |
| Vibration Motors | 0mA (off) | 100mA each |
| MPU6050 | 4mA | 4mA |
| **Total** | ~100mA | ~900mA |

**Recommended Battery:** 500mAh+ Li-Po with protection circuit
Currently using a 250mAh battery
---

## Schematic Notes

```
                                    ┌─────────────────┐
                                    │  ESP32-S3       │
                                    │  Super Mini     │
    ┌──────────┐                    │                 │
    │  OLED    │◄──── I2C ─────────►│ GPIO 5,6       │
    │  SH1106  │                    │                 │
    └──────────┘                    │                 │
                                    │                 │
    ┌──────────┐                    │                 │
    │  MPU6050 │◄──── I2C ─────────►│ GPIO 5,6       │
    └──────────┘                    │                 │
                                    │                 │
    ┌──────────┐                    │                 │
    │MAX98357A │◄──── I2S ─────────►│ GPIO 16,17,33  │
    │  Speaker │                    │                 │
    └──────────┘                    │                 │
                                    │                 │
    ┌──────────┐                    │                 │
    │ INMP441  │───── I2S ─────────►│ GPIO 11        │
    │  Stereo  │      (shared clk)  │ (clk: 16,17)   │
    └──────────┘                    │                 │
                                    │                 │
    ┌──────────┐                    │                 │
    │  WS2812  │◄──────────────────►│ GPIO 48        │
    └──────────┘                    │                 │
                                    │                 │
    ┌──────────┐                    │                 │
    │  Button  │────────────────────│ GPIO 41        │
    └──────────┘                    │                 │
                                    │                 │
    ┌──────────┐                    │                 │
    │ Vibro L  │◄───────────────────│ GPIO 4         │
    │ Vibro R  │◄───────────────────│ GPIO 3         │
    └──────────┘                    │                 │
                                    │                 │
    ┌──────────┐                    │                 │
    │ Battery  │────── ADC ────────►│ GPIO 15        │
    │ Monitor  │                    │                 │
    └──────────┘                    └─────────────────┘
```

---

## Future Expansion (Reserved)

| Feature | Notes |
|---------|-------|
| SD Card | SPI pins available (GPIO 2,3,4,8) - currently used by vibro motors |
| Touch Sensors | Any capacitive-capable GPIO |
| Additional I2C | Shared bus supports multiple devices |
| WiFi/BLE | Built-in ESP32-S3 radio |

---

## Configuration File

All hardware parameters are defined in `config.h` for easy modification and future web-based configuration:

```cpp
// Key hardware pins
I2C_SDA = 6, I2C_SCL = 5
FUNC_BTN = 41 (Active HIGH)
I2S_BCLK = 17, I2S_LRC = 16, I2S_DO = 33
I2S_DI = 11
LED_PIN = 48
VBAT_PIN = 15
VIBRO_LEFT = 4, VIBRO_RIGHT = 3
```
