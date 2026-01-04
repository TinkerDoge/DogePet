# Bug Fix: Face Animation Stuck When Multiple Events Fire Simultaneously

**Date**: January 4, 2026  
**Status**: FIXED  
**Severity**: High (UX blocker - face shaking stays on forever)

---

## Problem Summary

When multiple motion/touch events fire at the same time, the face shaking animation (`eyes->happy = true` + `eyes->setVFlicker()`) would sometimes **stay on forever** until another unrelated event triggered a face change. This created a stuck animation state that was difficult to recover from.

### Root Cause

The bug had **two layers**:

1. **Missing Flicker Cleanup**: When shake animation finished its timeout, the code reset `eyes->happy = false` but **never cleared** the `setVFlicker(speed, amplitude)` settings. The flicker effect persisted as an orphaned animation state.

2. **Event Priority Collision**: When two events fired simultaneously (e.g., shake + furious shake), the higher-priority event would set its flags but wouldn't explicitly clear the lower-priority animation's flicker settings. This left both animations active simultaneously, causing unpredictable visual behavior.

3. **Incomplete Priority Reset**: The `activeEventPriority` tracking didn't properly clear when animations naturally timed out, allowing stale lower-priority animations to resume.

---

## Event Priority System (FIXED)

A strict 4-tier event priority system now prevents overlapping animations:

| Priority | Event | Behavior | Duration |
|----------|-------|----------|----------|
| **3** (Highest) | FuriousShake | Angry + sweating + jiggling | 500ms |
| **2** (High) | Combo (head + chin) | Confused + horizontal flicker | 2000ms |
| **1** (Medium) | Shake / Tap | Happy + playful | 800ms |
| **0** (Lowest) | Tilt | Curious + subtle | 2000ms (debounced) |

**Rule**: A higher-priority event **immediately clears all lower-priority animations** by forcefully resetting mood flags and flicker settings.

---

## Solution: Event Preemption with Complete State Cleanup

### 1. New Helper Function: `clearLowPriorityAnimation()`

```cpp
static void clearLowPriorityAnimation(roboEyes* eyes, EventPriority minPriorityToClear) {
    if (!eyes) return;
    
    if (activeEventPriority <= minPriorityToClear) {
        // Clear ALL animation flags
        eyes->happy = false;
        eyes->curious = false;
        eyes->sweat = false;
        eyes->angry = false;
        eyes->tired = false;
        
        // CRITICAL: Clear ALL flicker settings
        eyes->setVFlicker(0, 0);  // Vertical flicker
        eyes->setHFlicker(0, 0);  // Horizontal flicker
        
        // Reset triggered flags
        shakeTriggered = false;
        tiltTriggered = false;
        shakeStartMs = 0;
    }
}
```

**Why this works**: Flicker settings are decoupled from mood flags in RoboEyes. Both must be explicitly reset, or the animation persists.

### 2. Updated Animation Timeout Logic

**Before** (BROKEN):
```cpp
if (shakeTriggered && shakeStartMs > 0) {
    if (now - shakeStartMs > SHAKE_RESET_MS) {
        eyes->happy = false;  // Clears flag but...
        eyes->blink();
        shakeTriggered = false;
        // MISSING: setVFlicker(0, 0) ❌
    }
}
```

**After** (FIXED):
```cpp
if (shakeTriggered && shakeStartMs > 0) {
    if (now - shakeStartMs > SHAKE_RESET_MS) {
        eyes->happy = false;
        eyes->setVFlicker(0, 0);  // ✓ NOW clears flicker
        eyes->blink();
        shakeTriggered = false;
        if (activeEventPriority == PRIORITY_SHAKE_TAP) {
            activeEventPriority = (EventPriority)-1;
        }
    }
}
```

### 3. Preemption on Event Trigger

When a higher-priority event fires, it now clears lower-priority animations **first**:

```cpp
case 3:  // FuriousShake (HIGHEST PRIORITY)
    if (!furiousShakeTriggered) {
        // FIRST: Clear all lower-priority animations
        clearLowPriorityAnimation(eyes, PRIORITY_FURIOUS_SHAKE - 1);
        
        // THEN: Set this event's animation state
        eyes->angry = true;
        eyes->sweat = true;
        eyes->confused = true;
        // ... rest of setup
        activeEventPriority = PRIORITY_FURIOUS_SHAKE;
    }
    break;
```

### 4. Complete Reset on Still Event

The "Still" (no motion) event now performs a **complete animation reset**:

```cpp
case 5:  // Still - relax, return to normal
    // Clear EVERY mood flag and flicker setting
    eyes->angry = false;
    eyes->happy = false;
    eyes->curious = false;
    eyes->sweat = false;
    eyes->confused = false;
    eyes->tired = false;
    eyes->setVFlicker(0, 0);
    eyes->setHFlicker(0, 0);
    
    // Reset ALL state flags
    furiousShakeTriggered = false;
    shakeTriggered = false;
    tiltTriggered = false;
    activeEventPriority = (EventPriority)-1;
```

---

## Changes Made

### File: `libraries/DogePetLib/src/Events.cpp`

**Summary of edits**:
1. Added `clearLowPriorityAnimation()` helper function (lines ~43-63)
2. Updated shake animation timeout to call `eyes->setVFlicker(0, 0)` (line ~255)
3. Updated furious shake timeout to call `eyes->setHFlicker(0, 0)` and `eyes->setVFlicker(0, 0)` (line ~265)
4. Added preemption call in Combo handler: `clearLowPriorityAnimation(eyes, PRIORITY_COMBO - 1)` (line ~157)
5. Added preemption call in FuriousShake handler: `clearLowPriorityAnimation(eyes, PRIORITY_FURIOUS_SHAKE - 1)` (line ~297)
6. Added preemption call in Shake handler: `clearLowPriorityAnimation(eyes, PRIORITY_SHAKE_TAP - 1)` (line ~281)
7. Enhanced Still event handler to clear ALL flicker and flags explicitly (lines ~329-350)

---

## Testing Checklist

### Unit Tests
- [ ] Shake animation resets completely after 800ms timeout
- [ ] Furious shake animation resets completely after 500ms timeout
- [ ] When furious shake fires during shake, shake animation is completely cleared
- [ ] When Still fires, all mood flags and flicker settings are reset

### Integration Tests
- [ ] Rapid head shakes (same event multiple times) - no stuck animation
- [ ] Simultaneous head shake + aggressive movement (furious shake)
- [ ] Tap + furious shake at same time
- [ ] Multiple combo touches in sequence
- [ ] Transition from shake → still → shake (animation smooth)

### Edge Cases
- [ ] Power cycle during shake animation
- [ ] Sleep/wake during active animation
- [ ] Touch sensor noise causing multiple rapid events
- [ ] Motion sensor glitch causing rapid event spam

---

## Performance Impact

**None** - All operations are existing API calls, no additional loops or allocations.

---

## Future Improvements

1. **State Machine Refactor**: Consider moving to a full state machine (Mealy/Moore) to eliminate manual flag tracking
2. **Animation Timeline**: Add explicit animation duration tracking instead of separate timeout timers
3. **Mood Composition**: Implement a mood bit-set instead of individual bool flags for cleaner state management

---

## Related Issues

- `Motion::update()` event codes: 1=Tilt, 2=Shake, 3=FuriousShake, 4=Tap, 5=Still
- `Face::getEyes()` returns `roboEyes*` for direct flag manipulation
- `setVFlicker(speed, amplitude)` and `setHFlicker(speed, amplitude)` are persistent settings in FluxGarage_RoboEyes

---

**Fixed by**: GitHub Copilot  
**Verified**: Code inspection and edge case analysis
