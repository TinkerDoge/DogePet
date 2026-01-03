
DogePet Serial API Protocol v2.0
This document defines the JSON-based serial communication protocol between the DogePet Companion App (PC) and the ESP32 Firmware.

Communication Settings
Baud Rate: 115200 (default)
Format: JSON strings terminated by newline (\n).
Commands (PC -> ESP32)
1. Connection Handshake
Sent when clicking "CONNECT".

{"cmd": "connect"}
Expected Response:

{"status": "ok", "msg": "DogePet Protected Mode Active"}
2. Pin Configuration
Get Current Pinout
{"cmd": "get_pinout"}
Expected Response:

{
  "status": "ok", 
  "sda": 21, "scl": 22, 
  "btn": 0, "led": 2,
  "vibL": 12, "vibR": 13, "vbat": 34,
  "mic_sd": 32, "mic_ws": 33, "mic_sck": 25,
  "i2s_do": 26, "i2s_bclk": 27, "i2s_lrc": 25
}
Set Pinout (Save to NVS)
{
  "cmd": "set_pinout",
  "sda": 21, "scl": 22,
  "vibL": 12, ... (any subset of pins)
}
Expected Response:

{"status": "ok", "msg": "Pinout saved. Restart required."}
3. Sensor Data Reading
Polled by the app when specific tabs are active.

{"cmd": "get_sensors"}
Expected Response:

{
  "status": "ok",
  "vbat": 3.85,       // Battery Voltage (Float)
  "mic_db": 45.2,     // Mic Volume Level (dB approx)
  "ax": 0.12,         // Accel X (-1.0 to 1.0)
  "ay": -0.05,        // Accel Y
  "az": 0.98          // Accel Z
}
4. Direct Actions & Tests
Used for testing hardware peripherals.

{"cmd": "action", "type": "<ACTION_NAME>", "value": "<OPTIONAL_VALUE>"}
Supported Action Types:
Type	Value	Description
blink
-	Trigger a single blink immediately
laugh
-	Trigger "Laugh" animation (eyes shake up/down)
confused
-	Trigger "Confused" animation (eyes shake left/right)
save_restart	-	Save prefs and reboot device
test_led	-	Toggle LED (if configured)
test_led_on	-	Turn LED ON
test_led_off	-	Turn LED OFF
test_vib_l_on	-	Turn Left Vib Motor ON
test_vib_l_off	-	Turn Left Vib Motor OFF
test_vib_r_on	-	Turn Right Vib Motor ON
test_vib_r_off	-	Turn Right Vib Motor OFF
test_vib_seq1	-	Trigger "Heartbeat" haptic seq
test_vib_seq2	-	Trigger "Alarm" haptic seq
test_oled_pattern	-	Display test pattern on Screen
test_oled_clear	-	Clear Screen
test_oled_text	"Hello"	Display string on Screen
test_tone_on	-	Play 440Hz Tone
test_tone_off	-	Stop Audio
test_tone_custom	"1000,500"	Play 1000Hz for 500ms
5. Eye Configuration
Set various parameters for the eyes.

{"cmd": "get_eyes"}
Expected Response:
{
  "status": "ok",
  "width": 36, "height": 40, "radius": 8, "spacing": 10,
  "mood": 0, "position": 0,
  "autoBlink": true, "idleMode": true, "sweat": false, "curiosity": false, "cyclops": false
}

{"cmd": "set_eyes", "<PARAMETER>": "<VALUE>", ...}
Supported Eye Parameters:
Parameter	Type	Range/Values	Description
width	integer	10 - 128	Width of each eye in pixels
height	integer	10 - 64	Height of each eye in pixels
radius
integer	0 - 20	Corner radius of the eye rectangle
spacing	integer	0 - 50	Distance between eyes in pixels
mood	integer	0-3	0=Default, 1=Tired, 2=Angry, 3=Happy
position	integer	0-8	0=Center, 1=N, 2=NE, 3=E... (Clockwise)
autoBlink	bool	true/false	Enable automatic blinking
idleMode	bool	true/false	Enable random eye movements
sweat	bool	true/false	Enable sweat drops animation
curiosity	bool	true/false	Enable curiosity effect (eye size tracking)
cyclops	bool	true/false	Enable single eye mode
Example
{
  "cmd": "set_eyes",
  "width": 36,
  "height": 40,
  "radius": 8,
  "spacing": 10,
  "mood": 3,
  "position": 0,
  "autoBlink": true,
  "idleMode": true,
  "sweat": false,
  "curiosity": true,
  "cyclops": false
}
Implementation Notes for Firmware
JSON Parsing: The firmware uses ArduinoJson to parse incoming lines.
Non-Blocking: Ensure test commands (like sequences) rely on millis() or freeRTOS tasks if strict non-blocking behavior is needed, though simple delay() is acceptable for test modes.
Persistence: Pin settings are stored in NVS under the "dogepet_hw" namespace.