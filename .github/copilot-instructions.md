# DogePet AI Coding Assistant Instructions

## Project Overview
DogePet is an **ESP32-S3 embedded companion robot** firmware written in Arduino C++. It's a single-file monolithic sketch (`DogePet.ino`) with modular headers, featuring animated RoboEyes, motion-reactive emotions (MPU6050), BLE notifications (ChronosESP32), optional Gemini AI integration, and I2S audio synthesis.

**Core Architecture Pattern**: Central sketch coordinates specialized subsystems via namespaces and lightweight wrappers. The `sectors/` folder contains an experimental modular refactor (not yet integrated).

## Critical Developer Knowledge

### 1. Build & Flash Workflow
- **Platform**: Arduino IDE or PlatformIO for ESP32-S3
- **Board**: ESP32-S3 Super Mini (Arduino ESP32 core 2.x+)
- **Configuration**: Edit `config.h` BEFORE building (WiFi credentials, Gemini API key, feature flags)
- **Libraries**: All dependencies bundled in `libraries/` folder (Adafruit_GFX, Adafruit_SH110X, NimBLE-Arduino, ChronosESP32)
- **IntelliSense**: `.vscode/c_cpp_properties.json` configured for ESP32-S3 with local library paths
- **First Boot**: MPU6050 auto-calibrates - keep device flat and still for 5 seconds

### 2. Hardware Pin Mapping (config.h)
```cpp
I2C_SDA=6, I2C_SCL=5           // OLED (0x3C) + MPU6050
TOUCH_DOWN=35, TOUCH_UP=39     // TPS223 touch sensors (active-HIGH: LOW idle, HIGH when touched)
FUNC_BTN=41                    // Function button (active-LOW, pull-up, hardware double-press = power off)
LED_PIN=48                     // WS2812 status LED
I2S: BCLK=17, LRC=16, DO=33    // Audio output (MAX98357A)
I2S_DI=14                      // Microphone input (INMP441)
VBAT_PIN=15                    // Battery voltage (ADC with divider)
```

**Touch Sensor Notes**: TPS223 modules are active-HIGH but may float without pull resistors. Use `pinMode(pin, INPUT_PULLDOWN)` to ensure stable LOW idle state and prevent false triggers.

### 3. Configuration System (config.h)
All magic numbers, thresholds, feature flags, and secrets live in `config.h`:
- **Feature toggles**: `ENABLE_WIFI`, `ENABLE_GEMINI_AI`, `ENABLE_LAZY_MODE`, `ENABLE_*_SFX`
- **Motion thresholds**: `TILT_HAPPY_DEG`, `SHAKE_ANGRY_DPS`, `SHAKE_FURIOUS_DPS`
- **Timing constants**: `MOOD_HOLD_MS`, `GEMINI_COOLDOWN_MS`, `DISPLAY_UPDATE_MS`
- **WiFi/API secrets**: `WIFI_SSID`, `WIFI_PASSWORD`, `GEMINI_API_KEY` (NEVER commit credentials)

**Pattern**: When adding new behaviors, define thresholds/delays in `config.h`, not inline.

### 4. Core Subsystems (Namespace APIs)

#### Audio::* (audio.h/cpp)
- **I2S mixer** with layered voices for "droid" sound effects
- **Non-blocking sequencer**: `startSequence()`, check `isSequencePlaying()`
- **Microphone**: `getMicrophoneLevel()`, `isLoudNoiseDetected()` with cooldown tracking
- **Presets**: `sfxAngry()`, `sfxNotify()`, `playCuteChatterFree()`, `binaryTalkRandom()`
- **AI SFX parsing**: `playSfxSequence("880>660,150,Q,200; 440,120,S,160")` parses AI-generated sound specs

#### AICompanion::* (ai_companion.h/cpp)
- **Gemini API integration** with token-optimized prompts (< 150 chars responses)
- **State codes**: Robot state compressed to single chars (N/H/A/F/T for moods)
- **Message handling**: BLE messages prefixed `AI:`, `@doge`, or `DogePet:` auto-route here
- **Background chatter**: `handleBackgroundChatter()` with randomized cooldowns (30-90s)
- **Response parsing**: `executeAIActions()` extracts JSON fields (animation, sfx, toast_message, thought)
- **Usage tracking**: `getUsageStats()` for monitoring API call counts

#### AnimationEngine::* (animation_engine.h/cpp)
- **Eye commands**: `executeEyeAnimation("blink")`, `lookDirection("left")`, `setEyeMood("happy")`
- **Sound wrappers**: `executeSoundFX("notify")` maps AI strings to Audio:: presets

### 5. RoboEyes Integration (FluxGarage_RoboEyes.h)
- **External global controls**:
  ```cpp
  extern bool gEyesAutoFlush;      // Set false to disable display.display() calls
  extern int gEyesViewportYMax;    // Limit eye drawing area (e.g., 48 for toast overlay)
  ```
- **Draw throttling**: Update at ~33Hz (`ROBOEYES_UPDATE_MS = 30ms`) to prevent I2C bus congestion
- **Mood mapping**: `setEyesMood(MoodState)` -> `roboEyes.happy/angry/tired` flags

### 6. State Machine & Loop Architecture

**Main loop flow** (DogePet.ino ~line 1298):
```cpp
loop() {
  chrono.loop();              // BLE event processing
  updatePowerManagement();    // Track activity, handle lazy/sleep modes
  checkMicrophoneNoise();     // Detect loud sounds
  AICompanion::handleBackgroundChatter(); // Periodic AI chatter
  [input handling]            // Touch/button debouncing, tap detection
  [face rendering]            // Switch between FACE_EYES/FACE_CLOCK/FACE_NOTIF
  Audio::update();            // Fill I2S buffer
  Audio::sequencerUpdate();   // Advance multi-step sounds
}
```

**Critical timing**: 
- Display updates throttled to 50ms (`DISPLAY_UPDATE_MS`)
- Eye updates at 30ms (`ROBOEYES_UPDATE_MS`)
- Audio buffers filled every loop (Audio::update())

### 7. Input Handling Patterns
- **Debounced tap detection**: `touchDownDoubleTap()`, `touchDownTripleTap()` use windowed tap counting
  - Debounce time: 25ms (configurable via `TOUCH_DEBOUNCE_MS` in config.h)
  - Double-tap window: 500ms, Triple-tap window: 900ms
  - **Critical**: Triple-tap checked BEFORE double-tap to prevent premature triggering
- **Hold detection**: `FUNC_BTN` hold (2s) + touch sensor combos for BLE/WiFi toggles
- **Hardware limitation**: FUNC_BTN double-press is hardware power-off, triple-press impossible

**Gesture to action mapping** (README.md):
- Double-tap TOUCH_DOWN → cycle faces
- Triple-tap TOUCH_DOWN → WiFi config portal
- Triple-tap TOUCH_UP → toggle silent mode
- Hold FUNC_BTN + TOUCH_DOWN → toggle BLE
- Hold FUNC_BTN + TOUCH_UP → toggle WiFi

### 8. Toast/Notification System
- **Typewriter effect**: `showToastTypewriter(text, durationMs, typeSpeedMs)` animates character-by-character
- **Binary chatter mode**: Set `toastScatter=true` for scatter text, `toastNoFrame=true` to skip border
- **Viewport clipping**: Toast draws at Y >= `gEyesViewportYMax` to avoid eye overlap

### 9. Power Management
- **Lazy mode**: After 2min idle (`LAZY_AFTER_MS`), enters low-power state with periodic jingles
- **Display dimming**: Brightness reduces to `SLEEP_BRIGHTNESS` after `DIM_AFTER_MS` (10s)
- **Wake triggers**: Touch, button, motion, microphone noise, or BLE notifications

### 10. Developer Tools

#### Python Asset Generators (tools/)
- **font_to_header.py**: TTF/OTF → Adafruit_GFX bitmap headers with UTF-8 support
  ```bash
  python tools/font_to_header.py --ttf fonts/shopee/shopee2021-regular.ttf \
    --size 12 --text assets/vi_charset.txt --name ShopeeRegular12 --out include/ShopeeRegular12.h
  ```
- **png_to_header.py**: PNG icons → PROGMEM bitmap arrays
  ```bash
  python tools/png_to_header.py --in icon/src --out icon/icons_auto.h --w 12 --h 12
  ```

#### Serial Debugging
- Verbose logging for AI: Set `ENABLE_AI_DEBUG_LOGS = true` in `config.h`
- Motion debug: `debugPrintIMUIfChanged()` prints accel/gyro when thresholds crossed
- Audio diagnostics: `Audio::runDiagnostics()` in setup() checks I2S/mic hardware

## Common Editing Patterns

### Adding New Emotions/Moods
1. Add enum to `MoodState` in `motion.h`
2. Map to RoboEyes flags in `setEyesMood()` (DogePet.ino)
3. Add trigger logic in `updateEmotionsFromIMU()` (motion.cpp)
4. Add thresholds to `config.h` (e.g., `GENTLE_TAP_DPS`)

### Adding New Sound Effects
1. Define preset in `Audio::` namespace (audio.cpp)
2. Use `playBleep()` for layered tones or `startSequence()` for melodies
3. Add feature flag to `config.h` (e.g., `ENABLE_NEW_SFX`)
4. Call from appropriate event handler with debounce check

### Extending AI Capabilities
1. Update `GEMINI_SYSTEM_PROMPT` in `ai_companion.h` (keep < 120 chars)
2. Add new JSON response fields to `executeAIActions()` parser
3. Test with `AI: test` BLE message (see setup() debug logs)
4. Monitor token usage with `getUsageStats()`

### Modifying UI/Display
1. Check `gEyesViewportYMax` to reserve vertical space
2. Use `displayNeedsUpdate` flag to prevent unnecessary redraws
3. Throttle updates with `lastDisplayUpdateMs` checks
4. For overlays, draw after RoboEyes but before `display.display()`

## Critical "Gotchas"

1. **stdint.h workarounds**: Arduino IDE prototype generation requires manual typedef guards (see DogePet.ino lines 10-17)
2. **I2C bus contention**: OLED and MPU6050 share I2C - don't update display faster than 50ms
3. **Audio timing**: Always call `Audio::update()` in loop(), sequencer won't advance without it
4. **BLE notification callbacks**: Run in BLE task context - copy strings immediately (see chrono.setNotificationCallback)
5. **WiFi blocking**: `WiFi.begin()` can block for 30s - UI feedback critical (see setWifiEnabled)
6. **API credentials**: NEVER commit `GEMINI_API_KEY` or `WIFI_PASSWORD` - use placeholders in examples

## File Organization Strategy
- **Monolithic sketch**: `DogePet.ino` contains setup/loop and UI rendering
- **Subsystem headers**: `audio.h`, `motion.h`, `ai_companion.h` export namespace APIs
- **Config centralization**: `config.h` is single source of truth for all constants
- **Future refactor**: `sectors/` folder contains experimental modular architecture (not yet wired to main sketch)

## Testing & Validation
- **Hardware requirements**: All features need physical ESP32-S3 + peripherals (no emulator)
- **BLE testing**: Use nRF Connect app or ChronosESP32 companion app
- **AI testing**: Send `AI: test` via BLE notifications to verify Gemini integration
- **Audio validation**: Check serial output for "I2S initialized" and diagnostics

## Common Tasks Quick Reference

**Add new config constant**: Edit `config.h` → Rebuild  
**Change WiFi credentials**: `config.h` lines 198-200  
**Adjust motion sensitivity**: `config.h` lines 75-82  
**Generate font header**: `python tools/font_to_header.py [args]`  
**Debug AI responses**: `ENABLE_AI_DEBUG_LOGS = true` in `config.h`  
**Test microphone**: Check serial for "Loud noise detected!" messages  
**Reduce API costs**: Increase `GEMINI_COOLDOWN_MS` and `AI_CHATTER_INTERVAL_MS`

---
**Last Updated**: Based on codebase analysis (October 2025)
