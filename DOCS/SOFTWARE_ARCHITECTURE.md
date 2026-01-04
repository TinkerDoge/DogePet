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

## Core Modules Summary

**Latest Updates (v2.1):**
- **6-Axis Motion Sensing**: Full accelerometer (X, Y, Z) + gyroscope (X, Y, Z) filtering now available
- **Priority-Based Event System**: 4-level priority prevents overlapping behaviors (FURIOUS_SHAKE > COMBO > SHAKE_TAP > TILT)
- **Two-Tier Wake Logic**: DIM mode wakes with normal touch/motion; SLEEP mode only wakes with combo or furious shake
- **State-Change-Only Logging**: Motion and touch events log only on state transitions, eliminating serial spam
- **Animation Timeouts**: Shake auto-resets after 800ms, furious shake confused animation after 500ms
- **Reduced Power Consumption**: SLEEP mode battery reading every 10s without logging

---

## Core Modules

### 1. Face Module (`Face.h` / `Face.cpp`)
**Responsibility**: Manages the OLED display and the procedural eye animations with mood expressions.

*   **Hardware**: SH1106G OLED (I2C 0x3C).
*   **Libraries**: `Adafruit_SH110X`, `FluxGarage_RoboEyes`.
*   **Key Functions**:
    *   `init()`: Initializes OLED and Eye objects with Settings defaults.
    *   `applySettings()`: Applies dynamic face settings (size, radius, contrast, mood flags).
    *   `update()`: Called in main loop to refresh animations.
    *   `setMode(DisplayMode)`: Switch between Eyes, Sleep, Toast, Custom, Off modes.
    *   `showBootScreen(msg)`: Displays a splash screen with status text.
    *   `updateProgressBar(percent, status)`: Renders a loading bar during boot.
    *   `playLineAnimation()`: Runs a CRT-style line scan test.
    *   `showSleepFace()`: Displays closed eyes with animated Zzz.
    *   `showDimFace()`: Displays tired/droopy eyes.
    *   `showActiveFace()`: Returns to normal animated eyes.
*   **Eye Moods** (via RoboEyes flags):
    *   `happy`: Shows happy eyelids (lower eyelids rise up).
    *   `angry`: Shows angry eyelids (top eyelids lower down).
    *   `tired`: Shows tired eyelids (top eyelids droop down).
    *   `curious`: Eyes enlarge when looking left/right (inquisitive look).
    *   `sweat`: Animated sweat drops in corners (anxious/stressed).
*   **Settings Integration**: Loads and applies Settings for eye size, spacing, auto-blink, idle animation, contrast, and mood persistence.

### 2. Motion Module (`Motion.h` / `Motion.cpp`)
**Responsibility**: High-level motion interpreter with 6-axis event detection and state-change logging.

*   **Hardware**: MPU6050/MPU6500 (I2C 0x68) - 3-axis accelerometer + 3-axis gyroscope.
*   **Approach**: Simple I2C polling with noise-gated low-pass filtering on all 6 axes.
*   **Key Static Functions** (for compatibility):
    *   `Motion::init()`: Initialize global instance.
    *   `Motion::update()`: Poll sensor, return event.
    *   `Motion::isReady()`: Check if initialized.
    *   `Motion::calibrate()`: Recalibrate sensor.
    *   `Motion::getGyroMag()`: Get gyro magnitude (dps).
    *   **Sensor Data Accessors** (for event logging):
        *   `Motion::getLastAccelX()`, `getLastAccelY()`, `getLastAccelZ()`: Last filtered acceleration.
        *   `Motion::getLastGyroX()`, `getLastGyroY()`, `getLastGyroZ()`: Last filtered rotation.
    *   `Motion::logMotionStatus()`: Log motion state changes (call in main loop).
    *   `Motion::setEventDebug(bool)`: Toggle motion event JSON logging.
*   **Motion Events** (trigger behavior reactions):
    *   `Tilt`: Gentle tilt beyond threshold → curious eyes (detected via accelerometer angles).
    *   `Shake`: Brief shake detected → happy eyes + blink.
    *   `FuriousShake`: Sustained hard shake → angry + sweat + confused animation (forces wake).
    *   `Tap`: Single tap detected → blink + attention.
    *   `Still`: Device returned to stationary.
*   **State-Change Logging** (minimal serial output):
    *   Only logs when motion state changes: `{"status":"data","type":"motion","state":"still"|"moving"|"shake"|"furious_shake"}`
    *   Prevents spam by logging state transitions, not continuous updates.
    *   Periodic sensor data available via `logMotionStatus()` (not enabled by default).
*   **Power Integration**:
    *   Motion Tap/Tilt/Shake call `Power::onMotion()` → wakes from DIM mode only.
    *   Motion FuriousShake calls `Power::forceWake()` → wakes from any state including SLEEP.
*   **Configuration** (from `config.h`):
    *   `TILT_THRESHOLD_DEG`: Tilt detection (default 20°).
    *   `SHAKE_ANGRY_DPS`: Shake threshold (default 200 dps).
    *   `SHAKE_FURIOUS_DPS`: Furious shake (default 280 dps).
    *   `TAP_SPIKE_DPS`: Tap detection threshold (default 140 dps).
    *   `IMU_TICK_MS`: Sensor polling interval (default 40ms).

### 3. Audio Module (`Audio.h` / `Audio.cpp`)
**Responsibility**: Manages the I2S subsystem for both Input (Microphone) and Output (Amplifier) with a **Polyphonic Retro Engine**.

*   **Hardware**: 
    *   **Input**: INMP441 MEMS Microphone (I2S Standard).
    *   **Output**: MAX98357A Class-D Amp (I2S Standard).
*   **Driver**: ESP32 I2S Driver (via `driver/i2s.h`).
*   **Synthesis Engine**:
    *   **Waveforms**: Square Wave (Retro/Arcade), White Noise (Percussive/FX).
    *   **Polyphony**: Dual-Oscillator architecture (Osc1 + Osc2).
    *   **Effects**:
        *   **Mix**: Additive synthesis for harmonies (e.g., chords).
        *   **Ring Mod**: Multiplicative synthesis for metallic/robotic textures.
*   **Procedural Speech**:
    *   `speak(Mood)`: Generates unique, non-repeating "droid babble" sequences based on emotion.
    *   **Happy**: Fast, rising, harmonious chords (Major intervals).
    *   **Curious**: Sliding pitch "whistles" with phasing effects.
    *   **Angry**: Fast, discordant, ring-modulated noise and square waves.
    *   **Sad**: Slow, descending, pure low tones.
*   **Key Functions**:
    *   `init()`: Installs the I2S driver in full-duplex mode.
    *   `playTone(freq, dur, type, modFreq, effect)`: Core synthesizer function.
    *   `speak(Mood)`: Main high-level voice function.
    *   `chirp()`, `surpriseBeep()`: Legacy SFX helpers (remapped to new engine).
*   **Sound Design**:
    *   All sounds are procedurally generated in real-time (no samples).
    *   Aesthetics: 8-bit, "Chiptune", "Droid-like".
*   **Touch Interaction Sounds**:
    *   `tapSound()`: Gentle 8-bit blip.
    *   `happySound()`: "1-Up" style major arpeggio.
    *   `contentSound()`: Two-tone descent.
    *   `satisfiedSound()`: Short purr burst (Square + Noise).

### 4. Haptics Module (`Haptics.h` / `Haptics.cpp`)
**Responsibility**: Controls vibration motors for haptic feedback with non-blocking patterns.

*   **Hardware**: 2x ERM vibration motors (GPIO 3, 4) via PWM (LEDC).
*   **Intensity Range**: All patterns scaled to 80-100% PWM (204-255) optimized for weak motors, controlled via `Settings::haptic.intensity`.
*   **Non-Blocking Architecture**: Uses phase-based state machine instead of blocking delays.
*   **Key Functions**:
    *   `init()`: Configures LEDC PWM for motor control.
    *   `click()`: TAP pattern - Single pulse with ramp-up (50ms) for snappy feedback on button press.
    *   `doubleClick()`: DOUBLE-TAP pattern - Heartbeat rhythm (lub-DUB): 60ms + 50ms gap + 90ms for combo events.
    *   `alarm()`: ALARM pattern - Alternating left/right pulses (6 × 80ms = 480ms) for urgent alerts.
    *   `startPurr()` / `stopPurr()`: Continuous purr pattern control.
    *   `patternTick()`: Non-blocking update function (call in loop) - Executes all active patterns without delays.
    *   `purrTick()`: Compatibility wrapper that calls `patternTick()`.
*   **Purr Pattern**: 5-phase rhythmic cycle
    - Phase 0: 120ms @ 85% intensity
    - Phase 1: 100ms @ 100% intensity  
    - Phase 2: 110ms @ 85% intensity
    - Phase 3: 90ms @ 80% intensity
    - Phase 4: 130ms @ 85% intensity
    - Loops continuously while purring for natural cat-like feel.
*   **Performance**: Main loop overhead ~1ms per iteration (vs. 60-480ms blocking before).

### 5. Power Module (`Power.h` / `Power.cpp`)
**Responsibility**: Battery monitoring and sleep state management with two-tier wake logic.

*   **Hardware**: ADC on GPIO 15 with voltage divider (0.0–4.2V → 0–3.3V scaled).
*   **State Machine**:
    *   `ACTIVE`: Full operation. Battery monitored every 1s, logged every 30s.
    *   `DIM`: Tired eyes with slower blinks. Idle after 1 minute. Battery monitored every 1s, logged every 30s.
    *   `SLEEPING`: Closed eyes with animated Zzz. Idle after 2 minutes. Battery monitored every 10s (no logging to save power).
*   **Wake Logic** (Two-Tier System):
    *   **Tier 1 (DIM Wake)**: `onActivity()`, `onMotion()`, `onLoudNoise()` → Wakes from DIM mode only (ignored if already ACTIVE).
        *   Used by regular touch, motion, and audio for normal interactions.
    *   **Tier 2 (Any-State Wake)**: `forceWake()` → Unconditionally wakes from SLEEP (also from DIM/ACTIVE).
        *   Used only by combo touch and motion furious shake (powerful triggers).
*   **Battery Management**:
    *   Low battery (< 3.6V) displays warning; shutdown threshold at 3.0V.
    *   Voltage averaging for stable readings (11 samples).
    *   Automatic periodic logging every 30 seconds in ACTIVE/DIM modes.
*   **Callbacks**: `onSleepCallback`, `onWakeCallback`, `onDimCallback` allow modules to react to state changes.

### 6. Touch Module (`Touch.h` / `Touch.cpp`)
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

### 7. Animation Module (`Animation.h` / `Animation.cpp`)
**Responsibility**: Orchestrates high-level behaviors and command sequences.

*   **Dependencies**: `Face`, `Audio`, `Motion`.
*   **Key Functions**:
    *   `init()`: Prepares behavior state logic.
    *   `playStartupSequence()`: Runs the "Wake Up" sequence (Blink -> Laugh -> Happy).
    *   `tick()`: Main behavior tree update (called in loop).

### 8. Events Module (`Events.h` / `Events.cpp`)
**Responsibility**: Central event handler coordinating touch/motion interactions with priority-based behavior management.

*   **Dependencies**: `Face`, `Touch`, `Motion`, `Haptics`, `Audio`, `Power`.
*   **Key Functions**:
    *   `init()`: Initializes event state and priorities.
    *   `update()`: Processes touch events and triggers responses (call in loop after `Touch::update()`).
    *   `onMotionEvent(void*)`: Handle motion events from Motion module.
*   **Event Handling Features**:
    *   All touch interactions trigger haptic and sound feedback.
    *   Touch reactions skip if sleeping (except combo for wake).
    *   **Priority-Based System** (prevents overlapping behaviors):
        *   **PRIORITY_FURIOUS_SHAKE (3)**: Always triggers, overrides everything.
        *   **PRIORITY_COMBO (2)**: Combo touch forces wake from SLEEP.
        *   **PRIORITY_SHAKE_TAP (1)**: Regular shake/tap interactions.
        *   **PRIORITY_TILT (0)**: Subtle tilt (debounced every 2s to prevent spam).
    *   **State-Change Logging** for touch:
        *   Head/chin pressed/released: `{"status":"data","type":"touch","sensor":"head|chin","state":"pressed|released"}`
        *   Actual events logged only: `{"status":"event","type":"tap|petting_start|..."}`
    *   **Animation Timeouts**:
        *   Shake animation auto-resets after 800ms.
        *   Furious shake confused animation auto-resets after 500ms.
*   **Touch Behaviors**:
    *   Head tap → blink + click + tap sound
    *   Head hold start → happy eyes + purr + happy sound
    *   Head hold end → normal eyes + stop purr + content sound
    *   Chin tap → wink + click + chirp
    *   Chin hold start → closed eyes + purr + purr sound
    *   Chin hold end → open eyes + stop purr + satisfied sound
    *   Combo (head + chin) → confused + double-click + surprise beep + **forceWake()**
*   **Motion Behaviors**:
    *   Motion Tilt → curious eyes + click + tap sound (DIM wake only).
    *   Motion Shake → happy eyes + blink + double-click + surprise beep (DIM wake only).
    *   Motion Furious Shake → angry + sweat + confused animation + alarm + yawn + **forceWake()** (SLEEP wake).
    *   Motion Tap → blink + click + chirp (DIM wake only).
    *   Motion Still → reset all animations.

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
    Touch::update();           // Process touch input, generate events
    
    // 2. Handle events (must be after Touch::update)
    Events::update();          // Process touch events → haptic + sound feedback + face expressions
    
    // 3. Update non-blocking systems
    Motion::update();          // Update motion sensor, detect shake/tilt/tap/still
    Motion::logMotionStatus(); // Log motion state periodically (every 5s)
    Power::update();           // Update power state and battery monitoring
    Haptics::purrTick();       // Update ALL patterns (tap, double-tap, alarm, purr) - NON-BLOCKING
    Face::update();            // Update face (animations)
    Animation::tick();         // Update behavior tree
}
```

**Why this order matters:**
- `Touch::update()` must run before `Events::update()` because Events reads the events that Touch generates.
- `Motion::update()` runs before `Power::update()` to ensure motion-based wake events are processed.
- `Motion::logMotionStatus()` has minimal overhead (only logs every 5 seconds).
- `Haptics::purrTick()` must be called every loop to execute non-blocking patterns without stalling the main loop.

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
- **Both Held**: Confused expression + horizontal eye flicker + haptic double-click heartbeat (85-100% intensity) + surprise beep
- Forces wake from SLEEP mode (only combo action strong enough to wake from sleep)

### Petting Behavior
When the user holds the touch button (simulating petting the bot's head):
1. Eyes become happy (`eyes->happy = true`).
2. Haptic purr pattern starts (`Haptics::startPurr()`) with happy sound.
3. Continuous rhythmic purr (5-phase cycle, 80-100% intensity).
4. Occasional blinks while being pet.
5. On release, haptic purr stops with content sound, eyes return to normal.

### Motion Interactions (All with Haptic + Sound Feedback)

**Motion Events:**
- **Tilt**: Gentle tilt beyond threshold → Curious eyes + click haptic + tap sound (wakes from DIM only)
- **Shake**: Brief shake detected → Happy eyes + blink + double-click haptic + surprise beep (wakes from DIM only)
- **Furious Shake**: Sustained hard shake → Angry eyes + sweat + confused animation (jiggling) + alarm haptic + yawn sound (forces wake from ANY state including SLEEP)
- **Tap**: Single tap detected → Blink + click haptic + chirp sound (wakes from DIM only)
- **Still**: Device returned to stationary → Eyes return to normal

### Power State & Wake Logic

**DIM Mode (1 minute idle):**
- Tired eyes, slower blinks, dimmed screen
- Wakes with: Any touch tap, chin tap, motion tap/tilt/shake (via `onActivity()` / `onMotion()`)

**SLEEP Mode (2 minutes idle):**
- Closed eyes with Zzz animation, minimal CPU/display updates
- Wakes ONLY with: Combo touch (head + chin simultaneous) OR motion furious shake (via `forceWake()`)
- Normal touch/motion events do NOT wake from sleep
- Battery reading continues every 10s, but logging disabled (reduced power consumption)

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

## Settings System (`Settings.h` / `Settings.cpp`)

**Responsibility**: Manages dynamic (runtime-applied) and persistent (NVS-saved) settings.

*   **Dynamic Settings** (apply immediately, no reboot):
    *   Face: width, height, radius, spacing, contrast, blink interval, idle interval, curious mode, sweat mode
    *   Audio: volume (0-100), microphone logging enable/disable
    *   Haptics: vibration intensity (0-255, scaled to 80-100% PWM)
    *   LED: brightness, RGB color values
*   **NVS Persistence**: Settings stored in ESP32 Preferences (namespace-based), loaded at boot with `config.h` defaults.
*   **Key Functions**:
    *   `begin()`: Initialize NVS and load settings.
    *   `loadFromNVS()`: Read settings from NVS, use config.h defaults if not found.
    *   `applyFaceSettings()`: Apply face settings to Eyes object immediately.
    *   `applyAudioSettings()`: Apply audio settings immediately.
    *   `applyHapticSettings()`: Apply haptic intensity immediately.

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
| **Power/Battery** | `VBAT_MIN_V`, `VBAT_MAX_V`, `VBAT_CAL`, `VBAT_SAMPLES`, `VBAT_READ_INTERVAL_MS`, `VBAT_LOG_INTERVAL_MS`, `DEFAULT_IDLE_TIMEOUT_MS`, `DEFAULT_SLEEP_TIMEOUT_MS` |
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
