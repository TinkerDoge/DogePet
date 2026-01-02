// DogePet Web Configuration Implementation
// WiFi AP + WebServer for real-time robot face settings
// Uses built-in ESP32 WebServer (no external dependencies)

#include "include/web_config.h"
#include "FluxGarage_RoboEyes.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

// =============================================================================
// GLOBALS
// =============================================================================
static WebServer server(WEB_PORT);
static roboEyes* pEyes = nullptr;

// Current settings (runtime state)
static struct {
    uint8_t eyeWidth = 28;
    uint8_t eyeHeight = 40;
    uint8_t borderRadius = 8;
    int8_t spacing = 10;
    uint8_t mood = 0;       // 0=Default, 1=Tired, 2=Angry, 3=Happy
    uint8_t position = 0;   // 0=Center, 1-8=N,NE,E,SE,S,SW,W,NW
    bool autoBlink = true;
    bool idleMode = true;
    bool cyclops = false;
    bool curiosity = false;
    bool sweat = false;
    uint8_t blinkInterval = 3;
    uint8_t blinkVariation = 4;
    uint8_t idleInterval = 4;
    uint8_t idleVariation = 5;
    uint8_t hFlickerAmplitude = 2;
    uint8_t vFlickerAmplitude = 10;
    bool hFlicker = false;
    bool vFlicker = false;
} eyeSettings;

// =============================================================================
// HTML PAGE (embedded as raw string)
// =============================================================================
static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DogePet Settings</title>
    <style>
        :root {
            --bg-primary: #0f0f1a;
            --bg-secondary: #1a1a2e;
            --bg-card: #16213e;
            --accent: #e94560;
            --accent-glow: #ff6b6b;
            --text: #eee;
            --text-dim: #888;
            --success: #4ade80;
            --border: #2a2a4a;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: 'Segoe UI', system-ui, sans-serif;
            background: var(--bg-primary);
            color: var(--text);
            min-height: 100vh;
            padding: 20px;
        }
        h1 {
            text-align: center;
            font-size: 2rem;
            margin-bottom: 24px;
            background: linear-gradient(135deg, var(--accent), var(--accent-glow));
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            text-shadow: 0 0 40px rgba(233,69,96,0.3);
        }
        .status {
            text-align: center;
            padding: 8px 16px;
            background: var(--bg-secondary);
            border-radius: 20px;
            margin-bottom: 20px;
            font-size: 0.9rem;
        }
        .status.connected { border: 1px solid var(--success); color: var(--success); }
        .status.error { border: 1px solid var(--accent); color: var(--accent); }
        
        .card {
            background: var(--bg-card);
            border-radius: 16px;
            padding: 20px;
            margin-bottom: 16px;
            border: 1px solid var(--border);
            box-shadow: 0 4px 20px rgba(0,0,0,0.3);
        }
        .card h2 {
            font-size: 1.1rem;
            margin-bottom: 16px;
            color: var(--accent-glow);
            display: flex;
            align-items: center;
            gap: 8px;
        }
        .card h2::before {
            content: '';
            width: 4px;
            height: 20px;
            background: var(--accent);
            border-radius: 2px;
        }
        
        /* Sliders */
        .slider-group { margin-bottom: 16px; }
        .slider-label {
            display: flex;
            justify-content: space-between;
            margin-bottom: 6px;
            font-size: 0.9rem;
        }
        .slider-value {
            color: var(--accent);
            font-weight: bold;
            min-width: 50px;
            text-align: right;
        }
        input[type="range"] {
            width: 100%;
            height: 8px;
            border-radius: 4px;
            background: var(--bg-secondary);
            outline: none;
            -webkit-appearance: none;
        }
        input[type="range"]::-webkit-slider-thumb {
            -webkit-appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: var(--accent);
            cursor: pointer;
            box-shadow: 0 0 10px var(--accent);
        }
        
        /* Radio & Toggle Groups */
        .option-group {
            display: flex;
            flex-wrap: wrap;
            gap: 8px;
            margin-bottom: 12px;
        }
        .option-btn {
            flex: 1;
            min-width: 70px;
            padding: 10px 12px;
            background: var(--bg-secondary);
            border: 2px solid var(--border);
            border-radius: 8px;
            color: var(--text);
            font-size: 0.85rem;
            cursor: pointer;
            text-align: center;
            transition: all 0.2s;
        }
        .option-btn:hover { border-color: var(--accent); }
        .option-btn.active {
            background: var(--accent);
            border-color: var(--accent);
            color: #fff;
            box-shadow: 0 0 15px rgba(233,69,96,0.4);
        }
        
        /* Position Grid */
        .position-grid {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 8px;
            max-width: 200px;
            margin: 0 auto;
        }
        .pos-btn {
            aspect-ratio: 1;
            background: var(--bg-secondary);
            border: 2px solid var(--border);
            border-radius: 8px;
            color: var(--text-dim);
            font-size: 0.75rem;
            cursor: pointer;
            transition: all 0.2s;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .pos-btn:hover { border-color: var(--accent); color: var(--text); }
        .pos-btn.active {
            background: var(--accent);
            border-color: var(--accent);
            color: #fff;
        }
        
        /* Toggle Switches */
        .toggle-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px 0;
            border-bottom: 1px solid var(--border);
        }
        .toggle-row:last-child { border-bottom: none; }
        .toggle {
            position: relative;
            width: 50px;
            height: 26px;
        }
        .toggle input { opacity: 0; width: 0; height: 0; }
        .toggle-slider {
            position: absolute;
            cursor: pointer;
            top: 0; left: 0; right: 0; bottom: 0;
            background: var(--bg-secondary);
            border-radius: 26px;
            transition: 0.3s;
        }
        .toggle-slider::before {
            position: absolute;
            content: '';
            height: 20px;
            width: 20px;
            left: 3px;
            bottom: 3px;
            background: var(--text-dim);
            border-radius: 50%;
            transition: 0.3s;
        }
        .toggle input:checked + .toggle-slider { background: var(--accent); }
        .toggle input:checked + .toggle-slider::before {
            transform: translateX(24px);
            background: #fff;
        }
        
        /* Action Buttons */
        .action-group {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 10px;
        }
        .action-btn {
            padding: 14px 16px;
            background: linear-gradient(135deg, var(--accent), #c73659);
            border: none;
            border-radius: 10px;
            color: #fff;
            font-size: 0.9rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        .action-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(233,69,96,0.4);
        }
        .action-btn:active { transform: translateY(0); }
        
        @media (max-width: 400px) {
            .action-group { grid-template-columns: 1fr; }
            .option-btn { min-width: 60px; font-size: 0.8rem; }
        }
    </style>
</head>
<body>
    <h1>🐕 DogePet Config</h1>
    <div id="status" class="status connected">● Connected</div>
    
    <!-- Eye Geometry -->
    <div class="card">
        <h2>Eye Geometry</h2>
        <div class="slider-group">
            <div class="slider-label">
                <span>Eye Width</span>
                <span class="slider-value" id="widthVal">28px</span>
            </div>
            <input type="range" id="eyeWidth" min="10" max="60" value="28">
        </div>
        <div class="slider-group">
            <div class="slider-label">
                <span>Eye Height</span>
                <span class="slider-value" id="heightVal">40px</span>
            </div>
            <input type="range" id="eyeHeight" min="10" max="60" value="40">
        </div>
        <div class="slider-group">
            <div class="slider-label">
                <span>Border Radius</span>
                <span class="slider-value" id="radiusVal">8px</span>
            </div>
            <input type="range" id="borderRadius" min="0" max="20" value="8">
        </div>
        <div class="slider-group">
            <div class="slider-label">
                <span>Spacing</span>
                <span class="slider-value" id="spacingVal">10px</span>
            </div>
            <input type="range" id="spacing" min="-20" max="40" value="10">
        </div>
    </div>
    
    <!-- Mood -->
    <div class="card">
        <h2>Mood Expression</h2>
        <div class="option-group" id="moodGroup">
            <button class="option-btn active" data-mood="0">Default</button>
            <button class="option-btn" data-mood="1">Tired</button>
            <button class="option-btn" data-mood="2">Angry</button>
            <button class="option-btn" data-mood="3">Happy</button>
        </div>
    </div>
    
    <!-- Position -->
    <div class="card">
        <h2>Eye Position</h2>
        <div class="position-grid" id="posGrid">
            <button class="pos-btn" data-pos="8">NW</button>
            <button class="pos-btn" data-pos="1">N</button>
            <button class="pos-btn" data-pos="2">NE</button>
            <button class="pos-btn" data-pos="7">W</button>
            <button class="pos-btn active" data-pos="0">●</button>
            <button class="pos-btn" data-pos="3">E</button>
            <button class="pos-btn" data-pos="6">SW</button>
            <button class="pos-btn" data-pos="5">S</button>
            <button class="pos-btn" data-pos="4">SE</button>
        </div>
    </div>
    
    <!-- Animations -->
    <div class="card">
        <h2>Animations & Effects</h2>
        <div class="toggle-row">
            <span>Auto Blink</span>
            <label class="toggle">
                <input type="checkbox" id="autoBlink" checked>
                <span class="toggle-slider"></span>
            </label>
        </div>
        <div class="toggle-row">
            <span>Idle Mode</span>
            <label class="toggle">
                <input type="checkbox" id="idleMode" checked>
                <span class="toggle-slider"></span>
            </label>
        </div>
        <div class="toggle-row">
            <span>Cyclops (Single Eye)</span>
            <label class="toggle">
                <input type="checkbox" id="cyclops">
                <span class="toggle-slider"></span>
            </label>
        </div>
        <div class="toggle-row">
            <span>Curiosity Effect</span>
            <label class="toggle">
                <input type="checkbox" id="curiosity">
                <span class="toggle-slider"></span>
            </label>
        </div>
        <div class="toggle-row">
            <span>Sweat Drops</span>
            <label class="toggle">
                <input type="checkbox" id="sweat">
                <span class="toggle-slider"></span>
            </label>
        </div>
    </div>
    
    <!-- Actions -->
    <div class="card">
        <h2>Trigger Animations</h2>
        <div class="action-group">
            <button class="action-btn" onclick="triggerAction('blink')">👁️ Blink</button>
            <button class="action-btn" onclick="triggerAction('laugh')">😆 Laugh</button>
            <button class="action-btn" onclick="triggerAction('confused')">😵 Confused</button>
        </div>
    </div>

    <script>
        const API_BASE = '';
        let debounceTimer = null;
        
        // Slider handlers with debounce
        document.querySelectorAll('input[type="range"]').forEach(slider => {
            slider.addEventListener('input', e => {
                const id = e.target.id;
                const val = e.target.value;
                document.getElementById(id === 'eyeWidth' ? 'widthVal' : 
                    id === 'eyeHeight' ? 'heightVal' :
                    id === 'borderRadius' ? 'radiusVal' : 'spacingVal').textContent = val + 'px';
                
                clearTimeout(debounceTimer);
                debounceTimer = setTimeout(() => sendSettings(), 100);
            });
        });
        
        // Mood buttons
        document.querySelectorAll('#moodGroup .option-btn').forEach(btn => {
            btn.addEventListener('click', e => {
                document.querySelectorAll('#moodGroup .option-btn').forEach(b => b.classList.remove('active'));
                e.target.classList.add('active');
                sendSettings();
            });
        });
        
        // Position grid
        document.querySelectorAll('#posGrid .pos-btn').forEach(btn => {
            btn.addEventListener('click', e => {
                document.querySelectorAll('#posGrid .pos-btn').forEach(b => b.classList.remove('active'));
                e.target.classList.add('active');
                sendSettings();
            });
        });
        
        // Toggle switches
        document.querySelectorAll('.toggle input').forEach(toggle => {
            toggle.addEventListener('change', () => sendSettings());
        });
        
        // Send all settings to device
        async function sendSettings() {
            const data = {
                eyeWidth: parseInt(document.getElementById('eyeWidth').value),
                eyeHeight: parseInt(document.getElementById('eyeHeight').value),
                borderRadius: parseInt(document.getElementById('borderRadius').value),
                spacing: parseInt(document.getElementById('spacing').value),
                mood: parseInt(document.querySelector('#moodGroup .active').dataset.mood),
                position: parseInt(document.querySelector('#posGrid .active').dataset.pos),
                autoBlink: document.getElementById('autoBlink').checked,
                idleMode: document.getElementById('idleMode').checked,
                cyclops: document.getElementById('cyclops').checked,
                curiosity: document.getElementById('curiosity').checked,
                sweat: document.getElementById('sweat').checked
            };
            
            try {
                const resp = await fetch(API_BASE + '/api/eyes', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(data)
                });
                if (!resp.ok) throw new Error('Failed');
                updateStatus(true);
            } catch (e) {
                updateStatus(false);
            }
        }
        
        // Trigger one-shot animations
        async function triggerAction(action) {
            try {
                const resp = await fetch(API_BASE + '/api/action', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ action })
                });
                if (!resp.ok) throw new Error('Failed');
                updateStatus(true);
            } catch (e) {
                updateStatus(false);
            }
        }
        
        // Status indicator
        function updateStatus(connected) {
            const el = document.getElementById('status');
            if (connected) {
                el.className = 'status connected';
                el.textContent = '● Connected';
            } else {
                el.className = 'status error';
                el.textContent = '● Connection Error';
            }
        }
        
        // Load current settings on page load
        async function loadSettings() {
            try {
                const resp = await fetch(API_BASE + '/api/eyes');
                const data = await resp.json();
                
                document.getElementById('eyeWidth').value = data.eyeWidth;
                document.getElementById('eyeHeight').value = data.eyeHeight;
                document.getElementById('borderRadius').value = data.borderRadius;
                document.getElementById('spacing').value = data.spacing;
                
                document.getElementById('widthVal').textContent = data.eyeWidth + 'px';
                document.getElementById('heightVal').textContent = data.eyeHeight + 'px';
                document.getElementById('radiusVal').textContent = data.borderRadius + 'px';
                document.getElementById('spacingVal').textContent = data.spacing + 'px';
                
                document.querySelectorAll('#moodGroup .option-btn').forEach(b => {
                    b.classList.toggle('active', parseInt(b.dataset.mood) === data.mood);
                });
                document.querySelectorAll('#posGrid .pos-btn').forEach(b => {
                    b.classList.toggle('active', parseInt(b.dataset.pos) === data.position);
                });
                
                document.getElementById('autoBlink').checked = data.autoBlink;
                document.getElementById('idleMode').checked = data.idleMode;
                document.getElementById('cyclops').checked = data.cyclops;
                document.getElementById('curiosity').checked = data.curiosity;
                document.getElementById('sweat').checked = data.sweat;
                
                updateStatus(true);
            } catch (e) {
                updateStatus(false);
            }
        }
        
        loadSettings();
    </script>
</body>
</html>
)rawliteral";

// =============================================================================
// API HANDLERS
// =============================================================================

// GET /api/eyes - Return current settings as JSON
static void handleGetEyes() {
    JsonDocument doc;
    doc["eyeWidth"] = eyeSettings.eyeWidth;
    doc["eyeHeight"] = eyeSettings.eyeHeight;
    doc["borderRadius"] = eyeSettings.borderRadius;
    doc["spacing"] = eyeSettings.spacing;
    doc["mood"] = eyeSettings.mood;
    doc["position"] = eyeSettings.position;
    doc["autoBlink"] = eyeSettings.autoBlink;
    doc["idleMode"] = eyeSettings.idleMode;
    doc["cyclops"] = eyeSettings.cyclops;
    doc["curiosity"] = eyeSettings.curiosity;
    doc["sweat"] = eyeSettings.sweat;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// Apply settings to RoboEyes
static void applyEyeSettings() {
    if (!pEyes) return;
    
    // Geometry
    pEyes->setWidth(eyeSettings.eyeWidth, eyeSettings.eyeWidth);
    pEyes->setHeight(eyeSettings.eyeHeight, eyeSettings.eyeHeight);
    pEyes->setBorderradius(eyeSettings.borderRadius, eyeSettings.borderRadius);
    pEyes->setSpacebetween(eyeSettings.spacing);
    
    // Mood
    pEyes->setMood(eyeSettings.mood);
    
    // Position (only set if not center)
    if (eyeSettings.position > 0) {
        pEyes->setPosition(eyeSettings.position);
    }
    
    // Animations
    pEyes->setAutoblinker(eyeSettings.autoBlink, eyeSettings.blinkInterval, eyeSettings.blinkVariation);
    pEyes->setIdleMode(eyeSettings.idleMode, eyeSettings.idleInterval, eyeSettings.idleVariation);
    pEyes->setCyclops(eyeSettings.cyclops);
    pEyes->setCuriosity(eyeSettings.curiosity);
    pEyes->setSweat(eyeSettings.sweat);
}

// POST /api/eyes - Update settings
static void handlePostEyes() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"No body\"}");
        return;
    }
    
    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    
    if (err) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    // Update settings from JSON
    if (doc["eyeWidth"].is<int>()) eyeSettings.eyeWidth = doc["eyeWidth"];
    if (doc["eyeHeight"].is<int>()) eyeSettings.eyeHeight = doc["eyeHeight"];
    if (doc["borderRadius"].is<int>()) eyeSettings.borderRadius = doc["borderRadius"];
    if (doc["spacing"].is<int>()) eyeSettings.spacing = doc["spacing"];
    if (doc["mood"].is<int>()) eyeSettings.mood = doc["mood"];
    if (doc["position"].is<int>()) eyeSettings.position = doc["position"];
    if (doc["autoBlink"].is<bool>()) eyeSettings.autoBlink = doc["autoBlink"];
    if (doc["idleMode"].is<bool>()) eyeSettings.idleMode = doc["idleMode"];
    if (doc["cyclops"].is<bool>()) eyeSettings.cyclops = doc["cyclops"];
    if (doc["curiosity"].is<bool>()) eyeSettings.curiosity = doc["curiosity"];
    if (doc["sweat"].is<bool>()) eyeSettings.sweat = doc["sweat"];
    
    // Apply to RoboEyes
    applyEyeSettings();
    
    Serial.printf("[WebConfig] Settings updated: W=%d H=%d R=%d S=%d M=%d\n",
        eyeSettings.eyeWidth, eyeSettings.eyeHeight, eyeSettings.borderRadius,
        eyeSettings.spacing, eyeSettings.mood);
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// POST /api/action - Trigger one-shot animations
static void handleAction() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"No body\"}");
        return;
    }
    
    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    
    if (err || !doc["action"].is<const char*>()) {
        server.send(400, "application/json", "{\"error\":\"Invalid request\"}");
        return;
    }
    
    String action = doc["action"].as<String>();
    
    if (pEyes) {
        if (action == "blink") {
            pEyes->blink();
            Serial.println("[WebConfig] Action: blink");
        } else if (action == "laugh") {
            pEyes->anim_laugh();
            Serial.println("[WebConfig] Action: laugh");
        } else if (action == "confused") {
            pEyes->anim_confused();
            Serial.println("[WebConfig] Action: confused");
        }
    }
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// Serve the main HTML page
static void handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

// =============================================================================
// PUBLIC FUNCTIONS
// =============================================================================

bool setupWebConfig(roboEyes* eyesPtr) {
    pEyes = eyesPtr;
    
    // Start WiFi Access Point
    Serial.println("\n[WebConfig] Starting WiFi AP...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS);
    
    IPAddress ip = WiFi.softAPIP();
    Serial.printf("[WebConfig] AP SSID: %s\n", AP_SSID);
    Serial.printf("[WebConfig] IP: %s\n", ip.toString().c_str());
    
    // Setup routes
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/eyes", HTTP_GET, handleGetEyes);
    server.on("/api/eyes", HTTP_POST, handlePostEyes);
    server.on("/api/action", HTTP_POST, handleAction);
    
    // Enable CORS for local development
    server.enableCORS(true);
    
    // Start server
    server.begin();
    Serial.println("[WebConfig] Web server started on port 80");
    Serial.printf("[WebConfig] Open http://%s in browser\n", ip.toString().c_str());
    
    return true;
}

void loopWebConfig() {
    server.handleClient();
}
