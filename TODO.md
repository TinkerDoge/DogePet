# DogePet TODO / Improvement Plan

Short, actionable items to improve code quality, stability, and features after the Sectors refactor.

## Priority 0 — Fixes for current branch
- [ ] Verify build after refactor on ESP32-S3 board package
- [ ] Replace any remaining direct RoboEyes state access in `DogePet.ino` with Display sector APIs
- [ ] Ensure `Sectors::beginAll()` is called after hardware init and before UI drawing
- [ ] Confirm pin constants from `config.h` are used everywhere (no hardcoded pins)
- [ ] Confirm BLE Chronos callbacks still update Vitals/Notifications

## Priority 1 — UX polish
- [ ] Add `Sectors::Display::isBlinking()` to trigger blink SFX precisely
- [ ] Expose `setCuriosity`, `setBorderradius`, `setSpacebetween` tuning via config
- [ ] Add status LED patterns per mood (calm pulse, angry strobe)
- [ ] Smooth toast typewriter with per-character micro-beep toggle (Voice)

## Priority 2 — Reliability & power
- [ ] Add brownout-safe boot (defer OLED init until VBAT stable)
- [ ] Implement lazy brightness scaling + auto dim with motion timeout
- [ ] Debounce touch/btn with state machine to avoid ghosting
- [ ] Persist volume and talkative level to NVS (Preferences)

## Priority 3 — Sensors & motion
- [ ] Add auto-recalibration routine for MPU6050 (long idle on flat surface)
- [ ] Provide tilt calibration wizard (store offsets in NVS)
- [ ] Expose filtered accel/gyro to a debug overlay
- [ ] Add step/gesture recognizer hooks (nod/shake head mapping)

## Priority 4 — AI/Brain
- [ ] Centralize AI prompts/templates and add locale pack (short vs verbose)
- [ ] Add a simple rules engine fallback when WiFi is off
- [ ] Add guardrails to ensure AI never floods the toast/voice channels
- [ ] Track request/error counters and expose via a tiny debug web UI

## Priority 5 — Audio/Voice
- [ ] Add `Voice::beginMicrophone()` gating based on config to save power
- [ ] Add sequence presets (startup, curious, sleepy) with param hooks
- [ ] Gate idle chatter SFX by `ENABLE_IDLE_AUDIO_CHATTER`
- [ ] Adaptive mic threshold (noise floor tracking)

## Priority 6 — Rendering/UI
- [ ] Migrate any remaining `DogePet.ino` rendering helpers into `sectors/src/UI.*`
- [ ] Consolidate clock/notification drawing into `Vitals` completely
- [ ] Add small FPS meter toggle for debugging
- [ ] Add dithering option for 1-bit assets

## Priority 7 — Testing & docs
- [ ] Add a minimal unit test harness for non-Arduino logic (string parsing, math)
- [ ] Document sector APIs with quick examples in `sectors/example.ino`
- [ ] Add wiring diagram image and update README
- [ ] Create a tiny serial CLI for quick diagnostics (e.g., `mood happy`, `blink`)

## Stretch goals
- [ ] Voice synthesis (formant/PSOLA toy voice) for simple phrases
- [ ] BLE GATT service for remote control app (mode, mood, volume)
- [ ] OTA updates (Basic HTTP/SPIFFS-based)
- [ ] Multi-face layout system with tiny scene graph
