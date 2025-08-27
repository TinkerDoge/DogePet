Here’s a structured plan to reorganize DogePet into modular “sectors” and to introduce a status‑driven behaviour layer. The goal is to decouple concerns, make each module easy to maintain, and allow higher‑level logic to adjust parameters based on the bot’s current state.

Proposed sectors
Sector	Responsibilities / interfaces
Brain (Gemini)	Manages AI integration via ai_companion. Provides asynchronous send/receive interface; exposes sendUserMessage() and getLatestResponse(). Maintains chat history, rate limiting and error handling. Emits events such as onAnimationCommand(String), onSoundFxCommand(String), and onThought(String) for other modules to consume.
Motion (MPU6050)	Encapsulates the mpu6050 driver and motion interpreter. Provides raw IMU readings and higher‑level events such as onTilt(float degrees), onShake(), onFuriousShake(), and onTap(). Allows adjustable thresholds (happy/angry/furious) via setters so that status logic can tune sensitivity.
Animations (FluxGarage RoboEyes)	Wraps calls into the FluxGarage_RoboEyes library. Exposes high‑level functions such as setMood(Mood), look(Direction), blink(), openEyes(), sleepEyes(). Receives animation commands from Brain and status manager. Maintains its own update timer to drive idle wandering.
UI (Rendering & Toasts)	Responsible for composing the OLED display: draws eyes (via Animations), clock, notification scroll area and toast overlay. Provides methods showToast(text, duration), showNotification(msg), switchFaceMode(mode). Throttles display refresh via a configurable update interval.
Sensor & Input	Handles low‑level touch and button interrupts. Debounces events and forwards them to the Status Manager (e.g., onModeCycle(), onFuncPress(short/long/double)).
UnderlyingReading	Consolidates “vitals”: battery reading, Wi‑Fi/BLE connectivity, idle timers and lazy mode. Reads VBAT_PIN and caches voltage for UI; monitors WiFi.status() and BLE state; triggers low‑power mode when inactive for LAZY_AFTER_MS.

Each sector should be encapsulated in its own class with a clear API. The main loop would simply call Motion.update(), Brain.update(), UI.update(), etc., and react to events via an event bus or callback mechanism.

Status layer

Introduce a BotStatus state machine that tracks the bot’s overall state. Example statuses might include:

Idle – default eyes mode; periodic blinking and chatter if enabled.

Happy – set when a gentle tilt is detected; eyes brighten, mood held for MOOD_HOLD_MS.

Angry – quick shake; angry eyes and corresponding sound; held for MOOD_HOLD_MS.

Furious – prolonged heavy shaking; furious animation and aggressive sound; held for FURIOUS_HOLD_MS.

Tired – triggered when the bot is inverted or after a long run; droopy eyes and yawning sound.

Curious – AI has just replied with a question; eyes dart, the bot may blink more often.

Sleep – lazy mode; dim display, minimal movement, wakes on tap or loud sound.

Notified – there are unread BLE notifications; display shows a badge, sound plays.

The BotStatus manager would:

Listen for events from sensors, inputs, AI and underlying readings.

Transition between states based on priority and timers (e.g., Furious overrides Happy).

Publish parameters to modules. For example:

Animations: choose mood and idle wander speed; adjust RoboEyes flicker amplitude.

Motion thresholds: in Tired status, lower shake sensitivity so small movements don’t wake it; in Furious, increase damping.

Audio: select different sound profiles; reduce volume in Sleep.

UI: select face mode; hide or dim notifications in Sleep; show toast messages for specific statuses.

A simple configuration table could map each status to parameter values. For example:

Status	Mood code	Idle blink interval	Eye flicker amplitude	Sound volume	Chatty level
Idle	Neutral	normal	none	medium	as set in BOT_TALKATIVE_LEVEL
Happy	Happy	slower	slight horizontal jiggle	high	suppressed until timeout
Angry	Angry	fast	strong horizontal jiggle	medium	suppressed
Furious	Furious	rapid	strong horizontal & vertical jiggle	high	suppressed
Curious	Neutral	normal	none	medium	increased frequency of AI chatter
Tired	Tired	very slow	none	low	suppressed
Sleep	Tired/closed eyes	none	none	muted	off

Transitions back to Idle occur after the corresponding hold durations or when external stimuli end. The status manager can also adjust talkativeness: for example, after a furious episode, disable idle chatter for a cool‑down period.

By breaking the firmware into these clear sectors and introducing a well‑defined status layer, you can more easily extend features and tune behaviours without entangling modules.