# DogePet Project TODO List
**For AI Agents & Continuous Improvement**

Generated: October 6, 2025  
Based on: Comprehensive codebase analysis + similar project research

---

## 🎯 Priority 1: Critical Bugs & Hardware Issues

### HW-001: Touch Sensor Stability
**Status**: ✅ FIXED (2025-10-06)  
**Description**: TPS223 touch sensors had low debounce time (12ms) causing false triggers and making triple-tap nearly impossible  
**Solution Applied**: 
- Increased debounce time from 12ms → 25ms for noise immunity
- Increased tap window: double-tap 450ms → 500ms, triple-tap 650ms → 900ms
- Reordered detection: triple-tap now checked BEFORE double-tap (prevents premature firing)
**Files**: `DogePet.ino` lines 527-543, 1320-1345  
**Success Criteria**: Reliable triple-tap detection without triggering double-tap  
**Priority**: HIGH  
**Notes**: Further hardware debouncing (0.1µF caps) may still improve reliability in noisy environments

### HW-002: I2C Bus Reliability
**Status**: 🔴 OPEN  
**Description**: OLED and MPU6050 share I2C bus at 400kHz - potential conflicts during heavy load  
**Task**: Implement I2C error recovery and reduce clock to 100kHz if instability detected  
**Files**: `DogePet.ino` setup(), `config.h`  
**Success Criteria**: Zero I2C hangs during 48hr stress test  
**Priority**: HIGH

### HW-003: Audio Feedback Loop
**Status**: 🟡 NEEDS TESTING  
**Description**: Microphone may pick up speaker output causing feedback  
**Task**: Implement spatial separation detection and adaptive gain control  
**Files**: `audio.cpp` MIC_GAIN constant, `config.h` MIC_NOISE_THRESHOLD  
**Success Criteria**: No feedback squeal at max volume with close mic placement  
**Priority**: MEDIUM

---

## 🚀 Priority 2: Core Features & Enhancements

### FEAT-001: Conversation Memory System
**Status**: 🔴 OPEN  
**Description**: AI has no memory of previous conversations (mentioned in Roadmap)  
**Task**: Implement rolling 5-message history with SPIFFS persistence  
**Technical Approach**:
- Add `/ai_history.json` file to SPIFFS
- Modify `AICompanion::handleMessage()` to prepend context
- Compress old messages to keep under Gemini token limits
- Add `clearHistory()` command via triple-tap gesture
**Files**: `ai_companion.cpp`, `ai_companion.h`  
**Success Criteria**: AI references previous 3 messages accurately  
**Priority**: HIGH  
**Estimated Effort**: 6-8 hours

### FEAT-002: Personality Customization
**Status**: 🔴 OPEN  
**Description**: Allow users to configure AI personality traits  
**Task**: Add personality presets (Friendly/Sarcastic/Professional/Playful)  
**Technical Approach**:
- Add personality modes to config portal
- Modify `GEMINI_SYSTEM_PROMPT` dynamically
- Store selection in Preferences
**Files**: `ai_companion.h`, WiFi config portal  
**Success Criteria**: 4 distinct personality modes working correctly  
**Priority**: MEDIUM  
**Estimated Effort**: 4 hours

### FEAT-003: Weather Display Integration
**Status**: 🔴 OPEN  
**Description**: ChronosESP32 library provides weather data but not displayed  
**Task**: Add weather face mode with icon display  
**Technical Approach**:
- Add `FACE_WEATHER` mode to FaceMode enum
- Use ChronosESP32 weather APIs (`getWeatherAt()`, `getForecastHour()`)
- Design weather icons (12x12 PNG → header conversion)
- Show temp, condition, forecast on OLED
**Files**: `DogePet.ino`, `tools/png_to_header.py`, new `weather.cpp/h`  
**Success Criteria**: Weather face shows current + 3hr forecast with icons  
**Priority**: MEDIUM  
**Estimated Effort**: 8 hours

### FEAT-004: Advanced Eye Animations
**Status**: 🔴 OPEN  
**Description**: Limited eye expressions (mentioned in Roadmap: "surprised, sleepy")  
**Task**: Expand emotional range with new RoboEyes modes  
**Technical Approach**:
- Add `MS_SURPRISED`, `MS_SLEEPY`, `MS_CONFUSED` to MoodState
- Map to RoboEyes animations (wide eyes, droopy lids, asymmetric)
- Trigger from AI responses or specific events
**Files**: `motion.h`, `animation_engine.cpp`, `FluxGarage_RoboEyes.h`  
**Success Criteria**: 3 new distinct eye moods working smoothly  
**Priority**: LOW  
**Estimated Effort**: 6 hours

### FEAT-005: Desktop Configuration App
**Status**: 🔴 OPEN (Mentioned in Roadmap)  
**Description**: WiFi portal is limited - need full-featured desktop config tool  
**Task**: Build Electron/Tauri cross-platform app for device management  
**Technical Approach**:
- BLE connection for config transfer (use ChronosESP32 custom characteristic)
- Visual personality editor
- Font upload tool
- Firmware OTA updater
- Log viewer
**Files**: New `desktop-app/` folder, BLE command protocol in `DogePet.ino`  
**Success Criteria**: Windows/Mac/Linux app can configure all settings  
**Priority**: LOW  
**Estimated Effort**: 40+ hours

---

## 🔧 Priority 3: Code Quality & Architecture

### ARCH-001: Complete Sectors Refactor
**Status**: 🟡 IN PROGRESS (Experimental folder exists)  
**Description**: `sectors/` modular architecture not wired to main sketch  
**Task**: Integrate sectors system and deprecate monolithic DogePet.ino  
**Technical Approach**:
- Wire `Sectors::beginAll()` into `setup()`
- Wire `Sectors::updateAll()` into `loop()`
- Migrate global variables to sector namespaces
- Update documentation
**Files**: `sectors/*.cpp/h`, `DogePet.ino`  
**Success Criteria**: All functionality preserved, codebase modular  
**Priority**: MEDIUM  
**Estimated Effort**: 16 hours

### ARCH-002: Reduce Global Variable Scope
**Status**: 🔴 OPEN  
**Description**: 50+ global variables make testing difficult  
**Task**: Encapsulate state in structs/classes  
**Technical Approach**:
- Create `RobotState` struct for motion/mood/power
- Create `UIState` struct for display/toast/face mode
- Pass state references instead of globals
**Files**: `DogePet.ino` lines 200-310  
**Success Criteria**: <10 global variables remaining  
**Priority**: LOW  
**Estimated Effort**: 12 hours

### ARCH-003: Implement Unit Testing Framework
**Status**: 🔴 OPEN  
**Description**: No automated tests - manual testing only  
**Task**: Add Google Test or Unity framework for embedded testing  
**Technical Approach**:
- Create `test/` folder with PlatformIO test configuration
- Mock hardware dependencies (I2C, I2S, BLE)
- Test modules: Audio sequencer, AI JSON parsing, tap detection
**Files**: New `test/` folder, `platformio.ini`  
**Success Criteria**: 50+ unit tests with CI integration  
**Priority**: MEDIUM  
**Estimated Effort**: 20 hours

---

## ⚡ Priority 4: Performance Optimizations

### PERF-001: Display Update Optimization
**Status**: 🟡 NEEDS PROFILING  
**Description**: Full screen redraws may be inefficient  
**Task**: Implement dirty rectangle tracking for partial updates  
**Technical Approach**:
- Track changed regions with bounding boxes
- Only update modified areas in `display.display()`
- Profile FPS improvement
**Files**: `DogePet.ino` display rendering functions  
**Success Criteria**: 2x faster display updates, lower I2C traffic  
**Priority**: LOW  
**Estimated Effort**: 8 hours

### PERF-002: Audio Mixing Optimization
**Status**: 🟡 ACCEPTABLE  
**Description**: Software mixer runs every loop - may benefit from ARM optimizations  
**Task**: Use ESP32 DSP instructions for voice mixing  
**Technical Approach**:
- Replace float math with fixed-point (Q15 format)
- Use ESP32 DSP intrinsics (`esp_dsp.h`)
- Benchmark CPU usage reduction
**Files**: `audio.cpp` `update()` function  
**Success Criteria**: 30% less CPU usage for audio  
**Priority**: LOW  
**Estimated Effort**: 12 hours

### PERF-003: Reduce AI Response Latency
**Status**: 🟡 ACCEPTABLE  
**Description**: Gemini API calls take 2-5 seconds  
**Task**: Implement response caching and predictive preloading  
**Technical Approach**:
- Cache common responses (greetings, confirmations)
- Pre-fetch likely follow-ups based on context
- Use HTTP keep-alive for faster subsequent requests
**Files**: `ai_companion.cpp` Gemini API client  
**Success Criteria**: Sub-1s responses for cached queries  
**Priority**: LOW  
**Estimated Effort**: 6 hours

---

## 🎨 Priority 5: User Experience

### UX-001: Haptic Feedback System
**Status**: 🔴 OPEN  
**Description**: No tactile feedback - could improve interaction feel  
**Task**: Add vibration motor support with gesture-specific patterns  
**Technical Approach**:
- Add vibration motor to GPIO (via MOSFET)
- Create vibration patterns (tap=short, hold=pulsing, error=double)
- Add config toggles
**Files**: New `haptic.cpp/h`, `config.h`  
**Success Criteria**: Distinct haptic feedback for 5+ gestures  
**Priority**: LOW  
**Estimated Effort**: 6 hours

### UX-002: Multi-Language Support
**Status**: 🟡 PARTIAL (Vietnamese fonts exist)  
**Description**: UI text is hardcoded English  
**Task**: Add i18n framework with Vietnamese + English switchable  
**Technical Approach**:
- Create `lang/` folder with JSON translation files
- Add language selector in config portal
- Use font_to_header.py for extended Unicode coverage
**Files**: New `lang/` folder, toast/clock rendering  
**Success Criteria**: Full UI in EN + VI languages  
**Priority**: LOW  
**Estimated Effort**: 10 hours

### UX-003: Sleep Meditation Mode
**Status**: 🔴 OPEN (CREATIVE IDEA)  
**Description**: Utilize audio + display for calming sleep aid  
**Task**: Add "Meditation Mode" with breathing exercises + ambient sounds  
**Technical Approach**:
- Slow-breathing eye animation (4s in, 6s out)
- Gentle ambient tones (nature sounds, white noise)
- Auto-enable after 9PM or via gesture
**Files**: `audio.cpp` new sequences, `DogePet.ino` new mode  
**Success Criteria**: 10-minute meditation program playable  
**Priority**: LOW  
**Estimated Effort**: 8 hours

---

## 🛡️ Priority 6: Stability & Reliability

### STAB-001: Watchdog Timer Implementation
**Status**: 🔴 OPEN  
**Description**: No watchdog - system hangs require manual reset  
**Task**: Enable ESP32 hardware watchdog with strategic feed points  
**Technical Approach**:
- Enable TWDT (Task Watchdog Timer) in setup()
- Add `esp_task_wdt_reset()` in main loop
- Set timeout to 10 seconds
**Files**: `DogePet.ino` setup() and loop()  
**Success Criteria**: Auto-recovery from freezes within 10s  
**Priority**: HIGH  
**Estimated Effort**: 2 hours

### STAB-002: Brownout Detection & Recovery
**Status**: 🔴 OPEN  
**Description**: Battery undervoltage can corrupt SPIFFS  
**Task**: Implement graceful shutdown on low battery  
**Technical Approach**:
- Monitor VBAT voltage continuously
- Below 3.2V: disable WiFi/BLE, show low battery face
- Below 3.0V: save state and enter deep sleep
**Files**: `clock.cpp` battery functions, `DogePet.ino`  
**Success Criteria**: No SPIFFS corruption after 10 brownout events  
**Priority**: HIGH  
**Estimated Effort**: 4 hours

### STAB-003: OTA Firmware Update System
**Status**: 🔴 OPEN  
**Description**: Manual USB flashing is inconvenient  
**Task**: Add WiFi OTA update capability with rollback protection  
**Technical Approach**:
- Use ArduinoOTA library
- Add update UI on OLED with progress bar
- Implement partition validation
**Files**: New `ota.cpp/h`, `DogePet.ino`  
**Success Criteria**: Successful OTA from desktop app  
**Priority**: MEDIUM  
**Estimated Effort**: 8 hours

---

## 📊 Priority 7: Monitoring & Analytics

### MON-001: Usage Statistics Tracking
**Status**: 🟡 PARTIAL (AI stats exist)  
**Description**: Limited insight into device usage patterns  
**Task**: Track and display usage analytics  
**Technical Approach**:
- Log gesture counts, uptime, mood changes
- Store in SPIFFS with daily rollover
- Add `/stats` endpoint to config portal
**Files**: New `analytics.cpp/h`, WiFi portal  
**Success Criteria**: 30-day usage dashboard accessible  
**Priority**: LOW  
**Estimated Effort**: 6 hours

### MON-002: Crash Report System
**Status**: 🔴 OPEN  
**Description**: Crashes leave no trace for debugging  
**Task**: Implement coredump storage to SPIFFS  
**Technical Approach**:
- Enable ESP32 coredump to flash partition
- Parse and display stack trace on next boot
- Upload crash logs to desktop app
**Files**: `platformio.ini` partition table, new `crash_reporter.cpp`  
**Success Criteria**: Last 5 crashes retrievable with stack traces  
**Priority**: MEDIUM  
**Estimated Effort**: 10 hours

---

## 🎓 Priority 8: Documentation & Tooling

### DOC-001: Interactive Hardware Assembly Guide
**Status**: 🔴 OPEN  
**Description**: 3D models in `Prints/` but no assembly instructions  
**Task**: Create step-by-step assembly guide with photos  
**Technical Approach**:
- Photograph assembly process
- Create markdown guide in `docs/ASSEMBLY.md`
- Add wiring diagram (Fritzing or similar)
**Files**: New `docs/` folder  
**Success Criteria**: Beginner can assemble from guide  
**Priority**: LOW  
**Estimated Effort**: 6 hours

### DOC-002: API Documentation Generator
**Status**: 🔴 OPEN  
**Description**: No API docs for namespace functions  
**Task**: Add Doxygen documentation generation  
**Technical Approach**:
- Add Doxygen comments to all public APIs
- Configure Doxyfile for HTML output
- Host on GitHub Pages
**Files**: All `.h` files, new `Doxyfile`  
**Success Criteria**: Complete API reference published  
**Priority**: LOW  
**Estimated Effort**: 12 hours

### TOOL-001: Animation Preview Tool
**Status**: 🔴 OPEN  
**Description**: Hard to visualize eye animations without hardware  
**Task**: Create Python simulator for RoboEyes  
**Technical Approach**:
- Use pygame to render OLED display
- Implement RoboEyes animation logic in Python
- Allow live editing of parameters
**Files**: New `tools/robo_eyes_simulator.py`  
**Success Criteria**: Real-time animation preview matching hardware  
**Priority**: LOW  
**Estimated Effort**: 16 hours

---

## 🌟 Priority 9: Advanced/Experimental Features

### EXP-001: Voice Recognition (On-Device)
**Status**: 🔴 OPEN (CHALLENGING)  
**Description**: Microphone input currently only detects noise level  
**Task**: Implement wake word detection using ESP-SR library  
**Technical Approach**:
- Train "Hey DogePet" wake word model
- Use ESP32-S3 AI acceleration
- Trigger AI interaction on detection
**Files**: New `voice_recognition.cpp/h`, `audio.cpp`  
**Success Criteria**: 90% wake word accuracy in quiet environment  
**Priority**: EXPERIMENTAL  
**Estimated Effort**: 40+ hours

### EXP-002: Bluetooth Audio Streaming
**Status**: 🔴 OPEN  
**Description**: I2S audio output could stream from phone  
**Task**: Add A2DP sink mode for music playback  
**Technical Approach**:
- Use ESP32 A2DP library
- Mix music with sound effects
- Show playback controls on OLED
**Files**: `audio.cpp`, new BLE profiles  
**Success Criteria**: Stream music from phone while maintaining SFX  
**Priority**: EXPERIMENTAL  
**Estimated Effort**: 20 hours

### EXP-003: Gesture Recognition (IMU)
**Status**: 🔴 OPEN  
**Description**: MPU6050 data underutilized - could detect complex gestures  
**Task**: Implement shake/rotation/toss gesture classifier  
**Technical Approach**:
- Collect training data for gestures
- Train TinyML model (TensorFlow Lite Micro)
- Deploy to ESP32
**Files**: New `gesture_ml.cpp/h`, `motion.cpp`  
**Success Criteria**: Recognize 5+ gestures with 85% accuracy  
**Priority**: EXPERIMENTAL  
**Estimated Effort**: 60+ hours

### EXP-004: Companion App (Mobile)
**Status**: 🔴 OPEN  
**Description**: ChronosESP32 app is generic - need custom DogePet app  
**Task**: Build React Native mobile app for iOS/Android  
**Technical Approach**:
- BLE connection management
- Remote control (trigger faces, send messages)
- AI conversation history viewer
- Settings sync
**Files**: New `mobile-app/` folder  
**Success Criteria**: App store published for both platforms  
**Priority**: EXPERIMENTAL  
**Estimated Effort**: 80+ hours

---

## 🔒 Priority 10: Security & Privacy

### SEC-001: Encrypted Credential Storage
**Status**: 🟡 PLAINTEXT  
**Description**: WiFi password and API key stored unencrypted in SPIFFS  
**Task**: Implement ESP32 flash encryption  
**Technical Approach**:
- Enable flash encryption in partition table
- Migrate secrets to NVS with encryption
- Update config portal
**Files**: `platformio.ini`, `ai_companion.cpp`, WiFi functions  
**Success Criteria**: Credentials unreadable via USB flash dump  
**Priority**: MEDIUM  
**Estimated Effort**: 6 hours

### SEC-002: Rate Limiting for BLE Commands
**Status**: 🔴 OPEN  
**Description**: No protection against BLE spam attacks  
**Task**: Add rate limiting for incoming BLE notifications  
**Technical Approach**:
- Track command timestamps per client
- Max 10 commands/minute per MAC address
- Reject excess with error message
**Files**: `DogePet.ino` BLE callback  
**Success Criteria**: Withstand 100 cmd/sec spam without crash  
**Priority**: LOW  
**Estimated Effort**: 3 hours

---

## 📋 Maintenance Tasks

### MAINT-001: Update Library Dependencies
**Status**: 🟡 ONGOING  
**Description**: Bundled libraries may have security/bug fixes  
**Task**: Audit and update all libraries in `libraries/` folder  
**Files**: All library folders  
**Success Criteria**: All libraries at latest stable version  
**Priority**: MEDIUM  
**Effort**: 4 hours quarterly

### MAINT-002: Remove Commented Code
**Status**: 🟡 CLEANUP NEEDED  
**Description**: Significant commented-out code blocks throughout  
**Task**: Audit and remove dead code, add TODOs where needed  
**Files**: `DogePet.ino`, `audio.cpp`, `config.h`  
**Success Criteria**: No commented code older than 6 months  
**Priority**: LOW  
**Effort**: 2 hours

### MAINT-003: Reduce Serial Debug Output
**Status**: 🟡 VERBOSE  
**Description**: Debug prints consume flash and slow execution  
**Task**: Gate all Serial prints behind `ENABLE_DEBUG_LOGS` flag  
**Files**: All `.cpp/.ino` files  
**Success Criteria**: Silent operation when debugging disabled  
**Priority**: LOW  
**Effort**: 4 hours

---

## 🎮 Community Contributions

### COMM-001: Example Sketches Collection
**Status**: 🔴 OPEN  
**Description**: No example programs for learning  
**Task**: Create `examples/` folder with 10+ sample programs  
**Examples**:
- Hello World (basic eyes)
- Notification viewer
- Motion reactive mood
- Simple AI chatbot
- Weather display
**Files**: New `examples/` folder  
**Success Criteria**: Each example < 200 lines, fully documented  
**Priority**: LOW  
**Effort**: 16 hours

### COMM-002: Plugin Architecture
**Status**: 🔴 OPEN (AMBITIOUS)  
**Description**: Allow community to add features without forking  
**Task**: Design plugin API for face modes and behaviors  
**Technical Approach**:
- Define plugin interface (init/update/draw)
- Load plugins from SPIFFS at runtime
- Add plugin manager in config portal
**Files**: New `plugin_system.cpp/h`  
**Success Criteria**: 3rd party face mode loadable dynamically  
**Priority**: EXPERIMENTAL  
**Effort**: 40+ hours

---

## 📈 Success Metrics

**Track these KPIs to measure improvement:**

- **Stability**: Mean Time Between Failures (MTBF) > 72 hours
- **Responsiveness**: UI gesture response < 100ms
- **AI Quality**: Conversation coherence > 4/5 user rating
- **Battery Life**: >12 hours continuous operation
- **Code Quality**: Test coverage > 60%
- **Community**: >10 GitHub stars, >3 contributors

---

## 🤖 AI Agent Instructions

When working on tasks from this list:

1. **Read the context** - Check referenced files and understand existing patterns
2. **Follow conventions** - Match code style, use existing namespaces, update docs
3. **Test thoroughly** - Verify on hardware if possible, add unit tests
4. **Update documentation** - Modify README.md, copilot-instructions.md, and this TODO
5. **Commit atomically** - One task per commit with descriptive message
6. **Cross-reference** - Update related tasks when completing dependencies

**Priority Legend:**
- 🔴 OPEN - Not started
- 🟡 IN PROGRESS - Partially complete
- ✅ DONE - Completed and verified
- ⚠️ BLOCKED - Waiting on dependency

---

**Last Updated**: October 6, 2025  
**Maintainer**: TinkerDoge  
**License**: See LICENSE file
