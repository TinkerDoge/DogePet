🐾 PetBot v0.1

A small ESP32-S3 based companion bot with animated eyes, notifications, and motion-reactive emotions.
Designed to be lively, expressive, and extendable with sensors & peripherals.

⚡ Hardware Overview

MCU: ESP32-S3 Super Mini 
https://www.espboards.dev/esp32/esp32-s3-super-mini/

Ultra-small size: 22.52 x 18 mm
Ultra-low power consumption: deep sleep power consumption of about 43μA
Onboard WS2812 RGB LED for programmable multi-color status indication
Dual-core Xtensa LX7 CPU running at up to 240 MHz
512 KB SRAM, 384 KB ROM built-in, with 4 MB Flash
Secure encryption features: AES-128/256, RSA, HMAC, digital signatures, and secure startup
11 digital IO pins
22 external interrupt pins
6 analog input pins
11 PWM pins

Display: 128×64 OLED (SH1106, I²C)

Eyes: RoboEyes library renders animated eyes (blinks, idle wander, moods)

Touch: TPS223 capacitive touch module (digital HIGH when touched) → switches modes

Button: FUNC_BTN (active-LOW) → hold 2s to toggle BLE

Status LED: WS2812 (NeoPixel, single pixel) → shows BLE state / mood cues

Audio (planned): I²S DAC/Amp (MAX98357A) for sound playback

Motion Sensor: MPU6050 (I²C) → accelerometer + gyroscope

Used for tilt/gesture detection, mood reactions (happy/angry/blink/jiggle).

🎛 Faces / Modes

Touch sensor cycles between faces:

Eyes (default)

RoboEyes animated, blinking, idle wander

Reacts to tilt (curious), shakes (angry), taps (blink), random jiggle

Clock

Shows time/date from BLE (Chronos) or local RTC

Status bar: BLE state + notif badge

Bottom dock: icons for Eyes / Clock / Notif

Notifications

Shows last BLE-received notification (title + snippet)

Badge in top bar when unread

📡 Connectivity

BLE via ChronosESP32

Syncs time & phone notifications

Toggle on/off with FUNC_BTN hold (2s)

Status LED behavior:

🔵 Blue = BLE enabled

⚫ Off = BLE disabled

🤖 Emotions & Liveliness

Happy: gentle tilt detected

Angry: moderate shake detected

Furious: very intense/continued shaking detected (angry eyes + intense jiggle)

Blink: on tap/bump or random (but NOT when angry or furious)
      Includes smart cooldown system to prevent continuous blinking during rapid movements

Micro-jiggle: subtle flicker motion at random intervals

Note: auto-sleep/tired mode currently disabled (always awake & curious).
REMOVED: "Follow gravity" directional nudges - bot now only reacts to direct user interactions.

🔌 Wiring
Peripheral	ESP32-S3 Pin	Notes
OLED (SDA/SCL)	9 / 8	SH1106, 128×64, I²C addr 0x3C
Touch (TPS223)	13	HIGH on touch, used for face switching
FUNC_BTN	1	Active-LOW, hold 2s → BLE toggle
WS2812 LED	48	Single NeoPixel, 5V power, 330Ω inline
MPU6050 (I²C)	9 / 8	Same I²C bus as OLED
I²S DAC/Amp	(planned)	BCLK=11, LRC=10, DIN=12
🔧 Setup

Flash using Arduino IDE (ESP32 board package + libraries below).

Required Libraries:

FluxGarage_RoboEyes

Adafruit_GFX + Adafruit_SH110X

Adafruit_NeoPixel

ChronosESP32

On first boot, MPU6050 is auto-calibrated (keep device flat & still).

📜 Roadmap / Ideas

Audio feedback with MAX98357A (blips, chirps, voices).

More expressive moods (sleepy, surprised).

Custom animations (wink, happy dance).

Desktop companion app for configuration.