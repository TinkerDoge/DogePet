# DogePet Software Architecture

## Overview
The DogePet firmware uses a **library-based modular architecture** for improved maintainability, testability, and code reuse. All modules are packaged in the `DogePetLib` Arduino library located in `libraries/DogePetLib/`.

The main sketch (`DogePet.ino`) acts as the orchestrator, including a single header (`<DogePetLib.h>`) to access all modules. It initializes modules in sequence, runs a boot test, and coordinates global events.

---

## Library Structure

### DogePetLib (`libraries/DogePetLib/`)

The library contains all hardware modules with a single entry point:

```cpp
#include <DogePetLib.h>  // Includes all modules
```

**Library Files:**
- `DogePetLib.h` - Main include header
- `config.h` - Centralized configuration (pins, thresholds, timing)
- `mpu6050.h` - Simple inline I2C helper for MPU6050
- `Motion.h/.cpp` - Motion sensing with event detection
- `Face.h/.cpp` - OLED display and eye animations
- `Audio.h/.cpp` - I2S mic input and speaker output
- `Haptics.h/.cpp` - Vibration motor control
- `Touch.h/.cpp` - Debounced touch input
- `Power.h/.cpp` - Battery monitoring and sleep management
- `LED.h/.cpp` - WS2812 NeoPixel control
- `Events.h/.cpp` - Touch → behavior handler
- `Animation.h/.cpp` - High-level animation sequences
- `FluxGarage_RoboEyes.h` - Eye animation library

---

## Core Modules

### 1. Face Module (`Face.h` / `Face.cpp`)
**Responsibility**: Manages the OLED display and the procedural eye animations.

*   **Hardware**: SH1106G OLED (I2C 0x3C).
*   **Libraries**: `Adafruit_SH110X`, `FluxGarage_RoboEyes`.
*   **Key Functions**:
    *   `init()`: Initializes OLED and Eye objects.
    *   `showBootScreen(msg)`: Displays a splash screen with status text.
    *   `updateProgressBar(percent, status)`: Renders a loading bar during boot.
    *   `playLineAnimation()`: Runs a CRT-style line scan test.
    *   `update()`: Called in the main loop to refresh eye animations.
    *   `showSleepFace()`: Displays closed eyes with animated Zzz.
    *   `showDimFace()`: Displays tired/droopy eyes.
    *   `showActiveFace()`: Returns to normal animated eyes.

### 2. Motion Module (`Motion.h` / `Motion.cpp`)
**Responsibility**: High-level motion interpreter with event detection.

*   **Hardware**: MPU6050/MPU6500 (I2C 0x68).
*   **Approach**: Simple I2C polling with noise-gated low-pass filtering.
*   **Key Static Functions** (for compatibility):
    *   `Motion::init()`: Initialize global instance.
    *   `Motion::update()`: Poll sensor, return event.
    *   `Motion::isReady()`: Check if initialized.
    *   `Motion::calibrate()`: Recalibrate sensor.
*   **Motion Events**:
    *   `None`: No significant motion.
    *   `Tilt`: Gentle tilt beyond threshold.
    *   `Shake`: Brief shake detected.
    *   `FuriousShake`: Sustained hard shake.
    *   `Tap`: Single tap detected.
    *   `Still`: Device returned to stationary.
*   **Configuration** (from `config.h`):
    *   `TILT_THRESHOLD_DEG`: Tilt detection (default 20°).
    *   `SHAKE_ANGRY_DPS`: Shake threshold (default 200 dps).
    *   `SHAKE_FURIOUS_DPS`: Furious shake (default 280 dps).

### 3. Audio Module (`Audio.h` / `Audio.cpp`)
**Responsibility**: Manages the I2S subsystem for both Input (Microphone) and Output (Amplifier).

*   **Hardware**: 
    *   **Input**: INMP441 MEMS Microphone (I2S Standard).
    *   **Output**: MAX98357A Class-D Amp (I2S Standard).
*   **Driver**: ESP32 I2S Driver (via `driver/i2s.h`).
*   **Key Functions**:
    *   `init()`: Installs the I2S driver in full-duplex mode.
    *   `readMicDB()`: Reads audio samples and calculates approximate dB level.
    *   `playTone(freq, duration)`: Synthesizes a sine wave and pushes it to the I2S buffer.
    *   `playMelody()`: Plays the startup jingle (C5-E5-G5-C6).
    *   `update()`: Monitors mic level and logs only when dB changes.
    *   `setMicLogEnabled(bool)`: Enable/disable mic logging.

### 4. Animation Module (`Animation.h` / `Animation.cpp`)
**Responsibility**: Orchestrates high-level behaviors and command sequences.

*   **Dependencies**: `Face`, `Audio`, `Motion`.
*   **Key Functions**:
    *   `init()`: Prepares behavior state logic.
    *   `playStartupSequence()`: Runs the "Wake Up" sequence (Blink -> Laugh -> Happy).
    *   `tick()`: Main behavior tree update (called in loop).

### 5. Power Module (`Power.h` / `Power.cpp`)
**Responsibility**: Battery monitoring and power state management (sleep/wake).

*   **Hardware**: Battery voltage via ADC (GPIO 15).
*   **Key Functions**:
    *   `init()`: Configures ADC and initializes state.
    *   `update()`: Manages power state transitions (called in loop).
    *   `getVoltage()`: Returns battery voltage.
    *   `getPercent()`: Returns battery percentage (0-100).
    *   `onActivity()` / `onMotion()` / `onLoudNoise()`: Activity tracking for sleep.
    *   `isSleeping()`: Returns true if in sleep mode.
    *   `wake()` / `sleep()`: Force state changes.
*   **Power States**:
    *   `ACTIVE`: Normal operation.
    *   `DIM`: Screen dimmed after idle timeout (1 min).
    *   `SLEEPING`: Low power mode after sleep timeout (2 min).
*   **Callbacks**: `onSleepCallback`, `onWakeCallback`, `onDimCallback`.

### 6. Haptics Module (`Haptics.h` / `Haptics.cpp`)
**Responsibility**: Controls vibration motors for haptic feedback.

*   **Hardware**: 2x ERM vibration motors (GPIO 3, 4) via PWM.
*   **Key Functions**:
    *   `init()`: Configures LEDC PWM for motor control.
    *   `buzz(left, right, durationMs)`: Direct motor control.
    *   `click()`: Short tactile feedback.
    *   `doubleClick()`: Heartbeat pattern.
    *   `alarm()`: Urgent alternating pattern.
    *   `startPurr()` / `stopPurr()`: Cat-like rhythmic vibration.
    *   `purrTick()`: Non-blocking purr pattern update (call in loop).

### 7. Touch Module (`Touch.h` / `Touch.cpp`)
**Responsibility**: Handles touch/button input with debounce and gesture detection.

*   **Hardware**: FUNC_BTN (GPIO 41) - Active HIGH.
*   **Key Functions**:
    *   `init()`: Configures input pins.
    *   `update()`: Processes input and generates events (call in loop).
    *   `getHeadEvent()`: Returns current touch event.
    *   `isHeadPressed()` / `isHeadHeld()`: State queries.
    *   `getHeadHoldDuration()`: Returns hold time in ms.
*   **Touch Events**:
    *   `NONE`: No event.
    *   `TAP`: Quick touch (< 300ms).
    *   `HOLD_START`: Just started holding (> 400ms).
    *   `HOLDING`: Continuously holding.
    *   `HOLD_END`: Just released from hold.
*   **Optional**: Chin touch sensor (compile-time `TOUCH_CHIN_ENABLED`).

### 8. LED Module (`LED.h` / `LED.cpp`)
**Responsibility**: Controls the WS2812 status LED.

*   **Hardware**: WS2812 NeoPixel (GPIO 48).
*   **Key Functions**:
    *   `init()`: Initializes LED driver.
    *   `on()` / `off()`: Simple on/off control.
    *   `setColor(r, g, b)`: Set RGB color.
    *   `setBrightness(level)`: Set brightness (0-255).

---

## Behaviors

### Petting Behavior
When the user holds the touch button (simulating petting the bot's head):
1. Eyes become happy (`eyes->happy = true`).
2. Haptic purr pattern starts (`Haptics::startPurr()`).
3. Occasional blinks while being pet.
4. On release, returns to normal state.

### Sleep Behavior
After periods of inactivity:
1. **1 minute idle**: Enter DIM mode (tired eyes, slower blinks).
2. **2 minutes idle**: Enter SLEEP mode (closed eyes, Zzz animation).
3. Any touch, motion, or loud noise triggers wake.

---

## Boot Sequence

The firmware uses a state machine in `loop()` for boot testing (after `setup()` completes pin/peripheral init):

| Step | State | Action | Visual/Feedback |
| :--- | :--- | :--- | :--- |
| **0** | `BOOT_INIT` | I2C Bus Scan | Splash Screen |
| **1** | `BOOT_DISPLAY_TEST` | Line Animation | "Display Test" Bar (10%) |
| **2** | `BOOT_IMU_TEST` | MPU6050 Calibration | "Calibrate IMU..." Bar (35%) |
| **3** | `BOOT_AUDIO_TEST` | Mic Read + Tone Test | "Mic Test" / "Speaker Test" Bar (60%) |
| **4** | `BOOT_HAPTIC_TEST` | Click + Purr Test | "Haptic Test" Bar (70%) |
| **5** | `BOOT_POWER_TEST` | Battery Read | "Batt: XX%" Bar (80%) |
| **6** | `BOOT_ANIMATION` | Wake Up Sequence | "Wake Up..." Bar (90%) |
| **7** | `BOOT_DONE` | Main Loop Active | "Ready!" (100%) |

---

## Directory Structure

```
DogePet/
├── DogePet.ino              # Main Sketch (uses DogePetLib)
├── include/                 # Font headers only
│   ├── Pixeboy20.h          # Custom fonts
│   ├── ShopeeRegular12.h
│   ├── ToastFont12.h
│   └── ToastRenderer.h
├── libraries/
│   └── DogePetLib/          # All modules packaged as library
│       ├── library.properties
│       └── src/
│           ├── DogePetLib.h     # Single include header
│           ├── config.h         # Pins, thresholds, timing
│           ├── mpu6050.h        # Simple I2C MPU helper
│           ├── Motion.h/.cpp    # Motion events
│           ├── Face.h/.cpp      # OLED + RoboEyes
│           ├── Audio.h/.cpp     # I2S mic + speaker
│           ├── Haptics.h/.cpp   # Vibration motors
│           ├── Touch.h/.cpp     # Touch input
│           ├── Power.h/.cpp     # Battery + sleep
│           ├── LED.h/.cpp       # NeoPixel
│           ├── Events.h/.cpp    # Touch behaviors
│           ├── Animation.h/.cpp # Startup sequences
│           └── FluxGarage_RoboEyes.h
├── companion/               # PC Companion App (Python/Flask)
│   ├── app.py               # Flask server
│   ├── static/              # Web UI (HTML/JS/CSS)
│   └── serial_comm.py       # Serial communication
├── tools/                   # Python utilities
│   ├── font_to_header.py    # TTF → GFX font converter
│   └── png_to_header.py     # PNG → bitmap converter
├── sectors/                 # Experimental refactor (not integrated)
└── DOCS/                    # Documentation
    ├── HARDWARE.md
    ├── SOFTWARE_ARCHITECTURE.md
    └── WebApp_Protocol.md
```

---

## Configuration System

All configurable parameters are centralized in `libraries/DogePetLib/src/config.h`:

| Category | Examples |
|----------|----------|
| **Hardware Pins** | `I2C_SDA`, `I2C_SCL`, `FUNC_BTN`, `LED_PIN` |
| **Display** | `SCREEN_W`, `SCREEN_H`, `SCREEN_ADDR` |
| **Eye Appearance** | `EYE_WIDTH`, `EYE_HEIGHT`, `EYE_SPACING` |
| **Audio** | `AUDIO_SAMPLE_RATE`, `AUDIO_VOLUME` |
| **Motion Thresholds** | `TILT_THRESHOLD_DEG`, `SHAKE_ANGRY_DPS` |
| **Power/Battery** | `VBAT_MIN_V`, `VBAT_MAX_V`, `IDLE_TIMEOUT_MS` |
| **Touch Timing** | `DEBOUNCE_MS`, `TAP_MAX_MS`, `HOLD_MIN_MS` |
| **Debug Flags** | `ENABLE_MOTION_DEBUG`, `DEBUG_SERIAL` |

---

## Usage Example

```cpp
#include <Wire.h>
#include <DogePetLib.h>

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL, 400000);
    
    Motion::init();
    Face::init();
    Touch::init();
    // ... other modules
}

void loop() {
    Motion::Event e = Motion::update();
    if (e == Motion::Event::Shake) {
        // React to shake
    }
    
    Touch::update();
    Face::update();
}
```
