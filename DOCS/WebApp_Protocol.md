# DogePet Serial Protocol v4.0

This document defines the serial communication protocol between the DogePet Companion App and the ESP32 Firmware.

## Communication Settings
- **Baud Rate**: `115200`
- **Format**: JSON strings terminated by newline `\n`

---

## 1. Settings Categories

Settings are divided into two categories based on when changes take effect:

### Dynamic Settings (Apply Immediately)
These settings apply immediately without requiring a reboot:

| Category | Fields | Description |
|:---------|:-------|:------------|
| **face** | width, height, radius, spacing, auto_blink, idle_mode, blink_interval, idle_interval, contrast, curious, sweat | Eye appearance, animation, and mood flags |
| **audio** | volume, mic_log | Sound volume and mic logging |
| **haptic** | intensity | Vibration motor strength (0-255, scaled to 80-100% PWM) |
| **led** | brightness, r, g, b | Status LED color and brightness |

### Persistent Settings (Require Reboot)
These settings are saved to NVS and require a reboot to take effect:

| Category | Fields | Description |
|:---------|:-------|:------------|
| **motion** | tilt_deg, shake_angry_dps, shake_furious_dps, tap_spike_dps | Motion detection thresholds |
| **power** | idle_timeout_ms, sleep_timeout_ms | Power management timing |
| **pins** | i2c_sda, i2c_scl, func_btn, touch_chin, i2s_bclk, i2s_lrc, i2s_do, i2s_di, led, vbat, vibro_left, vibro_right | GPIO pin assignments |

---

## 2. Commands (PC → ESP32)

### Get All Settings
Request the complete settings object.

**Command:** `get_config`

**Response:**
```json
{
  "type": "config",
  "data": {
    "bot_name": "DogePet",
    "firmware": "2.0.0",
    "pending_reboot": false,
    "face": {
      "width": 28,
      "height": 40,
      "radius": 8,
      "spacing": 10,
      "auto_blink": true,
      "idle_mode": true,
      "blink_interval": 3,
      "idle_interval": 4,
      "contrast": 255,
      "curious": true,
      "sweat": false
    },
    "audio": {
      "volume": 100,
      "mic_log": true
    },
    "haptic": {
      "intensity": 255
    },
    "led": {
      "brightness": 60,
      "r": 255,
      "g": 100,
      "b": 0
    },
    "motion": {
      "tilt_deg": 20.0,
      "shake_angry_dps": 200.0,
      "shake_furious_dps": 280.0,
      "tap_spike_dps": 140.0
    },
    "power": {
      "idle_timeout_ms": 60000,
      "sleep_timeout_ms": 120000
    },
    "pins": {
      "i2c_sda": 6,
      "i2c_scl": 5,
      "func_btn": 41,
      "touch_chin": 1,
      "i2s_bclk": 17,
      "i2s_lrc": 16,
      "i2s_do": 33,
      "i2s_di": 11,
      "led": 48,
      "vbat": 15,
      "vibro_left": 4,
      "vibro_right": 3
    }
  }
}
```

### Get Dynamic Settings Only
Request only settings that can be changed without reboot.

**Command:** `get_dynamic`

**Response:**
```json
{
  "type": "dynamic",
  "data": {
    "bot_name": "DogePet",
    "face": { ... },
    "audio": { ... },
    "haptic": { ... },
    "led": { ... }
  }
}
```

### Get Persistent Settings Only
Request only settings that require reboot.

**Command:** `get_persistent`

**Response:**
```json
{
  "type": "persistent",
  "data": {
    "pending_reboot": false,
    "motion": { ... },
    "power": { ... },
    "pins": { ... }
  }
}
```

### Set Settings
Update settings (partial updates supported).

**Command:**
```json
{"cmd": "set_config", "face": {"width": 32, "height": 44}}
```

**Response (dynamic change):**
```json
{"status": "ok", "msg": "Dynamic settings applied and saved."}
```

**Response (persistent change):**
```json
{"status": "ok", "msg": "Persistent settings saved. Reboot required."}
```

### Reboot Device
Restart the ESP32 immediately.

**Command:** `reboot`

**Response:**
```json
{"status": "info", "msg": "Rebooting..."}
```

### Factory Reset
Wipe all NVS settings and restore defaults.

**Command:** `factory_reset`

**Response:**
```json
{"status": "info", "msg": "Settings reset to defaults. Reboot required."}
```

---

## 3. Telemetry (ESP32 → PC)

The bot sends periodic status updates and event notifications.

### System Logs
```json
{"status": "info", "msg": "Boot complete"}
{"status": "error", "msg": "MPU6050 not found"}
```

### Events
```json
{"status": "event", "type": "tap", "source": "head"}
{"status": "event", "type": "petting_start", "source": "head"}
{"status": "event", "type": "petting_end", "duration_ms": 3500}
{"status": "event", "type": "wink", "source": "chin"}
{"status": "event", "type": "chin_scratching_start"}
{"status": "event", "type": "chin_scratching_end"}
{"status": "event", "type": "combo_confused"}
{"status": "event", "type": "shake"}
{"status": "event", "type": "wake"}
{"status": "event", "type": "sleep"}
```

**Event Descriptions:**
- `tap`: Quick touch on sensor
- `petting_start`: User holding head sensor (petting mode start)
- `petting_end`: Released head sensor after petting
- `wink`: Quick tap on chin sensor
- `chin_scratching_start`: User holding chin sensor (scratch mode start)
- `chin_scratching_end`: Released chin sensor
- `combo_confused`: Both head and chin held simultaneously (confused expression triggered)

### Sensor Data
```json
{"status": "data", "type": "power", "voltage": 3.85, "percent": 72}
{"status": "data", "type": "mic", "db": 45}
{"status": "data", "type": "motion", "ax": 0.02, "ay": -0.01, "az": 0.98, "gx": 1.2, "gy": -0.5, "gz": 0.1}
```

---

## 4. Companion App Implementation Notes

### Connection Flow
1. Open serial port at 115200 baud
2. Send `get_config` to retrieve current settings
3. Parse response and populate UI
4. Send `set_config` with only changed fields
5. If `pending_reboot` is true, prompt user to reboot

### UI Organization
Recommend organizing settings into tabs:

1. **Appearance** (Dynamic)
   - Eye size (width, height)
   - Eye shape (radius, spacing)
   - Animation (auto_blink, idle_mode)
   - Timing (blink_interval, idle_interval)
   - Display contrast
   - **Mood Flags:**
     - Curious mode (enable inquisitive eye enlargement)
     - Sweat mode (show anxious sweat drops)

2. **Sound & Haptics** (Dynamic)
   - Volume slider (0-100)
   - Haptic intensity slider (0-255, scaled to 80-100% motor PWM)
   - Mic logging toggle

3. **LED** (Dynamic)
   - Brightness slider
   - Color picker (RGB)

4. **Testing** (Dynamic - for preview/diagnostics)
   - Expression tester buttons: Happy, Angry, Tired, Curious, Love, Sweat
   - Haptic pattern tester: Click, Double-Click, Alarm, Purr
   - Audio playback buttons: Chirp, Purr, Surprise, Yawn

5. **Motion Sensitivity** (Persistent ⚠️)
   - Tilt threshold
   - Shake thresholds
   - Tap sensitivity

6. **Power** (Persistent ⚠️)
   - Idle timeout
   - Sleep timeout

7. **Hardware Pins** (Persistent ⚠️)
   - GPIO assignments (advanced users only)

### Visual Indicators
- Show ⚡ icon for dynamic settings (instant apply)
- Show 🔄 icon for persistent settings (reboot required)
- Show banner "Reboot required for changes to take effect" when `pending_reboot` is true

---

## 5. Example Session

```
PC: get_config
ESP: {"type":"config","data":{...}}

PC: {"cmd":"set_config","face":{"width":32}}
ESP: {"status":"ok","msg":"Dynamic settings applied and saved."}

PC: {"cmd":"set_config","motion":{"tilt_deg":25}}
ESP: {"status":"ok","msg":"Persistent settings saved. Reboot required."}

PC: reboot
ESP: {"status":"info","msg":"Rebooting..."}
```

---

**Protocol Version**: 4.0  
**Last Updated**: January 2026
