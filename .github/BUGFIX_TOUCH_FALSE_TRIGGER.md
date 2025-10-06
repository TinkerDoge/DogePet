# Touch Sensor False Trigger Fix - October 2025

## Problem Description

**Bug**: TOUCH_DOWN sensor was reading as HIGH by default (when not touched), causing FUNC_BTN alone to trigger BLE toggle countdown instead of requiring FUNC_BTN + TOUCH_DOWN combination.

**Root Causes**:
1. **Hardware issue**: TPS223 touch sensor pin floating without proper pull resistor configuration
2. **Logic bug**: Code defaulted to BLE toggle whenever FUNC_BTN was pressed without checking if TOUCH_DOWN was actually held

## Solutions Applied

### 1. Logic Fix (Primary Issue)
**File**: `DogePet.ino` ~line 807

**Before** (Buggy Logic):
```cpp
if (touchUpHeld && !touchDownHeld) {
  desiredAction = HoldAction::WifiToggle;
} else {
  desiredAction = HoldAction::BleToggle;  // ❌ Defaults to BLE!
}
```

**After** (Fixed Logic):
```cpp
if (touchUpHeld && !touchDownHeld) {
  desiredAction = HoldAction::WifiToggle;
} else if (touchDownHeld && !touchUpHeld) {
  desiredAction = HoldAction::BleToggle;  // ✅ Explicit check required
}
// else: FUNC pressed alone -> no action (prevents false trigger)
```

### 2. Hardware Stabilization
**File**: `DogePet.ino` setup() ~line 1108

**Before**:
```cpp
pinMode(TOUCH_DOWN, INPUT);  // Floating pin - can read random values!
pinMode(TOUCH_UP, INPUT);
```

**After**:
```cpp
pinMode(TOUCH_DOWN, INPUT_PULLDOWN);  // Ensures stable LOW idle state
pinMode(TOUCH_UP, INPUT_PULLDOWN);
```

## Why This Happened

### Floating Pin Issue
When TPS223 is configured for active-HIGH output but the ESP32 pin is set to `INPUT` without pull resistors:
- Pin has **high impedance** (millions of ohms)
- Susceptible to **electromagnetic interference** 
- Can randomly read HIGH or LOW
- **Coupling effect**: Nearby traces (like power lines) can induce voltage

### Logic Assumption Bug
The original code assumed:
> "If user holds FUNC_BTN and it's NOT a WiFi toggle (TOUCH_UP), then it MUST be a BLE toggle"

This failed when neither touch sensor was pressed, causing false BLE toggle triggers.

## Correct Behavior Now

| User Action | Expected Result |
|-------------|----------------|
| FUNC_BTN alone | ✅ Nothing happens |
| FUNC_BTN + TOUCH_DOWN | ✅ BLE toggle countdown |
| FUNC_BTN + TOUCH_UP | ✅ WiFi toggle countdown |
| FUNC_BTN + both touches | ✅ Nothing (squeeze ignored) |

## Testing Verification

### Test 1: FUNC_BTN Alone
```
1. Press and hold FUNC_BTN only
2. Verify: NO countdown overlay appears
3. Release after 3 seconds
4. Verify: BLE state unchanged
✅ PASS if no action occurs
```

### Test 2: FUNC_BTN + TOUCH_DOWN
```
1. Hold FUNC_BTN
2. Touch TOUCH_DOWN sensor
3. Verify: "Hold to toggle BLE" overlay appears
4. Hold for 2 seconds
5. Verify: BLE toggles on/off
✅ PASS if BLE toggles correctly
```

### Test 3: FUNC_BTN + TOUCH_UP
```
1. Hold FUNC_BTN  
2. Touch TOUCH_UP sensor
3. Verify: "Hold to toggle WiFi" overlay appears
4. Hold for 2 seconds
5. Verify: WiFi toggles on/off
✅ PASS if WiFi toggles correctly
```

### Test 4: Touch Sensor Idle State
```
1. Power on device
2. Open Serial Monitor
3. Add debug: Serial.println(digitalRead(TOUCH_DOWN));
4. Verify: Prints 0 (LOW) when not touched
5. Touch sensor
6. Verify: Prints 1 (HIGH) when touched
✅ PASS if idle = LOW, touched = HIGH
```

## Hardware Debugging Tips

If problems persist, check TPS223 module configuration:

### TPS223 Configuration Jumpers/Pads
Most TPS223 modules have solderable pads for:
- **TOG**: Toggle mode (not needed - leave open)
- **AHLB**: Active High/Low select
  - **Bridged**: Active HIGH (our configuration) ✅
  - **Open**: Active LOW

### Expected Voltage Readings
Use multimeter on TPS223 output pin:
- **Not touched**: 0V (GND)
- **Touched**: 3.3V (HIGH)

If readings are inverted or floating (1.5-2V), check:
1. TPS223 power supply (VCC should be 3.3V)
2. Ground connection
3. AHLB jumper configuration

## Files Modified

- ✅ `DogePet.ino` - Logic fix + pinMode change
- ✅ `.github/copilot-instructions.md` - Updated documentation

## Related Issues

This fix also prevents:
- ❌ Accidental BLE toggles when pocket-pressing FUNC_BTN
- ❌ False wake-ups from phantom touch detections  
- ❌ Unreliable gesture detection due to floating pins

---
**Status**: ✅ FIXED  
**Tested**: Pending hardware verification  
**Impact**: High - Core UX improvement
