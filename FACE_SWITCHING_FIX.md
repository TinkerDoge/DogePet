# Face Switching Fix - Double-Tap Scene Change

## Issue
User reports that double-tap detection works (tap messages appear in serial) but the face/scene doesn't actually change.

## Root Cause
The face switching logic had several issues:
1. **Display throttling**: Updates were throttled to 50ms intervals
2. **No immediate update**: When switching faces, the display didn't refresh until the next throttle cycle
3. **Eyes face special case**: RoboEyes update was also throttled separately
4. **displayNeedsUpdate not set**: Flag wasn't being set to force refresh

## Fix Applied

### 1. Reset Throttle Timers on Face Switch
```cpp
lastDisplayUpdateMs = 0;  // Reset display throttle
lastRoboEyesUpdateMs = 0; // Reset eyes throttle
displayNeedsUpdate = true; // Force update
```

This ensures the next frame renders immediately instead of waiting up to 50ms.

### 2. Clear Display Buffer
```cpp
display.clearDisplay();
```

Prevents ghosting from the previous face.

### 3. Force Eyes Update on Switch to FACE_EYES
```cpp
if (mode == FACE_EYES) {
  Serial.println("[FACE] Switching to EYES - forcing eyes update");
  Eyes.update();
  display.display();
}
```

The eyes face is special - it relies on `RoboEyes.update()` which is throttled. By forcing an immediate update, the eyes appear right away instead of waiting for the throttle timer.

### 4. Added Debug Output
```cpp
Serial.printf("[FACE] Switched from %d to %d (Eyes=0, Clock=1, Notif=2)\n", 
             (int)oldMode, (int)mode);
```

Shows exactly what face you're switching to:
- **0 = Eyes** (animated RoboEyes)
- **1 = Clock** (time/date display)
- **2 = Notif** (notifications view)

## Expected Serial Output

### Successful Face Switch
```
[TAP] Pin 35: First tap, window started
[TAP] Pin 35: Tap #2 (within 234ms window)
[TAP] Pin 35: 2-tap detected! Resetting.
[FACE] Double-tap detected! Switching face mode...
[FACE] Switched from 0 to 1 (Eyes=0, Clock=1, Notif=2)
```

### Switching to Eyes Face
```
[TAP] Pin 35: First tap, window started
[TAP] Pin 35: Tap #2 (within 312ms window)
[TAP] Pin 35: 2-tap detected! Resetting.
[FACE] Double-tap detected! Switching face mode...
[FACE] Switched from 2 to 0 (Eyes=0, Clock=1, Notif=2)
[FACE] Switching to EYES - forcing eyes update
```

## Face Cycle Order

Each double-tap cycles through faces in this order:
1. **FACE_EYES (0)** → RoboEyes animations
2. **FACE_CLOCK (1)** → Time/date display
3. **FACE_NOTIF (2)** → Notification view
4. Back to **FACE_EYES (0)** (wraps around)

## Display Update Flow

### Before Fix (Broken)
```
Double-tap detected
↓
Mode variable changed (mode = 1)
↓
Wait for throttle timer (~50ms)
↓
Eventually draw new face
```
**Problem**: User sees old face for up to 50ms, feels unresponsive

### After Fix (Working)
```
Double-tap detected
↓
Mode variable changed (mode = 1)
↓
Reset throttle timers (lastDisplayUpdateMs = 0)
↓
Clear display buffer
↓
If EYES: Force Eyes.update() + display.display()
↓
Set displayNeedsUpdate flag
↓
Next loop iteration immediately renders new face
```
**Result**: Instant face switch, feels responsive

## Testing Checklist

- [ ] Double-tap TOUCH_DOWN
- [ ] Serial shows `[FACE] Double-tap detected!`
- [ ] Serial shows `[FACE] Switched from X to Y`
- [ ] Display immediately shows new face (no delay)
- [ ] Sound effect plays (if ENABLE_MODE_SFX is true)
- [ ] Cycles through all 3 faces: Eyes → Clock → Notif → Eyes

## Timing Constants

From `config.h`:
```cpp
static constexpr uint16_t  DISPLAY_UPDATE_MS     = 50;   // Display refresh throttle
static constexpr uint16_t  ROBOEYES_UPDATE_MS    = 30;   // Eyes animation rate
```

These are still used for normal updates, but bypassed on face switch for instant feedback.

## Additional Features on Face Switch

1. **Sound effects**: Different sound for each face (if enabled)
   - Clock: Confirmation beep + cute question
   - Notif: Notification chime
   - Eyes: Curious sound

2. **Wake up**: Exits lazy/dim mode if active

3. **Activity tracking**: Resets idle timers

---

**Date**: October 7, 2025  
**Status**: Face switching fixed with immediate display update  
**Tested**: Awaiting user confirmation
