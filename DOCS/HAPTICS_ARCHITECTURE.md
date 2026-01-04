# Haptics System Architecture - Non-Blocking Implementation

## System Flow Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                      MAIN LOOP (DogePet.ino)                    │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │  Touch::update() │  (reads sensors, debounces)
                    └────────┬─────────┘
                             │
                             ▼
                    ┌──────────────────┐
                    │ Events::update() │  (reads touch events)
                    └────────┬─────────┘
                             │
            ┌────────────────┼────────────────┐
            │                │                │
            ▼                ▼                ▼
       TAP Event       HOLD_START Event   HOLD_END Event
            │                │                │
            ▼                ▼                ▼
  Haptics::click()  Haptics::startPurr()  Haptics::stopPurr()
            │                │                │
            └────────────────┼────────────────┘
                             │
                    Sets pattern state:
                    - patternActive = true
                    - patternPhase = 0 (TAP)
                    - patternPhase = 10 (DOUBLE-TAP)
                    - patternPhase = 20 (ALARM)
                    - purring = true/false
                             │
                             ▼
                    ┌──────────────────────┐
                    │ Haptics::purrTick() │ (in main loop)
                    └────────┬─────────────┘
                             │
                    ┌────────▼─────────┐
                    │ patternTick()    │ (main update function)
                    └────────┬─────────┘
                             │
        ┌────────────────────┼────────────────────┐
        │                    │                    │
        ▼                    ▼                    ▼
   TAP Pattern        DOUBLE-TAP Pattern     ALARM Pattern
   (phases 0-2)       (phases 10-15)         (phases 20-25)
   50ms pulse         200ms heartbeat        480ms alternating
        │                    │                    │
        └────────────────────┼────────────────────┘
                             │
                    Intensity scaling:
                    Settings::haptic.intensity (0-255)
                    → getScaledIntensity() → PWM (204-255)
                    (80-100% range for weak motors)
                             │
                             ▼
                    ┌──────────────────┐
                    │ analogWrite()    │ (PWM to motors)
                    │ VIBRO_LEFT=4     │
                    │ VIBRO_RIGHT=3    │
                    └──────────────────┘
```

## Pattern State Phases

```
Pattern Selection (set by Events):
┌────────────────────────────────────────────────┐
│ Haptics::click()       → patternPhase = 0      │
│ Haptics::doubleClick() → patternPhase = 10     │
│ Haptics::alarm()       → patternPhase = 20     │
│ Haptics::startPurr()   → purring = true        │
│ Haptics::stopPurr()    → purring = false       │
└────────────────────────────────────────────────┘

Pattern Execution (in patternTick() each loop):
┌──────────────────────────────────────────────────────────┐
│ PHASE 0: TAP PULSE (50ms)                                │
│ ├─ elapsed < 50ms: Ramp up from 0% to 100%              │
│ └─ elapsed ≥ 50ms: Stop, set patternActive=false         │
│                                                          │
│ PHASE 10-12: DOUBLE-TAP HEARTBEAT (200ms)               │
│ ├─ Phase 10: LUB (60ms) at 95% intensity                │
│ ├─ Phase 11: GAP (50ms) silent                          │
│ ├─ Phase 12: DUB (90ms) at 100% intensity               │
│ └─ Advance phase on timeout, stop when complete         │
│                                                          │
│ PHASE 20: ALARM ALTERNATING (480ms)                     │
│ ├─ Calculate pulseIndex = elapsed / 80                  │
│ ├─ Alternate: Left/Right/Left/Right/Left/Right          │
│ ├─ 6 pulses × 80ms = 480ms total                        │
│ └─ Stop when pulseIndex ≥ 6                            │
│                                                          │
│ PURR: CAT-LIKE RHYTHM (550ms cycle)                     │
│ ├─ Phase 0: 120ms @ 85% intensity                       │
│ ├─ Phase 1: 100ms @ 100% intensity                      │
│ ├─ Phase 2: 110ms @ 85% intensity                       │
│ ├─ Phase 3: 90ms @ 80% intensity                        │
│ ├─ Phase 4: 130ms @ 85% intensity                       │
│ └─ Loop back to Phase 0 (continuous if purring)         │
└──────────────────────────────────────────────────────────┘
```

## Intensity Scaling Formula

```cpp
// Settings::haptic.intensity: 0-255 (from Web UI slider)
// Motor weak → use 80-100% PWM range (204-255)

uint8_t getScaledIntensity(uint8_t settingsLevel) {
    // (settingsLevel / 255) × (80-100%) = settingsLevel/5 + 204
    // settingsLevel=0   → PWM=204 (80%)
    // settingsLevel=128 → PWM=230 (90%)
    // settingsLevel=255 → PWM=255 (100%)
    return 204 + (settingsLevel / 5);
}

// Then apply to patterns:
uint8_t intensity = getScaledIntensity(Settings::haptic.intensity);

// TAP pattern: ramp to full intensity
uint8_t pwm = (intensity * elapsed) / 50;  // 0 → intensity over 50ms

// DOUBLE-TAP LUB: 95% of scaled intensity
uint8_t pwm = (intensity * 95) / 100;

// ALARM: full intensity
uint8_t pwm = intensity;  // (intensity * 100) / 100
```

## Non-Blocking Execution Timeline

```
BEFORE (Blocking):
│Time│ Event   │ Main Loop     │ Face   │ Serial │
├────┼─────────┼───────────────┼────────┼────────┤
│ 0ms│ TAP     │ call click()  │        │        │
│ 1ms│ buzzing │ STALLED 60ms  │ FROZEN │ BLOCKED│
│    │ ......  │ (in delay())  │        │        │
│60ms│ DONE    │ Resume loop   │ Update │ Ready  │

AFTER (Non-Blocking):
│Time│ Event   │ Main Loop    │ Face       │ Serial       │
├────┼─────────┼──────────────┼────────────┼──────────────┤
│ 0ms│ TAP     │ click()      │ animate    │ ready        │
│    │         │ sets state   │            │              │
│ 1ms│ buzzing │ purrTick()   │ animate    │ ready        │
│    │         │ checks phase │            │              │
│ 2ms│ buzzing │ purrTick()   │ animate    │ ready        │
│...│ ...     │ purrTick()   │ animate    │ ready        │
│50ms│ DONE    │ purrTick()   │ animate    │ ready        │
│    │         │ stops motors │            │              │
```

## Key Improvements

1. **No Main Loop Blocking** ✅
   - Before: 60-480ms delays
   - After: ~1ms overhead per loop iteration

2. **Intensity Scaling** ✅
   - Settings awareness
   - 80-100% range for weak motors
   - User control via Web UI slider

3. **Improved Rhythms** ✅
   - TAP: Smooth ramp-up for snappy feel
   - DOUBLE-TAP: Heartbeat (lub-DUB) pattern
   - ALARM: Urgent alternating pulse
   - PURR: Variable intensity cat-like sound

4. **Backward Compatible** ✅
   - `purrTick()` still works (redirects to `patternTick()`)
   - Events.cpp unchanged
   - DogePet.ino unchanged

## Testing the System

1. **Hardware Required**:
   - ESP32-S3 with vibration motors on GPIO 3, 4
   - Touch sensor on GPIO 41 (and 1 if TOUCH_CHIN_ENABLED)

2. **Verify TAP Works**:
   - Press head sensor
   - Should feel single quick pulse (50ms)
   - No main loop stall

3. **Verify HOLD Works**:
   - Hold head sensor for 1+ second
   - Should feel cat purr rhythm
   - Eyes should show happy animation

4. **Verify Intensity**:
   - Open Web UI (http://localhost:5000)
   - Go to "Testing" → "HAPTICS"
   - Adjust "VIBRATION_INTENSITY" slider
   - Tap sensor repeatedly
   - Intensity should change smoothly (204-255 PWM)

5. **Verify No Blocking**:
   - Monitor Serial at 115200 baud
   - Tap sensor repeatedly
   - Serial output should be continuous (no gaps)
   - Face animation should remain smooth
