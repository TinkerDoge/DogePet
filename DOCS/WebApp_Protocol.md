# DogePet Serial API Protocol v2.0

This document defines the JSON-based serial communication protocol between the DogePet Companion App (PC) and the ESP32 Firmware.

### Communication Settings
*   **Baud Rate**: `115200` (default)
*   **Format**: JSON strings terminated by newline `\n`.

---

## 1. Connection Handshake
Sent when clicking "CONNECT".

**Command (PC -> ESP32):**
```json
{"cmd": "connect"}
```

**Expected Response:**
```json
{"status": "ok", "msg": "DogePet Protected Mode Active"}
```

---

## 2. Pin Configuration

### Get Current Pinout
**Command:**
```json
{"cmd": "get_pinout"}
```
**Response:**
```json
{
  "status": "ok", 
  "sda": 6, "scl": 5, 
  "btn": 41, "led": 48,
  "vibL": 4, "vibR": 3, "vbat": 15,
  "mic_sd": 11, "mic_ws": 16, "mic_sck": 17,
  "i2s_do": 33, "i2s_bclk": 17, "i2s_lrc": 16
}
```

### Set Pinout (Save to NVS)
**Command:**
```json
{
  "cmd": "set_pinout",
  "sda": 6, "scl": 5,
  "vibL": 4, "vibR": 3,
  ... // any subset of pins
}
```
**Response:**
```json
{"status": "ok", "msg": "Pinout saved. Restart required."}
```

---

## 3. Sensor Data Reading
Polled by the app when specific tabs are active.

**Command:**
```json
{"cmd": "get_sensors"}
```

**Response:**
```json
{
  "status": "ok",
  "vbat": 3.85,       // Battery Voltage (Float)
  "mic_db": 45.2,     // Mic Volume Level (dB approx)
  "ax": 0.12,         // Accel X (-1.0 to 1.0)
  "ay": -0.05,        // Accel Y
  "az": 0.98          // Accel Z
}
```

---

## 4. Direct Actions & Tests
Used for testing hardware peripherals.

**Command:**
```json
{"cmd": "action", "type": "<ACTION_NAME>", "value": "<OPTIONAL_VALUE>"}
```

### Supported Action Types

| Type | Value | Description |
| :--- | :--- | :--- |
| `blink` | - | Trigger a single blink immediately |
| `laugh` | - | Trigger "Laugh" animation (eyes shake up/down) |
| `confused` | - | Trigger "Confused" animation (eyes shake left/right) |
| `close` | - | Close eyes (manually) |
| `open` | - | Open eyes (manually) |
| `save_restart` | - | Save prefs and reboot device |
| `test_led` | - | Toggle LED (if configured) |
| `test_led_on` | - | Turn LED ON |
| `test_led_off` | - | Turn LED OFF |
| `test_vib_l_on` | - | Turn Left Vib Motor ON |
| `test_vib_l_off` | - | Turn Left Vib Motor OFF |
| `test_vib_r_on` | - | Turn Right Vib Motor ON |
| `test_vib_r_off` | - | Turn Right Vib Motor OFF |
| `test_vib_seq1` | - | Trigger "Heartbeat" haptic seq |
| `test_vib_seq2` | - | Trigger "Alarm" haptic seq |
| `test_purr_start` | - | Start purr vibration pattern |
| `test_purr_stop` | - | Stop purr vibration pattern |
| `test_sfx_chirp` | - | Play chirp sound |
| `test_sfx_purrr` | - | Play content "purrr" sound |
| `test_sfx_surprise` | - | Play surprise beep |
| `test_sfx_yawn` | - | Play yawn sound |
| `test_oled_pattern`| - | Display test pattern on Screen |
| `test_oled_clear` | - | Clear Screen |
| `test_oled_text` | `"Hello"` | Display string on Screen |
| `test_tone_on` | - | Play 440Hz Tone |
| `test_tone_off` | - | Stop Audio |
| `test_tone_custom` | `"1000,500"` | Play 1000Hz for 500ms |
| `test_mic_read` | - | Log a sample microphone reading to serial |
| `test_mpu_read` | - | Log a sample MPU reading to serial |

---

## 5. Eye Configuration
Set various parameters for the eyes.

### Get Settings
**Command:**
```json
{"cmd": "get_eyes"}
```
**Response:**
```json
{
  "status": "ok",
  "width": 36, "height": 40, "radius": 8, "spacing": 10,
  "mood": 0, "position": 0,
  "autoBlink": true, "idleMode": true, "sweat": false, "curiosity": false, "cyclops": false
}
```

### Set Settings
**Command:**
```json
{"cmd": "set_eyes", "<PARAMETER>": "<VALUE>", ...}
```
**Example:**
```json
{
  "cmd": "set_eyes",
  "width": 36, "height": 40, "radius": 8, "spacing": 10,
  "mood": 3, "position": 0,
  "autoBlink": true, "idleMode": true, "sweat": false, "curiosity": true, "cyclops": false
}
```

### Supported Parameters

| Parameter | Type | Range | Description |
| :--- | :--- | :--- | :--- |
| `width` | `int` | 10 - 128 | Width of each eye in pixels |
| `height` | `int` | 10 - 64 | Height of each eye in pixels |
| `radius` | `int` | 0 - 20 | Corner radius of the eye rectangle |
| `spacing` | `int` | 0 - 50 | Distance between eyes in pixels |
| `mood` | `int` | 0 - 3 | 0=Default, 1=Tired, 2=Angry, 3=Happy |
| `position` | `int` | 0 - 8 | 0=Center, 1=N, 2=NE, 3=E... (Clockwise) |
| `autoBlink` | `bool` | `true`/`false` | Enable automatic blinking |
| `idleMode` | `bool` | `true`/`false` | Enable random eye movements |
| `sweat` | `bool` | `true`/`false` | Enable sweat drops animation |
| `curiosity` | `bool` | `true`/`false` | Enable curiosity effect (eye size tracking) |
| `cyclops` | `bool` | `true`/`false` | Enable single eye mode |

---

## 6. Runtime Configuration

### Get Config
**Command:**
```json
{"cmd": "get_config"}
```
**Response:**
```json
{
  "status": "ok",
  "ledBrightness": 60,
  "audioVolume": 100,
  "audioSampleRate": 22050,
  "displayUpdateMs": 30,
  "btnDebounceMs": 30,
  "deviceName": "DogePet"
}
```

### Set Config
**Command:**
```json
{"cmd": "set_config", "ledBrightness": 80, "audioVolume": 50}
```
**Response:**
```json
{"status": "ok"}
```

### Supported Parameters

| Parameter | Type | Range | Description |
| :--- | :--- | :--- | :--- |
| `ledBrightness` | `int` | 0 - 255 | NeoPixel LED brightness |
| `audioVolume` | `int` | 0 - 100 | Audio output volume % |
| `audioSampleRate` | `int` | - | Audio sample rate (Hz) |
| `displayUpdateMs` | `int` | - | Display update interval (ms) |
| `btnDebounceMs` | `int` | - | Button debounce time (ms) |
| `deviceName` | `string` | - | Device name (max 31 chars) |

---

## Implementation Notes
*   **JSON Parsing**: The firmware uses `ArduinoJson` to parse incoming lines.
*   **Non-Blocking**: Ensure test commands (like sequences) rely on `millis()` or freeRTOS tasks if strict non-blocking behavior is needed, though simple `delay()` is acceptable for test modes.
*   **Persistence**: Pin settings are stored in NVS under the `dogepet_hw` namespace.
*   **Runtime Config**: Config settings are stored in NVS under the `dogepet_cfg` namespace.