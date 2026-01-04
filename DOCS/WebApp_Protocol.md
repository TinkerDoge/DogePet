# DogePet Serial Protocol v3.0 (Unified Config)

This document defines the serial communication protocol between the DogePet Companion App and the ESP32 Firmware.

### Communication Settings
*   **Baud Rate**: `115200`
*   **Format**: JSON strings terminated by newline `\n`.

---

## 1. Commands (PC -> ESP32)

### Get Configuration
Requests the entire runtime settings object from the bot.
**Command:** `get_config`
**Response:** `{"type":"config", "data": { ...settings object... }}`

### Set Configuration
Updates settings and triggers a reboot.
**Command:** `{"cmd":"set_config", ...settings...}`
**Response:** `{"status":"ok", "msg":"Config updated. Rebooting..."}`

### Reboot
Restarts the bot immediately.
**Command:** `reboot`
**Response:** `{"status":"info", "msg":"Rebooting..."}`

### Factory Reset
Wipes NVS settings and restores defaults.
**Command:** `factory_reset`
**Response:** `{"status":"info", "msg":"Resetting to defaults..."}`

---

## 2. Configuration Data Structure
The following fields are supported in the `get_config` / `set_config` JSON payloads:

| Key | Type | Description |
| :--- | :--- | :--- |
| `bot_name` | `string` | Name of the bot (max 31 chars) |
| `eye.width` | `int` | Width of eyes (px) |
| `eye.height` | `int` | Height of eyes (px) |
| `eye.radius` | `int` | Corner radius (px) |
| `eye.spacing` | `int` | Space between eyes (px) |
| `eye.auto_blink` | `bool` | Enable auto blinking |
| `eye.idle_mode` | `bool` | Enable random eye movement |
| `eye.blink_int` | `int` | Avg time between blinks (s) |
| `eye.idle_int` | `int` | Avg time between movements (s) |
| `motion.tilt` | `float` | Tilt threshold (deg) |
| `motion.shake` | `float` | Shake threshold (dps) |
| `motion.furious` | `float` | Furious shake threshold (dps) |
| `motion.tap` | `float` | Tap sensitivity (dps) |
| `power.idle_ms` | `int` | Time to dim (ms) |
| `power.sleep_ms` | `int` | Time to sleep (ms) |
| `volume` | `int` | Global volume (0-100) |
| `haptic_int` | `int` | Haptic intensity (0-255) |

---

## 3. Passive Telemetry (ESP32 -> PC)
The bot sends periodic status updates (passive) or responses to actions.

### System Logs
`{"status":"info|error", "msg":"..."}`

### Boot Info
Sent during startup to the serial monitor.
`{"status":"info", "msg":"I2C Scan..."}`
`{"status":"info", "msg":"Motion Sensor OK"}`