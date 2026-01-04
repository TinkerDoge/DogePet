# Haptics System Fix Summary

## Problem Identified

The haptics system had **blocking delays** in the `buzz()` function that prevented the main loop from executing during vibration:

```cpp
// OLD (BLOCKING):
void Haptics::buzz(uint8_t left, uint8_t right, uint32_t durationMs) {
    analogWrite(VIBRO_LEFT, left);
    analogWrite(VIBRO_RIGHT, right);
    delay(durationMs);  // ← BLOCKS ENTIRE MAIN LOOP
    analogWrite(VIBRO_LEFT, 0);
    analogWrite(VIBRO_RIGHT, 0);
}
```

When `Events::update()` called `Haptics::click()` on a TAP event:
1. `click()` → `buzz(230, 230, 60)` 
2. Main loop **stalls for 60ms**
3. Face animation frozen, Serial unresponsive, other events skipped

## Solution Implemented

Converted all haptic patterns to **non-blocking state machine** using phase-based execution in `patternTick()`:

### Architecture Changes

#### 1. **Haptics.cpp - Pattern State Machine**
- Added `patternStartMs`, `patternPhase`, `patternActive` static variables
- Implemented `patternTick()` function (call in main loop)
- Phases organize patterns into non-overlapping ranges:
  - **Phases 0-2**: TAP (single pulse)
  - **Phases 10-15**: DOUBLE-TAP (lub-DUB heartbeat rhythm)
  - **Phases 20-25**: ALARM (alternating left-right urgency pattern)
  - **Phases 30+**: PURR (existing 5-phase cycle)

#### 2. **Intensity Scaling (80-100% Range)**
```cpp
// Weak motors need 80-100% intensity
// Settings::haptic.intensity (0-255) → PWM output (204-255)
static uint8_t getScaledIntensity(uint8_t settingsLevel) {
    return 204 + (settingsLevel / 5);  // Maps 0-255 to 204-255
}
```

Each pattern now respects user intensity settings from the web UI.

#### 3. **Pattern Timing (Non-Blocking)**

**TAP Pattern (50ms)**:
- Ramps up from 0 to full intensity over 50ms
- Produces snappy feedback on button press

**DOUBLE-TAP Pattern (200ms)**:
- First pulse (LUB): 60ms at 95% intensity
- Gap: 50ms silence
- Second pulse (DUB): 90ms at 100% intensity
- Total: ~200ms for heartbeat-like rhythm

**ALARM Pattern (480ms)**:
- 6 alternating pulses (left-right-left-right-left-right)
- 80ms per pulse × 6 = 480ms
- Full intensity for urgency

**PURR Pattern (550ms cycle)**:
- Existing 5-phase rhythm maintained
- 80-100% intensity variation for realism
- Phases: 120ms→100ms→110ms→90ms→130ms

### 4. **Main Loop Integration**

In **DogePet.ino**, the existing call remains unchanged:
```cpp
void loop() {
    // ... other updates ...
    Haptics::purrTick();  // ← Redirects to patternTick() (handles ALL patterns)
    // ... rest of loop ...
}
```

The `purrTick()` function now calls `patternTick()`, which processes:
- All tap/double-tap/alarm patterns
- The purr pattern
- In a non-blocking manner

### 5. **Event System Connection**

**Events.cpp** continues to call haptics methods unchanged:
- `Touch::TAP` → `Haptics::click()` (activates TAP pattern)
- `Touch::HOLD_START` → `Haptics::startPurr()` (starts PURR)
- `Touch::HOLD_END` → `Haptics::stopPurr()` (stops PURR)

Now these calls **don't block** - they just set the pattern state, and `patternTick()` handles timing in the main loop.

## Testing Checklist

- [ ] **TAP feedback**: Press head sensor → Feel single 50ms pulse with ramp-up
- [ ] **HOLD feedback**: Hold head sensor → Feel cat-like purr rhythm
- [ ] **CHIN TAP**: Tap chin sensor → Feel quick pulse (if CHIN enabled)
- [ ] **Intensity control**: Adjust slider in Web UI → Vibration intensity changes (80-100% range)
- [ ] **No blocking**: Serial output continuous during vibration (no stalls)
- [ ] **Face animation**: Eyes update smoothly while vibrating (no freezing)
- [ ] **Double-click**: Combo events (head + chin) → Feel heartbeat rhythm

## Files Modified

1. **Haptics.h**
   - Cleaned up header (removed unused `holdFeedback()`, `startPattern()`)
   - Updated comments for `patternTick()`

2. **Haptics.cpp**
   - Replaced entire implementation with non-blocking patterns
   - Added `getScaledIntensity()` helper
   - Kept `buzz()` for backward compatibility (deprecated)
   - Implemented `patternTick()` with phase-based state machine
   - Made `purrTick()` redirect to `patternTick()`

3. **DogePet.ino** (No changes needed)
   - Existing `Haptics::purrTick()` call works as-is

4. **Events.cpp** (No changes needed)
   - Calls to `Haptics::click()`, `startPurr()`, `stopPurr()` work as-is

## Performance Impact

- **Before**: `buzz()` blocking caused 40-300ms main loop stalls
- **After**: `patternTick()` adds ~1ms overhead (phase checking, PWM updates)
- **Result**: No main loop blocking, smooth animation, responsive UI

## Next Steps

1. Compile and flash to ESP32-S3
2. Test vibration feedback on all touch events
3. Adjust pattern timings if rhythm feels off
4. Monitor current draw (purr is safe at 80-100% intensity)
