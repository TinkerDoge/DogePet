# DogePet Software Architecture

## Overview
The DogePet firmware has been refactored into a modular architecture to improve maintainability, testability, and separation of concerns. The system is composed of multiple singleton modules, each responsible for a specific hardware domain or logical function.

The main sketch (`DogePet.ino`) acts as the orchestrator, initializing these modules in a specific sequence, running a boot test sequence, and routing global events (like Serial commands and touch input).

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
**Responsibility**: Handles the Inertial Measurement Unit (IMU) for motion sensing.

*   **Hardware**: MPU6050/MPU6500 (I2C 0x68).
*   **Libraries**: `MPU6050` (I2Cdev).
*   **Key Functions**:
    *   `init()`: Connects to MPU6050 and verifies communication.
    *   `calibrate()`: Sets sensor offsets (assumes stationary boot).
    *   `isReady()`: Returns true if sensor is healthy.
    *   `getRawData(...)`: Populates 6-axis acceleration and gyro data.
    *   `update()`: Monitors IMU and logs only when values change.
    *   `setLogEnabled(bool)`: Enable/disable change-detection logging.

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
├── DogePet.ino          # Main Sketch (Coordinator + Boot State Machine)
├── include/             # Header Files (.h)
│   ├── config.h         # Pinouts, Constants & NVS Namespaces
│   ├── Face.h           # Face Module Interface
│   ├── Motion.h         # Motion Module Interface
│   ├── Audio.h          # Audio Module Interface
│   ├── Animation.h      # Animation Module Interface
│   ├── Power.h          # Power/Sleep Module Interface
│   ├── Haptics.h        # Haptics Module Interface
│   ├── Touch.h          # Touch Input Module Interface
│   ├── LED.h            # LED Module Interface
│   ├── serial_cmd.h     # Serial Command Handler Interface
│   └── FluxGarage_RoboEyes.h  # Eye Animation Library
├── Face.cpp             # Face Implementation
├── Motion.cpp           # Motion Implementation
├── Audio.cpp            # Audio Implementation
├── Animation.cpp        # Animation Implementation
├── Power.cpp            # Power/Sleep Implementation
├── Haptics.cpp          # Haptics Implementation
├── Touch.cpp            # Touch Input Implementation
├── LED.cpp              # LED Implementation (if exists)
├── serial_cmd.cpp       # Serial Command Handler
├── companion/           # PC Companion App (Python/Flask)
│   ├── app.py           # Flask server
│   ├── static/          # Web UI (HTML/JS/CSS)
│   └── serial_comm.py   # Serial communication
└── DOCS/                # Documentation
    ├── HARDWARE.md      # Pinouts & wiring
    ├── SOFTWARE_ARCHITECTURE.md  # This file
    └── WebApp_Protocol.md  # Serial API protocol
```

---

## Configuration System

All configurable parameters are centralized in `config.h` with NVS persistence:

| NVS Namespace | Purpose |
|---------------|----------|
| `dogepet_hw` | Hardware pin configuration |
| `dogepet_eyes` | Eye appearance settings |
| `dogepet_audio` | Audio/volume settings |
| `dogepet_hap` | Haptics settings |
| `dogepet_led` | LED color/brightness |
| `dogepet_pwr` | Power/battery settings |
| `dogepet_cfg` | Timing/behavior settings |
| `dogepet_imu` | IMU calibration offsets |
| `dogepet_dev` | Device info |
