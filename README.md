## DogePet — ESP32-S3 Pocket Companion

A small, lively ESP32-S3–based companion bot with animated RoboEyes, BLE notifications, motion-reactive emotions, and optional Gemini AI integration.

## Quick highlights
- MCU: ESP32-S3 Super Mini (tiny, low-power)
- Display: 128×64 SH1106 OLED (I²C)
- Eyes: RoboEyes animations (blink, look, moods)
- Motion sensing: MPU6050 (accelerometer + gyro)
- Single WS2812 status LED
- BLE + optional WiFi + Gemini AI support

## Features
- Expressive eye animations (idle, blink, gaze, emotions)
- Touch & button controls to switch modes and toggle BLE/WiFi
- Notification display (BLE notifications shown on the OLED)
- Motion-reactive behaviours (tilt = curious, shake = angry, etc.)
- Optional Gemini AI integration for chat-style replies and background "chatter" when WiFi is enabled

## Project structure (important files)
- `DogePet.ino` — main Arduino sketch
- `config.h` — central project configuration (WiFi, Gemini, feature toggles)
- `gemini_ai.*` — Gemini AI client and helpers
- `ai_companion.*`, `animation_engine.*`, `motion.*`, `mpu6050.*` — core systems
- `assets/`, `fonts/`, `include/`, `libraries/` — resources and libs

## Gemini AI (optional)
When enabled, DogePet can send short messages to Google's Gemini API and receive compact, structured responses used to drive animations, toast messages, and sound effects.

Enable in `config.h`:

```cpp
// === Gemini AI Configuration ===
static constexpr bool      ENABLE_GEMINI_AI         = true;  // Enable AI features
static constexpr const char* GEMINI_API_KEY         = "your_api_key_here";    // Set your API key here
static constexpr const char* GEMINI_MODEL           = "gemini-2.5-flash";    // AI model to use (updated for 2.5 Flash)
static constexpr uint32_t  GEMINI_COOLDOWN_MS      = 45000; // Longer cooldown for token efficiency
```

### Token Efficiency Optimizations
- **Shortened prompts**: System prompts reduced from ~300 chars to ~120 chars
- **Compact state info**: Robot state condensed using single-character codes (N/H/A/F/T for moods)
- **Reduced response limits**: Max tokens reduced from 250 to 150
- **Increased cooldowns**: Background chatter interval increased from 30s to 60s
- **Usage tracking**: Added request/error counting for monitoring API usage

Notes:
- Use a short system prompt (see `gemini_ai.h`) to keep responses brief (< 100 chars) for the small OLED
- AI responses are parsed into structured commands (animation, sound effects, toast_message, thought)
- The bot includes rate limiting and background chatter mode (configurable)

## Configuration summary (what to set)
- `ENABLE_WIFI` — required for Gemini AI and background chatter
- `WIFI_SSID`, `WIFI_PASSWORD` — your 2.4GHz network credentials
- `GEMINI_API_KEY` — API key from Google AI Studio
- `GEMINI_MODEL` — pick a model (e.g., `"gemini-1.5-flash"`)

Example WiFi config in `config.h`:
```cpp
static constexpr bool      ENABLE_WIFI              = true;
static constexpr const char* WIFI_SSID              = "your_wifi_network";
static constexpr const char* WIFI_PASSWORD          = "your_wifi_password";
static constexpr uint32_t  WIFI_CONNECT_TIMEOUT_MS = 30000;
```

## Controls & UX
- Touch sensor (TPS223): cycle faces/modes
- FUNC_BTN (active-LOW): short press = action, hold 2s = toggle BLE; double-press = toggle WiFi
- WS2812 LED: shows BLE/WiFi/mood status

Faces / Modes
- Eyes (default): RoboEyes animated, idle wander, blink
- Clock: time/date from BLE ChronosESP32 or local RTC
- Notifications: show last BLE notification with badge for unread

## Wiring (summary)
- OLED SDA/SCL -> pins 9 / 8 (I²C, SH1106 @ 0x3C)
- TPS223 touch -> pin 13 (HIGH on touch)
- FUNC_BTN -> pin 1 (active-LOW)
- WS2812 -> pin 48 (single NeoPixel, 5V with 330Ω inline)
- MPU6050 -> shared I²C (pins 9 / 8)
- I²S DAC (planned) -> BCLK=11, LRC=10, DIN=12

## Required libraries
- FluxGarage_RoboEyes
- Adafruit_GFX + Adafruit_SH110X
- Adafruit_NeoPixel
- ChronosESP32

Install these through the Arduino Library Manager or add them to your project's `libraries/` folder.

## Setup & Flashing
1. Configure `config.h` (WiFi, Gemini, feature toggles).
2. Open `DogePet.ino` in Arduino IDE (select ESP32-S3 board package).
3. Build & upload.
4. On first boot the MPU6050 auto-calibrates — keep the device flat & still.

## Troubleshooting
- "AI Init Failed" — verify `GEMINI_API_KEY`, model name, and WiFi connectivity.
- "WiFi Failed" — check SSID/password and that the network is 2.4GHz.
- No AI response — ensure message format (BLE messages prefixed with `AI:`, `@doge`, or `DogePet:`) and respect cooldown.
- Eyes or audio not working — check RoboEyes init and audio wiring/config.

## Advanced / Developer notes
- Response formatting: `gemini_ai` returns compact JSON-like responses the firmware parses into animation + sound + toast commands.
- To change reply style or length, edit the system prompt in `gemini_ai.h`.
- Background AI chatter can be toggled and temporally spaced to avoid API usage spikes.

## Privacy & Safety
- API keys and WiFi credentials are stored locally on the device in `config.h`. Keep them secure.
- Communications to Gemini are over HTTPS.
- The firmware does not persist conversation history by default.

## Roadmap
- Add audio feedback via MAX98357A (I²S) for sounds/voice
- More expressive face animations (surprised, sleepy)
- Conversation memory / personalized personality
- Desktop configuration app

## License
See `LICENSE` in the project root.
