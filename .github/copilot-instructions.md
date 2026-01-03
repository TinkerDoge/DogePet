# DogePet AI Coding Assistant Instructions

## Project Overview
DogePet is an **ESP32-S3 embedded companion robot** firmware written in Arduino C++. The codebase follows a **modular architecture** with `DogePet.ino` as the orchestrator and separate `.cpp/.h` modules for each hardware subsystem.

**Core Architecture Pattern**: Main sketch coordinates singleton modules via static class methods. Each module handles a specific hardware domain (Face, Motion, Audio, etc.). The `sectors/` folder contains an experimental refactor (not yet integrated).

---

## Critical Developer Knowledge

### 1. Build & Flash Workflow
- **Platform**: Arduino IDE or PlatformIO for ESP32-S3
- **Board**: ESP32-S3 Super Mini (Arduino ESP32 core 2.x+)
- **Configuration**: Edit `include/config.h` BEFORE building
- **Libraries**: All dependencies bundled in `libraries/` folder (Adafruit_GFX, Adafruit_SH110X, MPU6050, NimBLE-Arduino, ChronosESP32)
- **IntelliSense**: `.vscode/c_cpp_properties.json` configured for ESP32-S3 with local library paths
- **First Boot**: MPU6050 DMP auto-calibrates during boot sequence

### 2. Hardware Pin Mapping (config.h)
```cpp
I2C_SDA=6, I2C_SCL=5           // OLED (0x3C) + MPU6050 (0x68)
MPU_INT_PIN=2                  // MPU6050 DMP interrupt
FUNC_BTN=41                    // Head touch sensor (Active HIGH with INPUT_PULLDOWN)
TOUCH_CHIN=1                   // Optional chin touch sensor (compile-time)
LED_PIN=48                     // WS2812 status LED
I2S: BCLK=17, LRC=16, DO=33    // Audio output (MAX98357A)
I2S_DI=11                      // Microphone input (INMP441 stereo)
VBAT_PIN=15                    // Battery voltage (ADC with divider)
VIBRO_LEFT=4, VIBRO_RIGHT=3    // Vibration motors (PWM)
```

**Touch Sensor Notes**: TPP223 modules are active-HIGH. Use `INPUT_PULLDOWN` to ensure stable LOW idle state.

### 3. Configuration System (config.h)
All magic numbers, thresholds, and settings live in `include/config.h`:
- **Hardware pins**: All GPIO assignments
- **Display settings**: `SCREEN_W`, `SCREEN_H`, `OLED_CONTRAST`
- **Eye appearance**: `EYE_WIDTH`, `EYE_HEIGHT`, `EYE_BORDER_RADIUS`, `EYE_SPACING`
- **Audio settings**: `AUDIO_SAMPLE_RATE`, `AUDIO_VOLUME`
- **Power settings**: `VBAT_MIN_V`, `VBAT_MAX_V`, `IDLE_TIMEOUT_MS`, `SLEEP_TIMEOUT_MS`
- **Motion settings**: `TILT_THRESHOLD_DEG`, `SHAKE_THRESHOLD_DPS`
- **NVS namespaces**: For persistent storage

**Pattern**: When adding new behaviors, define thresholds/delays in `config.h`, not inline.

### 4. Core Modules (Static Class APIs)

#### Face:: (Face.h/cpp)
- **Display**: Manages SH1106G OLED via Adafruit_SH110X
- **Eyes**: Procedural animation via FluxGarage_RoboEyes
- **Key Functions**:
  - `init()`: Initialize OLED and eye objects
  - `update()`: Refresh eye animations (call in loop)
  - `showBootScreen(msg)`, `updateProgressBar(pct, status)`
  - `getEyes()`: Returns `roboEyes*` for direct manipulation
  - Sleep face with animated Zzz when `isSleeping = true`

#### Motion:: (Motion.h/cpp)
- **Hardware**: MPU6050/MPU6500 with DMP (Digital Motion Processor)
- **Interrupt-driven**: Uses GPIO 2 for DMP data ready
- **Key Functions**:
  - `init()`: Initialize DMP and attach interrupt
  - `calibrate()`: 6-point accel/gyro calibration
  - `update()`: Read DMP FIFO, output quaternion JSON
  - `isReady()`: Returns true if DMP initialized
- **Output**: `{"type":"dmp","w":0.99,"x":0.01,"y":-0.01,"z":0.01}`

#### Audio:: (Audio.h/cpp)
- **I2S Full-duplex**: Mic input (INMP441) + Amp output (MAX98357A)
- **Software synthesis**: Sine wave generation via I2S buffer
- **Key Functions**:
  - `init()`: Install I2S driver
  - `readMicDB()`: Read stereo mic samples, return dB level
  - `playTone(freq, duration)`: Synthesize and play tone
  - `playMelody()`: Startup jingle (C5-E5-G5-C6)
  - `chirp()`, `purrrSound()`, `surpriseBeep()`, `yawn()`: Sound presets
  - `update()`: Monitor mic level, log changes

#### Touch:: (Touch.h/cpp)
- **Debounced input**: 30ms debounce, tap/hold detection
- **Events**: `NONE`, `TAP`, `HOLD_START`, `HOLDING`, `HOLD_END`
- **Key Functions**:
  - `init()`: Configure input pins (FUNC_BTN, optional TOUCH_CHIN)
  - `update()`: Process input, generate events
  - `getHeadEvent()`, `getChinEvent()`: Query current event
  - `getHeadHoldDuration()`, `getChinHoldDuration()`: Hold timing

#### Events:: (Events.h/cpp)
- **Central event handler**: Coordinates touch → behavior reactions
- **Behaviors**:
  - Head tap → eye blink
  - Head hold → happy eyes + purr vibration
  - Chin tap → wink + chirp
  - Chin hold → closed eyes + vertical trembling + purr
  - Combo (head + chin) → confused + surprise beep
- **Safeguards**: Auto-clears stuck animations after timeout

#### Power:: (Power.h/cpp)
- **Battery monitoring**: ADC on GPIO 15 with voltage divider
- **State machine**: `ACTIVE` → `DIM` (1 min) → `SLEEPING` (2 min)
- **Activity tracking**: `onActivity()`, `onMotion()`, `onLoudNoise()`
- **Callbacks**: `onSleepCallback`, `onWakeCallback`, `onDimCallback`

#### Haptics:: (Haptics.h/cpp)
- **Dual motors**: PWM via LEDC (GPIO 3, 4)
- **Patterns**: `click()`, `doubleClick()`, `alarm()`
- **Non-blocking purr**: `startPurr()`, `purrTick()`, `stopPurr()`

#### LED:: (LED.h/cpp)
- **WS2812 NeoPixel**: Single LED on GPIO 48
- **Functions**: `setColor(r,g,b)`, `setBrightness(level)`, `on()`, `off()`

#### Animation:: (Animation.h/cpp)
- **High-level sequences**: `playStartupSequence()` (blink → laugh → happy)
- **Behavior tree**: `tick()` for complex behaviors (expandable)

### 5. Boot Sequence State Machine (DogePet.ino)
```
BOOT_INIT       → I2C scan, splash screen
BOOT_DISPLAY_TEST → Line animation test
BOOT_IMU_TEST   → DMP calibration
BOOT_AUDIO_TEST → Mic read + tone test
BOOT_HAPTIC_TEST → Click + purr test
BOOT_POWER_TEST → Battery voltage read
BOOT_ANIMATION  → Wake-up sequence
BOOT_DONE       → Normal loop operation
```

### 6. Main Loop Flow (DogePet.ino)
```cpp
loop() {
  if (bootState != BOOT_DONE) { runBootSequence(); return; }
  
  Motion::update();       // DMP quaternion updates
  Events::update();       // Touch → behavior handling
  Power::update();        // Sleep state management
  Haptics::purrTick();    // Non-blocking purr
  Touch::update();        // Input debouncing
  Face::update();         // Eye animation refresh
  Animation::tick();      // Behavior tree
}
```

### 7. RoboEyes Integration (FluxGarage_RoboEyes.h)
- **External globals** (legacy, defined in DogePet.ino):
  ```cpp
  bool gEyesAutoFlush = true;    // Auto display.display()
  int gEyesViewportYMax = 0;     // Viewport limit for overlays
  ```
- **Mood flags**: `eyes->happy`, `eyes->tired`, `eyes->angry`, `eyes->confused`
- **Animations**: `eyes->blink()`, `eyes->close()`, `eyes->open()`, `eyes->setVFlicker()`, `eyes->setHFlicker()`

### 8. Serial JSON Protocol
All serial output uses JSON for PC companion app parsing:
- **Status**: `{"status":"ok"|"error"|"info","msg":"..."}`
- **Data**: `{"status":"data","type":"mic"|"power"|"imu",...}`
- **DMP**: `{"type":"dmp","w":0.99,"x":0.01,"y":-0.01,"z":0.01}`
- **Events**: `{"status":"event","type":"tap"|"petting_start",...}`

---

## Developer Tools

### Python Asset Generators (tools/)
- **font_to_header.py**: TTF/OTF → Adafruit_GFX bitmap headers
  ```bash
  python tools/font_to_header.py --ttf fonts/shopee/shopee2021-regular.ttf \
    --size 12 --text assets/vi_charset.txt --name ShopeeRegular12 --out include/ShopeeRegular12.h
  ```
- **png_to_header.py**: PNG icons → PROGMEM bitmap arrays
  ```bash
  python tools/png_to_header.py --in icon/src --out icon/icons_auto.h --w 12 --h 12
  ```

---

## Common Editing Patterns

### Adding New Touch Behaviors
1. Define new behavior in `Events.cpp` under appropriate `TouchEvent` case
2. Use `Face::getEyes()` for eye animations
3. Use `Haptics::` for vibration feedback
4. Use `Audio::` for sound effects
5. Call `Power::onActivity()` to prevent sleep

### Adding New Sound Effects
1. Define new function in `Audio::` namespace (Audio.cpp)
2. Use `playTone(freq, duration)` for simple tones
3. Chain tones with small delays for melodies
4. Call from Events.cpp or Animation.cpp

### Adding New Eye Animations
1. Access eyes via `Face::getEyes()`
2. Set mood flags: `eyes->happy`, `eyes->tired`, `eyes->angry`
3. Use flicker: `eyes->setVFlicker(speed, amplitude)`, `eyes->setHFlicker(speed, amplitude)`
4. Remember to reset flags when behavior ends

### Modifying Boot Sequence
1. Add new `BootState` enum value in DogePet.ino
2. Add case in `runBootSequence()` switch
3. Update progress bar percentages
4. Ensure hardware test runs before dependent tests

---

## Critical "Gotchas"

1. **I2C bus contention**: OLED and MPU6050 share I2C - display updates throttled to 30ms
2. **DMP interrupt timing**: `Motion::update()` is interrupt-driven, runs fast
3. **PWM strapping pin**: GPIO 3 (VIBRO_RIGHT) is strapping pin - handle during boot
4. **LEDC mode**: Haptics uses `analogWrite()` which configures LEDC automatically
5. **Chin sensor optional**: Compile-time `TOUCH_CHIN_ENABLED` in config.h
6. **Events module**: All touch behaviors now handled in Events.cpp, not DogePet.ino

---

## File Organization

```
DogePet/
├── DogePet.ino          # Main orchestrator + boot state machine
├── [Module].cpp         # Module implementations
├── include/
│   ├── config.h         # All pins, constants, NVS namespaces
│   ├── [Module].h       # Module interfaces
│   └── FluxGarage_RoboEyes.h  # Eye animation library
├── libraries/           # Bundled dependencies
├── companion/           # PC companion app (Flask)
├── tools/               # Python utilities
├── sectors/             # Experimental refactor (not integrated)
└── DOCS/                # Documentation
```

---

## Testing & Validation

- **Hardware required**: All features need physical ESP32-S3 + peripherals
- **Serial monitor**: 115200 baud, JSON output
- **Touch test**: Watch for `{"status":"event","type":"tap",...}` messages
- **Motion test**: Watch for `{"type":"dmp","w":...}` quaternion output
- **Audio test**: Listen for boot melody and sound effects

---
**Last Updated**: January 2026 (v2.0 modular architecture)
