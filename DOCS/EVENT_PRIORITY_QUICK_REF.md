# Quick Reference: Event Priority System

## The Bug (FIXED)

**Symptom**: Face shaking (`eyes->happy` + `setVFlicker()`) stays on forever when multiple events fire simultaneously.

**Root Cause**: 
- Flicker settings were never explicitly cleared when animations timed out
- Higher-priority events didn't clear lower-priority flicker effects
- Result: Orphaned animation state persists forever

---

## The Fix in 30 Seconds

### Three key changes:

1. **NEW Helper Function** (line 43-63)
   ```cpp
   void clearLowPriorityAnimation(roboEyes* eyes, EventPriority minPriorityToClear) {
       // Clears ALL mood flags + flicker settings from lower-priority events
       eyes->setVFlicker(0, 0);  // ← CRITICAL LINE
       eyes->setHFlicker(0, 0);  // ← CRITICAL LINE
   }
   ```

2. **Timeout handlers NOW clear flicker** (lines ~267, ~280)
   ```cpp
   if (now - shakeStartMs > SHAKE_RESET_MS) {
       eyes->happy = false;
       eyes->setVFlicker(0, 0);  // ← ADDED THIS
   }
   ```

3. **Event handlers NOW preempt lower priorities** (lines ~157, ~281, ~297)
   ```cpp
   if (!furiousShakeTriggered) {
       clearLowPriorityAnimation(eyes, PRIORITY_FURIOUS_SHAKE - 1);  // ← ADDED THIS
       eyes->angry = true;
       // ...
   }
   ```

---

## Priority Quick Chart

| Priority | Event | Clears | Duration |
|----------|-------|--------|----------|
| 3 | FuriousShake | Shake, Tilt, Combo | 500ms |
| 2 | Combo | Shake, Tilt | 2000ms |
| 1 | Shake | Tilt | 800ms |
| 0 | Tilt | (None) | 2000ms |
| — | Still | Everything | Instant |

---

## How to Test

1. **Shake rapidly** (head shake event fires repeatedly)
   - Should NOT get stuck shaking
   - Should reset cleanly

2. **Shake + Combo simultaneously** (touch head + chin while shaking)
   - Shake should be immediately replaced by confused face
   - NO overlapping animations

3. **Furious + Shake at same time** (aggressive movement during shake)
   - Furious should immediately override
   - Old flicker completely cleared

4. **Multiple events in sequence**
   - Tilt → Shake → Still → Tilt (should repeat cleanly)
   - No orphaned animation state

---

## Files Changed

- **[Events.cpp](../libraries/DogePetLib/src/Events.cpp)** — Added preemption logic + flicker cleanup
- **[EVENT_PRIORITY_FIX.md](EVENT_PRIORITY_FIX.md)** — Full technical documentation
- **[EVENT_PRIORITY_ARCHITECTURE.md](EVENT_PRIORITY_ARCHITECTURE.md)** — Visual diagrams

---

## Debugging Hints

If you still see stuck animations:

1. **Check Serial output** (115200 baud) for event messages
2. **Look for missing** `setVFlicker(0,0)` or `setHFlicker(0,0)` calls
3. **Verify** `activeEventPriority` is properly reset to -1
4. **Check** that all 5 mood flags reset together (don't reset just one flag!)

---

## Key Takeaway

**Flicker settings are separate state objects** in RoboEyes and must be explicitly cleared.  
Setting `eyes->happy = false` does NOT automatically call `setVFlicker(0,0)`.

This was the critical missing piece. ✓
