# DogePet Gemini AI Integration

## Overview
Your DogePet can now connect to Google's Gemini AI to act like a true AI companion! The bot can receive text messages via BLE notifications and respond with AI-generated replies, complete with animated reactions and emotions.

## Features
- 🤖 **AI Conversations**: Send messages and get intelligent responses
- 📱 **BLE Integration**: Use any BLE notification app to chat with your bot
- 😊 **Emotional Reactions**: Bot shows happiness, curiosity, or other emotions based on responses
- 🔊 **Audio Feedback**: Sound effects for different AI states
- 💬 **Toast Notifications**: AI responses displayed on the OLED screen
- ⏱️ **Smart Cooldowns**: Prevents spam while allowing natural conversation flow

## Setup Instructions

### Prerequisites
- **WiFi Network**: 2.4GHz WiFi network with internet access
- **BLE App**: Android/iOS app for sending notifications (nRF Connect, LightBlue, etc.)
- **Google Account**: For Gemini API access

### 1. Get a Gemini API Key
1. Go to [Google AI Studio](https://aistudio.google.com/)
2. Sign in with your Google account
3. Create a new API key
4. Copy the API key

### 2. Configure Your Bot
1. Open `config.h` in your project
2. Find the Gemini AI Configuration section (around line 120)
3. Set `ENABLE_GEMINI_AI` to `true`
4. Replace the empty string in `GEMINI_API_KEY` with your actual API key:

```cpp
// === Gemini AI Configuration ===
static constexpr bool      ENABLE_GEMINI_AI         = true;  // Enable AI features
static constexpr const char* GEMINI_API_KEY         = "your_api_key_here";    // Set your API key here
static constexpr const char* GEMINI_MODEL           = "gemini-1.5-flash";    // AI model to use
static constexpr uint32_t  GEMINI_COOLDOWN_MS      = 30030; // Minimum time between AI requests
```

### 3. Set Up WiFi Connection (REQUIRED!)
The bot needs WiFi to connect to the Gemini API. In `config.h`, find the WiFi Configuration section:

```cpp
// === WiFi Configuration ===
static constexpr bool      ENABLE_WIFI              = true;  // Enable WiFi for AI features
static constexpr const char* WIFI_SSID              = "your_wifi_network";    // Set your WiFi network name
static constexpr const char* WIFI_PASSWORD          = "your_wifi_password";    // Set your WiFi password
static constexpr uint32_t  WIFI_CONNECT_TIMEOUT_MS = 30000; // WiFi connection timeout
```

**Important:** Replace `your_wifi_network` and `your_wifi_password` with your actual WiFi credentials.

### 4. Switch AI Models (Optional)
You can easily switch between different Gemini models by changing the `GEMINI_MODEL` constant:

**Available Models:**
- `"gemini-1.5-flash"` - Fast, efficient, good for most tasks (current default)
- `"gemini-1.5-pro"` - More capable, better reasoning, slower responses
- `"gemini-2.5-flash"` - Newer, faster version (if available)
- `"gemini-pro"` - Advanced reasoning (if available)

**To Switch Models:**
1. Open `config.h`
2. Change the `GEMINI_MODEL` value:
   ```cpp
   static constexpr const char* GEMINI_MODEL = "gemini-2.5-flash"; // Switch to newer model
   ```
3. Upload the updated code
4. The bot will automatically use the new model on next AI interaction

### 5. Advanced AI Companion Features

#### Background AI Chatter System
Your DogePet now has a sophisticated AI personality that actively thinks and expresses itself!

**🎭 AI-Driven Robot Control:**
The AI can now control the robot's physical expressions and sounds through structured responses:

**Robot State Awareness:**
- **Current Time**: AI knows what time it is and can react appropriately
- **Mood State**: AI can see the current emotion (happy, angry, tired, etc.)
- **Battery Level**: AI is aware of power status and charging state
- **Recent Notifications**: AI can reference and react to the last notification received
- **Activity Status**: AI knows if the robot is calm, jiggling, shaking, or furious

**🤖 AI Commands Available:**
- **Eye Animations**: blink, look_left, look_right, look_up, look_down, widen, narrow, happy, sad, angry, curious, surprise, thinking, sleepy
- **Sound Effects**: curious, happy, sad, angry, surprise, thinking, chatter, yes, no, notify, blink
- **Toast Messages**: Short messages displayed on screen (under 30 characters)
- **Inner Thoughts**: Longer reflections and observations

**📋 Structured AI Responses:**
The AI responds with JSON-like structures:
```json
{
  "animation": "curious",
  "sound_fx": "thinking",
  "toast_message": "Hmm, what time is it?",
  "thought": "I wonder what my owner is doing right now..."
}
```

**Configuration:**
```cpp
static constexpr bool      ENABLE_AI_CHATTER        = true;  // Enable background AI conversations
static constexpr uint32_t  AI_CHATTER_INTERVAL_MS   = 120000; // 2 minutes between background chatter
```

**🎪 What You'll See:**
- **"Dreaming... 💭"**: Shows when AI is thinking to itself
- **Eye Movements**: Eyes look around, widen, narrow, or show emotions
- **Sound Effects**: Robot makes appropriate sounds for its thoughts
- **Toast Messages**: Short AI messages appear on screen
- **Personality Growth**: AI develops consistent character traits over time

**🧠 AI Scenarios:**
- React to the current time of day
- Comment on current mood/emotion
- Think about recent notifications
- Wonder about owner's activities
- Reflect on purpose as a companion
- Imagine what humans are doing
- Consider learning new things
- Think about favorite interactions
- Wonder about AI companion future
- Reflect on being "alive" as a robot

### 6. Upload and Test
1. Upload the updated code to your DogePet
2. The bot will show connection status on startup:
   - "WiFi Connected! 📶" - WiFi connected successfully
   - "AI Ready! 🤖" - AI initialized and ready
3. If it shows "AI Init Failed" or "WiFi Failed", check your configuration

**🤖 Example AI Interactions:**
- **Morning**: "Good morning! The sun is shining and I'm feeling energetic today!"
- **Afternoon**: Eyes widen and look curious, plays a thinking sound, shows "Wondering about lunch..."
- **Evening**: Mood becomes tired, eyes narrow, plays a gentle sound, shows "Getting sleepy..."
- **Notification Reaction**: "I saw you got a message about 'meeting at 3pm'. I'll help you remember!"
- **Battery Low**: "I'm feeling a bit tired... my battery is getting low."

**🔧 Troubleshooting AI Features:**
- **No AI Responses**: Check WiFi connection and API key in `config.h`
- **Background Chatter Not Working**: Ensure `ENABLE_AI_CHATTER = true` and WiFi is connected
- **Eye Animations Not Working**: Check that RoboEyes library is properly initialized
- **Sound Effects Not Playing**: Verify `silentMode` is off and audio system is working
- **Toast Messages Not Showing**: Check display initialization and toast system

**🧠 AI Personality Development:**
Over time, your DogePet will develop a unique personality based on:
- **Time Patterns**: Learns your daily schedule and routines
- **Interaction History**: Remembers how you respond to its thoughts
- **Notification Types**: Learns about your interests from message content
- **Emotional Patterns**: Adapts its expressions based on your reactions
- **Context Awareness**: Becomes more helpful as it learns about your life

## WiFi Toggle Controls

### Double Press FUNC Button
Your DogePet now has smart WiFi controls for power management:

- **Double press** the FUNC button to toggle WiFi on/off
- **WiFi ON**: Enables internet connectivity and AI features
- **WiFi OFF**: Disables WiFi to save battery power
- **Status Display**: Shows connection status on OLED screen
- **Audio Feedback**: Plays confirmation sound when toggling

### Power Management
- **WiFi OFF** = Maximum battery life (no internet connectivity)
- **WiFi ON** = Full AI functionality (higher power consumption)
- **Auto-reconnect**: Re-enables AI when WiFi is turned back on
- **Smart toggling**: Can be changed anytime without restarting

### Usage Tips
1. **Start with WiFi ON** for full AI functionality
2. **Turn OFF WiFi** when you want to save battery
3. **Turn ON WiFi** when you want to chat with your bot
4. **Check status** via OLED screen messages

## How to Chat with Your AI DogePet

### Using BLE Notifications (Recommended)
1. Download a BLE notification app like "nRF Connect" or "LightBlue" on your phone
2. Connect to your DogePet (device name shows in serial monitor)
3. Send notifications with these prefixes to trigger AI responses:

**Message Formats:**
- `AI: Hello!` - Basic AI message
- `@doge How are you?` - Mention your bot
- `DogePet: What's the weather like?` - Direct addressing

**Example Conversation:**
```
You → AI: Hello there!
DogePet → "Hi! I'm doing great! 😊"

You → @doge Tell me a joke
DogePet → "Why did the robot go on a diet? Because it had too many bytes! 🤖"
```

### What to Expect
- **Response Time**: 2-10 seconds depending on network and API load
- **Response Length**: Kept short (<100 characters) to fit the small screen
- **Personality**: Friendly, playful, and emotive with emojis
- **Emotions**: Bot will react with happy, curious, or other expressions

## Technical Details

### Message Processing
1. BLE notification received with AI prefix
2. WiFi connectivity verified
3. Message sent to Gemini API via HTTPS
4. Response parsed and displayed on OLED
5. Bot shows emotional reaction
6. Cooldown prevents message spam

### Button Controls
- **Single press + hold FUNC button**: Toggle BLE connectivity (existing feature)
- **Double press FUNC button**: Toggle WiFi on/off (new AI feature)
- **Detection window**: 500ms for double press recognition
- **Non-blocking**: Works on all face modes
- **Audio feedback**: Confirmation sounds for all toggles

### Model Configuration
- **Dynamic URL Building**: API URLs built automatically based on configured model
- **Easy Model Switching**: Change `GEMINI_MODEL` in `config.h` to switch models
- **No Code Changes Required**: Model changes only require config update
- **Memory Efficient**: Uses fixed-size buffers for URL construction

### Background Chatter System
- **Prompt Library**: 20+ different thought prompts covering emotions, philosophy, creativity
- **Smart Timing**: Only runs when WiFi is connected and AI is idle
- **Non-intrusive**: Uses different visual indicators than user interactions
- **Personality Building**: Helps the AI develop a consistent character over time
- **Resource Aware**: Respects API rate limits and cooldown periods

### WiFi Requirements
- **Network**: 2.4GHz WiFi (ESP32 limitation)
- **Security**: WPA/WPA2 personal networks supported
- **Connection**: Automatic reconnection on failure
- **Timeout**: 30-second connection timeout
- **Power**: WiFi increases power consumption

### Memory Optimization
- Uses ArduinoJson for efficient JSON parsing
- Fixed-size buffers to prevent memory fragmentation
- Streaming HTTP client for large responses
- Automatic cleanup of unused memory

### Error Handling
- Network timeouts with fallback messages
- WiFi connection status monitoring
- API quota exceeded detection
- Invalid API key detection
- Automatic retry on temporary failures

## Troubleshooting

### "AI Init Failed" Error
- Check that `GEMINI_API_KEY` is set correctly in `config.h`
- Verify the API key is valid and active
- Ensure your ESP32 has internet connectivity
- Check serial monitor for detailed error messages

### "WiFi Failed" Error
- Verify `WIFI_SSID` and `WIFI_PASSWORD` are set correctly in `config.h`
- Check that your WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Ensure the WiFi signal is strong enough
- Check for WiFi network restrictions or captive portals
- The bot shows connection progress with dots in the serial monitor

### "AI: No WiFi" Error
- Your API key is configured but WiFi isn't connected
- Check WiFi credentials and network availability
- The AI feature requires both WiFi and a valid API key

### No Response to AI Messages
- Verify message format (must start with "AI:", "@doge", or "DogePet:")
- Check cooldown timer (30 second minimum between requests)
- Ensure bot is connected via BLE
- Check if API quota is exceeded

### Slow Responses
- API response times vary based on message complexity
- Network latency can affect response times
- Consider upgrading to a paid API plan for faster responses

## Advanced Configuration

### Adjusting Response Length
Modify the system prompt in `gemini_ai.h` to change response characteristics:

```cpp
#define GEMINI_SYSTEM_PROMPT "You are DogePet, a cute robotic companion... Keep your responses short and sweet (under 50 characters)..."
```

### Changing Cooldown Timer
In `config.h`, modify `GEMINI_COOLDOWN_MS`:
- Lower values (10000ms) = More responsive but may hit API limits
- Higher values (60000ms) = More conservative, better for API quotas

### API Rate Limiting
The bot includes built-in rate limiting to prevent API quota exhaustion:
- 30-second cooldown between AI requests
- Automatic error detection for quota exceeded
- Graceful degradation to offline responses

## Future Enhancements
- **Voice Input**: Add speech-to-text for voice conversations
- **Voice Output**: Text-to-speech for AI responses
- **Personality Modes**: Different AI personalities (cheerful, sarcastic, helpful)
- **Conversation Memory**: Context-aware conversations
- **Image Processing**: Send images for AI analysis

## Safety & Privacy
- **Local Storage**: API keys and WiFi credentials stored locally on your device
- **Secure Communication**: Messages sent securely via HTTPS to Google servers
- **No History**: Conversation history is not stored (each message is independent)
- **Network Access**: Bot connects to your WiFi network for internet access
- **Disable Anytime**: You can disable AI features by changing `ENABLE_GEMINI_AI` to `false`

## Support
If you encounter issues:
1. Check the serial monitor for error messages
2. Verify your API key is correct and active
3. Test with simple messages first
4. Check your internet connection stability

Enjoy chatting with your AI DogePet companion! 🤖💬
