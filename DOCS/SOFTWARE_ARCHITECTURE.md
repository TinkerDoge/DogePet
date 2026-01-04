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
*   **Sound Effects**:
    *   `chirp()`: Playful two-tone chirp (800Hz → 1200Hz).
    *   `purrrSound()`: Content purring sound (descending: 600Hz → 500Hz → 400Hz).
    *   `surpriseBeep()`: Surprised ascending beep (400Hz → 600Hz → 900Hz → 1200Hz).
    *   `yawn()`: Tired yawn sound (descending: 800Hz → 600Hz → 400Hz → 300Hz).
*   **Touch Interaction Sounds**:
    *   `tapSound()`: Gentle tap feedback - short pleasant beep (600Hz → 800Hz).
    *   `happySound()`: Happy/content sound for petting start - ascending pleasant tones (500Hz → 600Hz → 700Hz).
    *   `contentSound()`: Satisfied sound for petting end - gentle descending sigh (600Hz → 500Hz → 400Hz).
    *   `satisfiedSound()`: Gentle satisfied sound for chin scratch end - soft content purr (450Hz → 400Hz).

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
*   **Intensity Range**: All haptic patterns use 80-100% intensity (PWM 204-255) optimized for weaker motors.
*   **Key Functions**:
    *   `init()`: Configures LEDC PWM for motor control.
    *   `buzz(left, right, durationMs)`: Direct motor control.
    *   `click()`: Short tactile feedback (90% intensity, 60ms pulse).
    *   `doubleClick()`: Heartbeat pattern - lub-DUB rhythm (82-100% intensity).
    *   `alarm()`: Urgent alternating left/right pattern (90% intensity).
    *   `startPurr()` / `stopPurr()`: Cat-like rhythmic vibration (starts at 85% intensity).
    *   `purrTick()`: Non-blocking purr pattern update (call in loop) - 5-phase cycle with 80-100% intensity.
*   **Purr Pattern**: 5-phase rhythmic cycle (medium → strong → medium → soft → medium) with varied timing (90-130ms) for natural cat-like feel.

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

### 8. Events Module (`Events.h` / `Events.cpp`)
**Responsibility**: Central event handler that coordinates touch interactions with haptic and audio feedback.

*   **Dependencies**: `Face`, `Touch`, `Haptics`, `Audio`, `Power`.
*   **Key Functions**:
    *   `init()`: Initializes event state.
    *   `update()`: Processes touch events and triggers appropriate responses (call in loop after `Touch::update()`).
*   **Event Handling**: All touch interactions trigger both haptic and sound feedback for a more alive, responsive feel.
*   **Behaviors**:
    *   Head tap → blink + click haptic + tap sound
    *   Head hold start → happy eyes + purr haptic + happy sound
    *   Head hold end → normal eyes + stop purr + content sound
    *   Chin tap → wink + click haptic + chirp sound
    *   Chin hold start → closed eyes + purr haptic + purr sound
    *   Chin hold end → open eyes + stop purr + satisfied sound
    *   Combo (head + chin) → confused + double-click haptic + surprise beep

### 9. LED Module (`LED.h` / `LED.cpp`)
**Responsibility**: Controls the WS2812 status LED.

*   **Hardware**: WS2812 NeoPixel (GPIO 48).
*   **Key Functions**:
    *   `init()`: Initializes LED driver.
    *   `on()` / `off()`: Simple on/off control.
    *   `setColor(r, g, b)`: Set RGB color.
    *   `setBrightness(level)`: Set brightness (0-255).

---

## Main Loop Update Order

**Critical**: The update order in `loop()` is important for proper event handling:

```cpp
void loop() {
    // 1. Update sensors and input processing FIRST
    Touch::update();        // Process touch input, generate events
    
    // 2. Handle events (must be after Touch::update)
    Events::update();       // Process touch events → haptic + sound feedback
    
    // 3. Update non-blocking systems
    Haptics::purrTick();    // Update purr pattern (if active)
    Motion::update();       // Update motion sensor
    Power::update();        // Update power management
    
    // 4. Update display and animations
    Face::update();         // Update eye animations
    Animation::tick();      // Update behavior tree
}
```

**Why this order matters:**
- `Touch::update()` must run before `Events::update()` because Events reads the events that Touch generates.
- `Haptics::purrTick()` must be called every loop to maintain the non-blocking purr pattern.

---

## Behaviors

### Touch Interactions (All with Haptic + Sound Feedback)

**Head Touch:**
- **Tap**: Eye blink + haptic click (90% intensity) + tap sound
- **Hold Start (Petting)**: Happy eyes + haptic purr starts (85% intensity) + happy sound
- **Holding**: Continuous purr pattern (80-100% intensity, 5-phase cycle) + occasional blinks
- **Hold End**: Eyes return to normal + haptic purr stops + content sound

**Chin Touch (Optional):**
- **Tap**: Wink + haptic click (90% intensity) + chirp sound
- **Hold Start (Scratching)**: Eyes close + haptic purr starts (85% intensity) + purr sound
- **Holding**: Continuous purr pattern + eyes open/close rhythmically
- **Hold End**: Eyes open + haptic purr stops + satisfied sound

**Combo (Head + Chin):**
- **Both Held**: Confused happy face + haptic double-click heartbeat (82-100% intensity) + surprise beep

### Petting Behavior
When the user holds the touch button (simulating petting the bot's head):
1. Eyes become happy (`eyes->happy = true`).
2. Haptic purr pattern starts (`Haptics::startPurr()`) with happy sound.
3. Continuous rhythmic purr (5-phase cycle, 80-100% intensity).
4. Occasional blinks while being pet.
5. On release, haptic purr stops with content sound, eyes return to normal.

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
│           ├── config.h         # Compile-time defaults
│           ├── Settings.h/.cpp  # Runtime settings (NVS)
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

## Settings System

DogePet uses a two-tier configuration system:

### 1. Compile-Time Defaults (`config.h`)
Static constants defined at compile time. These serve as factory defaults.

### 2. Runtime Settings (`Settings.h/.cpp`)
Stored in ESP32 NVS (Non-Volatile Storage). Loaded at boot, can be modified via serial commands.

### Settings Categories

| Category | Type | When Applied | Description |
|----------|------|--------------|-------------|
| **Face** | Dynamic | Immediately | Eye size, shape, animation timing, contrast |
| **Audio** | Dynamic | Immediately | Volume, mic logging |
| **Haptic** | Dynamic | Immediately | Vibration intensity |
| **LED** | Dynamic | Immediately | Brightness, RGB color |
| **Motion** | Persistent | After reboot | Tilt, shake, tap thresholds |
| **Power** | Persistent | After reboot | Idle/sleep timeouts |
| **Pins** | Persistent | After reboot | GPIO assignments |

### Settings API

```cpp
// At boot (in setup)
Settings::begin();  // Load from NVS (or defaults)

// Apply dynamic settings to modules
Face::applySettings();   // Updates eye parameters live

// Change a setting
Settings::face.width = 32;
Settings::save();        // Save to NVS

// Check if reboot needed
if (Settings::pendingReboot) {
    Serial.println("Reboot required");
}

// Reset to factory defaults
Settings::resetDefaults();
```

### Display Mode Control

The Face module supports multiple display modes to prevent RoboEyes from interfering with other content:

```cpp
// Show custom content (disables eyes)
Face::setMode(DisplayMode::Custom);
Face::getDisplay()->clearDisplay();
Face::getDisplay()->println("Hello!");
Face::refresh();

// Resume eyes
Face::setMode(DisplayMode::Eyes);
```

**Display Modes:**
- `Eyes` - Normal RoboEyes animation
- `Sleep` - Closed eyes with Zzz animation
- `Toast` - Notification overlay (eyes paused)
- `Custom` - User draws directly
- `Off` - Display blank

---

## Configuration Defaults (`config.h`)

All default values are defined in `config.h`:

| Category | Examples |
|----------|----------|
| **Hardware Pins** | `I2C_SDA`, `I2C_SCL`, `FUNC_BTN`, `LED_PIN` |
| **Display** | `SCREEN_W`, `SCREEN_H`, `SCREEN_ADDR` |
| **Eye Appearance** | `DEFAULT_EYE_WIDTH`, `DEFAULT_EYE_HEIGHT`, `DEFAULT_EYE_SPACING` |
| **Audio** | `AUDIO_SAMPLE_RATE`, `DEFAULT_AUDIO_VOLUME` |
| **Motion Thresholds** | `DEFAULT_TILT_THRESHOLD_DEG`, `DEFAULT_SHAKE_ANGRY_DPS` |
| **Power/Battery** | `VBAT_MIN_V`, `VBAT_MAX_V`, `DEFAULT_IDLE_TIMEOUT_MS` |
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
    
    Settings::begin();  // Load settings from NVS
    
    Motion::init();
    Face::init();       // Uses Settings::face for eye params
    Touch::init();
    // ... other modules
}

void loop() {
    Motion::Event e = Motion::update();
    if (e == Motion::Event::Shake) {
        // React to shake
    }
    
    // IMPORTANT: Touch::update() must be called BEFORE Events::update()
    // Touch processes input and generates events, Events handles them
    Touch::update();
    Events::update();  // Handles touch events (haptic + sound feedback)
    Haptics::purrTick();  // Update non-blocking purr pattern
    
    Face::update();
}
```
