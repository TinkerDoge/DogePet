/**
 * DogePet Companion App - Frontend JavaScript
 * Handles UI interactions, API calls, and OLED preview rendering
 */

// =============================================================================
// STATE
// =============================================================================
const state = {
    connected: false,
    settings: {
        width: 36,
        height: 36,
        radius: 8,
        spacing: 10,
        mood: 0,
        position: 0,
        autoBlink: true,
        idleMode: true,
        sweat: false,
        curiosity: false,
        cyclops: false
    }
};

// Simulation State for Preview
const simState = {
    lastUpdate: 0,
    // Blink
    nextBlinkTime: 0,
    isBlinking: false,
    blinkEndTime: 0,
    // Idle
    nextIdleTime: 0,
    idlePosition: 0, // Override buffer
    // Sweat
    sweatY: 0,
    sweatDir: 1,
    // Cyclops Blink
    isCyclopsBlinking: false
};

// Tab Switching Logic
document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        // Update tabs
        document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        
        // Update content
        const tabId = btn.dataset.tab;
        document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
        document.getElementById(`tab-${tabId}`).classList.add('active');
    });
});

// Start Screen Handler (Simplified)
document.getElementById('startScreen').addEventListener('click', function() {
    this.style.opacity = '0';
    setTimeout(() => {
        this.style.display = 'none';
        fetchPorts();
    }, 500);
});

// Konami Code
const konami = ['ArrowUp','ArrowUp','ArrowDown','ArrowDown','ArrowLeft','ArrowRight','ArrowLeft','ArrowRight','b','a'];
let kPos = 0;
document.addEventListener('keydown', (e) => {
    if (e.key === konami[kPos]) {
        kPos++;
        if (kPos === konami.length) {
            playSound('start');
            alert('CHEAT CODE ACTIVATED: GOD MODE');
            kPos = 0;
        }
    } else {
        kPos = 0;
    }
});

let debounceTimer = null;
let oledCtx = null;

// =============================================================================
// API HELPER
// =============================================================================
async function fetchPorts() {
    const sel = document.getElementById('portSelect');
    if (!sel) return;
    sel.innerHTML = '<option>SCANNING...</option>';
    try {
        const res = await fetch('/api/ports');
        const data = await res.json();
        const ports = data.ports || [];
        
        sel.innerHTML = '';
        if (ports.length === 0) {
            const opt = document.createElement('option');
            opt.text = "NO DEVICES FOUND";
            sel.add(opt);
            return;
        }
        ports.forEach(p => {
            const opt = document.createElement('option');
            opt.value = p.port;
            opt.text = `${p.port} - ${p.description}`;
            sel.add(opt);
        });
    } catch (e) {
        sel.innerHTML = '<option>ERROR</option>';
    }
}

// =============================================================================
// INITIALIZATION
// =============================================================================
document.addEventListener('DOMContentLoaded', () => {
    // Canvas contexts are initialized in renderPreview
    // Event listeners are attached below
    initCanvas();
    initEventListeners();
    fetchPorts();
    renderPreview();
    
    // Start Animation Loop
    requestAnimationFrame(animationLoop);
    
    // Poll for connection status
    setInterval(checkStatus, 3000);
});

function initCanvas() {
    const canvas = document.getElementById('oledCanvas');
    oledCtx = canvas.getContext('2d');
    oledCtx.imageSmoothingEnabled = false;
}

// Event Listeners
function initEventListeners() {
    // Sliders
    document.querySelectorAll('input[type="range"]').forEach(slider => {
        slider.addEventListener('input', e => {
            const id = e.target.id;
            let val = parseInt(e.target.value);
            
            // Map slider ID to settings key
            let key = id;
            if (id === 'width_slider') key = 'width';
            else if (id === 'height_slider') key = 'height';
            else if (id === 'radius_slider') key = 'radius';
            else if (id === 'spacing_slider') key = 'spacing';

            // Constraint Logic (Screen is 128x64)
            const s = state.settings;
            
            if (key === 'width') {
                if (!s.cyclops) {
                    const maxW = Math.floor((128 - s.spacing) / 2);
                    if (val > maxW) val = maxW;
                } else {
                    if (val > 128) val = 128; // Full screen width for cyclops
                }
            } else if (key === 'spacing') {
                if (!s.cyclops) {
                    const maxSpace = 128 - (2 * s.width);
                    if (val > maxSpace) val = maxSpace;
                }
            } else if (key === 'height') {
                 if (val > 64) val = 64;
            } else if (key === 'radius') {
                const maxR = Math.floor(Math.min(s.width, s.height) / 2);
                if (val > maxR) val = maxR;
            }
            
            // Apply clamped value back to slider if changed
            if (val !== parseInt(e.target.value)) {
                e.target.value = val;
            }

            state.settings[key] = val;
            
            // Update UI label
            const labelEl = document.getElementById(
                id === 'width_slider' ? 'widthVal' : 
                id === 'height_slider' ? 'heightVal' :
                id === 'radius_slider' ? 'radiusVal' : 'spacingVal'
            );
            if (labelEl) labelEl.textContent = val + (id.includes('radius')||id.includes('spacing')?'px':'');
            
            // Debounce send values to avoid flooding serial
            clearTimeout(debounceTimer);
            debounceTimer = setTimeout(() => {
                sendSettings();
            }, 50);

            renderPreview();
        });
    });
    
    // Mood Buttons (Fixed Selector)
    document.querySelectorAll('.mood-btn').forEach(btn => {
        btn.addEventListener('click', e => {
            document.querySelectorAll('.mood-btn').forEach(b => b.classList.remove('active'));
            const target = e.currentTarget; // Use currenTarget to get button even if clicked on child
            target.classList.add('active');
            state.settings.mood = parseInt(target.dataset.value);
            sendSettings();
            renderPreview();
        });
    });
    
    // Position D-Pad (Fixed Selector)
    document.querySelectorAll('.dpad-btn').forEach(btn => {
        btn.addEventListener('click', e => {
            document.querySelectorAll('.dpad-btn').forEach(b => b.classList.remove('active'));
            const target = e.currentTarget;
            target.classList.add('active');
            state.settings.position = parseInt(target.dataset.value);
            sendSettings();
            renderPreview();
        });
    });
    
    // Toggles (Fixed Selector)
    document.querySelectorAll('.toggle-row input[type="checkbox"]').forEach(toggle => {
        toggle.addEventListener('change', e => {
            const id = e.target.id;
            let key = id;
            // Map toggle IDs to keys
            if(id === 'toggle_blink') key = 'autoBlink';
            else if(id === 'toggle_idle') key = 'idleMode';
            else if(id === 'toggle_sweat') key = 'sweat';
            else if(id === 'toggle_curiosity') key = 'curiosity';
            else if(id === 'toggle_cyclops') key = 'cyclops';
            
            state.settings[key] = e.target.checked;
            sendSettings();
            renderPreview();
        });
    });
    
    // Action Buttons
    document.querySelectorAll('.action-btn').forEach(btn => {
        btn.addEventListener('click', e => {
            const action = e.currentTarget.dataset.action;
            triggerAction(action);
        });
    });
    
    // START SCREEN
    const startScreen = document.getElementById('startScreen');
    if (startScreen) {
        document.addEventListener('keydown', () => {
            startScreen.style.opacity = '0';
            setTimeout(() => startScreen.remove(), 500);
        }, { once: true });
        startScreen.addEventListener('click', () => {
            startScreen.style.opacity = '0';
            setTimeout(() => startScreen.remove(), 500);
        }, { once: true });
    }

    // TABS
    const tabs = document.querySelectorAll('.tab-btn');
    const contents = document.querySelectorAll('.tab-content');
    let lastActiveTab = '';

    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
            const newTab = tab.dataset.tab;
            
            // Disable MPU streaming when leaving MPU tab
            if (lastActiveTab === 'mpu' && newTab !== 'mpu') {
                sendAction('mpu_stream_off');
            }
            // Enable MPU streaming when entering MPU tab
            if (newTab === 'mpu' && lastActiveTab !== 'mpu') {
                sendAction('mpu_stream_on');
            }
            
            lastActiveTab = newTab;
            
            tabs.forEach(t => t.classList.remove('active'));
            contents.forEach(c => c.classList.remove('active'));
            
            tab.classList.add('active');
            const target = document.getElementById(`tab-${tab.dataset.tab}`);
            if (target) target.classList.add('active');
        });
    });

    // Global Save Button
    const globalSaveBtn = document.getElementById('globalSaveBtn');
    if (globalSaveBtn) {
        globalSaveBtn.addEventListener('click', async () => {
            if (confirm('Save ALL HARDWARE SETTINGS and REBOOT device?')) {
                // Collect pins from all various tabs
                // Helper to safely get value or default to -1 (unused)
                const val = (id) => {
                    const el = document.getElementById(id);
                    return el && el.value ? parseInt(el.value) : -1;
                };

                const pinout = {
                    sda: val('pin_sda'),
                    scl: val('pin_scl'),
                    // MIC
                    mic_sd: val('pin_mic_sd'),
                    mic_ws: val('pin_mic_ws'),
                    mic_sck: val('pin_mic_sck'),
                    // AMP
                    i2s_do: val('pin_i2s_do'),
                    i2s_bclk: val('pin_i2s_bclk'),
                    i2s_lrc: val('pin_i2s_lrc'),
                    // REST
                    btn: val('pin_btn'), 
                    led: val('pin_led'),
                    vibL: val('pin_vibL'),
                    vibR: val('pin_vibR'),
                    vbat: val('pin_vbat')
                };
                
                try {
                    // Save pinout first
                    await fetch('/api/pinout', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify(pinout)
                    });
                    
                    // Trigger device restart
                    await fetch('/api/action', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ action: 'save_restart' })
                    });
                    
                    alert('Configuration saved. Device restarting...');
                } catch(e) {
                    alert('Error saving configuration');
                }
            }
        });
    }

    // Start polling loop
    pollSensors();

    // Connection Controls
    document.getElementById('refreshBtn').addEventListener('click', fetchPorts);
    document.getElementById('connectBtn').addEventListener('click', toggleConnection);
}

// Dynamic polling loop
function pollSensors() {
    if (!state.connected) {
        setTimeout(pollSensors, 1000);
        return;
    }

    let interval = 500; // Default 2Hz
    
    // MPU Tab - Check for fast poll
    const mpuTab = document.getElementById('tab-mpu');
    if (mpuTab && mpuTab.classList.contains('active')) {
        const fastPoll = document.getElementById('mpu-fast-poll');
        if (fastPoll && fastPoll.checked) {
            interval = 40; // ~25Hz
        }
        updateMPU();
    }
    
    // Other tabs - run if not too fast (limit to 500ms approx check)
    // For simplicity, just run them. 
    // Battery Tab
    if (document.getElementById('tab-bat') && document.getElementById('tab-bat').classList.contains('active')) {
         updateBattery();
    }
    // Mic Tab
    if (document.getElementById('tab-mic') && document.getElementById('tab-mic').classList.contains('active')) {
         updateMic();
    }

    setTimeout(pollSensors, interval);
}



// =============================================================================
// API CALLS & VISUALIZATION
// =============================================================================

async function updateBattery() {
    try {
        const resp = await fetch('/api/sensors?type=bat');
        const data = await resp.json();
        if (data.status === 'ok') {
            const el = document.getElementById('vbat-val');
            const pctEl = document.getElementById('vbat-pct');
            const barEl = document.getElementById('vbat-bar');
            
            // Use dedicated voltage field 'vbat_volts' to avoid collision with pin 'vbat'
            // If missing, fall back to 'vbat' ONLY if float-like (not integer pin 15)
            let v = data.vbat_volts;
            if (typeof v !== 'number' && typeof data.vbat === 'number') {
                if (data.vbat % 1 !== 0) v = data.vbat; // Use if float
            }
            v = v || 0;
            
            if (el) el.textContent = v > 0 ? v.toFixed(2) + ' V' : '-- V';
            
            // Calc percentage (3.2V to 4.2V lipo curve approx)
            let pct = 0;
            if (v > 3.0) {
                pct = Math.min(100, Math.max(0, (v - 3.2) / (4.2 - 3.2) * 100));
            }
            if (pctEl) pctEl.textContent = Math.round(pct) + ' %';
            if (barEl) barEl.style.width = pct + '%';
        }
    } catch(e) {}
}

async function updateMic() {
    try {
        const resp = await fetch('/api/sensors?type=mic');
        const data = await resp.json();
        if (data.status === 'ok') {
            const el = document.getElementById('mic-val');
            if (el) el.textContent = data.mic_db ? data.mic_db.toFixed(1) : '--';
            
            // Visualize Spectrum (Simulated for now based on level)
            const canvas = document.getElementById('micViz');
            if (canvas) {
                const ctx = canvas.getContext('2d');
                const w = canvas.width;
                const h = canvas.height;
                ctx.clearRect(0, 0, w, h);
                
                // Draw 2-channel simulated bars
                const db = data.mic_db || 0;
                const barH = Math.min(h, (db / 100) * h);
                
                ctx.fillStyle = '#ff4757';
                ctx.fillRect(w * 0.25 - 20, h - barH, 40, barH); // Left
                ctx.fillStyle = '#2ed573';
                ctx.fillRect(w * 0.75 - 20, h - barH * 0.9, 40, barH * 0.9); // Right
            }
        }
    } catch(e) {}
}

async function updateMPU() {
    try {
        const resp = await fetch('/api/sensors?type=mpu');
        const data = await resp.json();
        if (data.status === 'ok') {
            // Round for display
            const ax = typeof data.ax === 'number' ? data.ax.toFixed(2) : '--';
            const ay = typeof data.ay === 'number' ? data.ay.toFixed(2) : '--';
            const az = typeof data.az === 'number' ? data.az.toFixed(2) : '--';
            
            if(document.getElementById('acc-x')) document.getElementById('acc-x').innerText = ax;
            if(document.getElementById('acc-y')) document.getElementById('acc-y').innerText = ay;
            if(document.getElementById('acc-z')) document.getElementById('acc-z').innerText = az;
            
            // Gyro
            const gx = typeof data.gx === 'number' ? data.gx.toFixed(1) : '--';
            const gy = typeof data.gy === 'number' ? data.gy.toFixed(1) : '--';
            const gz = typeof data.gz === 'number' ? data.gz.toFixed(1) : '--';

            // 3D Visualization Logic
            const model = document.getElementById('mpu-model');
            if(model) {
                let final_roll = 0, final_pitch = 0, final_yaw = 0;
                let activeMode = "RAW";

                // PREFERRED: DMP Quaternion - check if we have valid quaternion data
                // Valid quaternion has w^2 + x^2 + y^2 + z^2 ≈ 1
                const hasQuaternion = typeof data.qw === 'number' && 
                    (Math.abs(data.qw) < 0.99 || Math.abs(data.qx) > 0.01 || Math.abs(data.qy) > 0.01 || Math.abs(data.qz) > 0.01);
                
                if (hasQuaternion) {
                    activeMode = "DMP";
                    const euler = getEuler({w:data.qw, x:data.qx, y:data.qy, z:data.qz});
                    
                    final_roll = euler.roll + 180; 
                    final_pitch = euler.pitch;               
                    final_yaw = euler.yaw;       

                    // Override Text UI with Euler Angles for visibility
                    if(document.getElementById('acc-x')) {
                         document.getElementById('acc-x').innerText = "R:" + euler.roll.toFixed(1);
                         document.getElementById('acc-x').style.fontSize = "0.8rem";
                    }
                    if(document.getElementById('acc-y')) {
                        document.getElementById('acc-y').innerText = "P:" + euler.pitch.toFixed(1);
                        document.getElementById('acc-y').style.fontSize = "0.8rem";
                    }
                    if(document.getElementById('acc-z')) {
                        document.getElementById('acc-z').innerText = "Y:" + euler.yaw.toFixed(1);
                        document.getElementById('acc-z').style.fontSize = "0.8rem";
                    }
                    
                    // Show "DMP" in Gyro slots
                    if(document.getElementById('gyro-x')) document.getElementById('gyro-x').innerText = "DMP";
                    if(document.getElementById('gyro-y')) document.getElementById('gyro-y').innerText = "ACT";
                    if(document.getElementById('gyro-z')) document.getElementById('gyro-z').innerText = "IVE";

                } else if (typeof data.ax === 'number') {
                    // FALLBACK: Raw Accel
                    activeMode = "RAW";
                    const ax = data.ax; const ay = data.ay; const az = data.az;
                    
                    if(document.getElementById('acc-x')) document.getElementById('acc-x').innerText = ax.toFixed(2);
                    if(document.getElementById('acc-y')) document.getElementById('acc-y').innerText = ay.toFixed(2);
                    if(document.getElementById('acc-z')) document.getElementById('acc-z').innerText = az.toFixed(2);
                    
                    if(document.getElementById('gyro-x')) document.getElementById('gyro-x').innerText = gx;
                    if(document.getElementById('gyro-y')) document.getElementById('gyro-y').innerText = gy;
                    if(document.getElementById('gyro-z')) document.getElementById('gyro-z').innerText = gz;

                    const RAD_TO_DEG = 180 / Math.PI;
                    const calc_roll = Math.atan2(ay, az) * RAD_TO_DEG;
                    const calc_pitch = Math.atan2(-ax, Math.sqrt(ay*ay + az*az)) * RAD_TO_DEG;
                    
                    final_roll = 180 + calc_roll; 
                    final_pitch = 0;
                    final_yaw = calc_pitch;
                }
                
                if (!simState.mpu) simState.mpu = {};
                simState.mpu.targetRoll = final_roll;
                simState.mpu.targetPitch = final_pitch;
                simState.mpu.targetYaw = final_yaw;
            }
        }
    } catch(e) {}
}



async function toggleConnection() {
    const btn = document.getElementById('connectBtn');
    const port = document.getElementById('portSelect').value;
    
    if (!state.connected) {
        if (!port) return alert('Select a port!');
        try {
            const res = await fetch('/api/connect', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify({ port: port })
            });
            const data = await res.json();
            if (data.status === 'ok') {
                state.connected = true;
                btn.textContent = 'DISCONNECT';
                btn.classList.add('connected');
                document.getElementById('status').classList.remove('disconnected');
                document.getElementById('status').classList.add('connected');
                document.getElementById('statusText').textContent = 'SYSTEM ONLINE';
                getPinout(); // Fetch initial pinout on connect
                getConfig(); // Fetch initial config on connect
                if(data.settings && data.settings.status === 'ok') applySettings(data.settings); // Sync settings
                // Show success message
                console.log('Connected to', data.port);
            }
        } catch (e) { alert('Connection Error'); }
    } else {
        await fetch('/api/disconnect', { method: 'POST' });
        state.connected = false;
        btn.textContent = 'CONNECT';
        btn.classList.remove('connected');
        document.getElementById('status').classList.add('disconnected');
        document.getElementById('status').classList.remove('connected');
        document.getElementById('statusText').textContent = 'SYSTEM OFFLINE';
    }
}

async function checkStatus() {
    try {
        const resp = await fetch('/api/status');
        const data = await resp.json();
        setConnected(data.connected);
    } catch (e) {
        setConnected(false);
    }
}

async function sendSettings() {
    if (!state.connected) return;
    
    try {
        await fetch('/api/eyes', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(state.settings)
        });
    } catch (e) {
        console.error('Failed to send settings:', e);
    }
}

async function sendAction(action, value=null) {
    if (!state.connected) return;
    try {
        await fetch('/api/action', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({ action: action, value: value })
        });
    } catch(e) { console.error(e); }
}

// Alias for button handlers
const triggerAction = sendAction;

// Animation stub for visual feedback
function animateBlink() {
    // Simple canvas flash or just let the update loop handle it
    const canvas = document.getElementById('oledCanvas');
    if(canvas) {
        canvas.style.opacity = '0.5';
        setTimeout(() => canvas.style.opacity = '1', 100);
    }
}

async function getPinout() {
    if (!state.connected) return;
    try {
        const resp = await fetch('/api/pinout');
        const data = await resp.json();
        if (data.status === 'ok') {
            const setVal = (id, val) => {
                const el = document.getElementById(id);
                if (el) el.value = (val !== undefined && val !== -1) ? val : '';
            };
            
            setVal('pin_sda', data.sda);
            setVal('pin_scl', data.scl);
            setVal('pin_btn', data.btn);
            setVal('pin_led', data.led);
            // Audio
            setVal('pin_mic_sd', data.mic_sd);
            setVal('pin_mic_ws', data.mic_ws);
            setVal('pin_mic_sck', data.mic_sck);
            setVal('pin_i2s_do', data.i2s_do);
            setVal('pin_i2s_bclk', data.i2s_bclk);
            setVal('pin_i2s_lrc', data.i2s_lrc);
            // Phase 4 extended
            setVal('pin_vibL', data.vibL);
            setVal('pin_vibR', data.vibR);
            setVal('pin_vbat', data.vbat);
        }
    } catch (e) {
        console.error('Failed to get pinout:', e);
    }
}

// Additional init for new inputs
document.addEventListener('DOMContentLoaded', () => {
    // OLED Text Send
    const btnOled = document.getElementById('oledSendTxtBtn');
    if(btnOled) {
        btnOled.addEventListener('click', () => {
            const txt = document.getElementById('oled_text_input').value;
            sendAction('test_oled_text', txt);
        });
    }
    
    // Custom Tone
    const btnTone = document.getElementById('playCustomToneBtn');
    if(btnTone) {
        btnTone.addEventListener('click', () => {
            const freq = document.getElementById('tone_freq').value;
            const dur = document.getElementById('tone_dur').value;
            // Send composite value or new structure? 
            // Simple generic action for now: "freq,dur" string
            sendAction('test_tone_custom', `${freq},${dur}`);
        });
    }
    
    // Config Sliders
    const volumeSlider = document.getElementById('volume_slider');
    if (volumeSlider) {
        volumeSlider.addEventListener('input', (e) => {
            const val = parseInt(e.target.value);
            document.getElementById('volumeVal').textContent = val;
        });
        volumeSlider.addEventListener('change', () => {
            sendConfig({ audioVolume: parseInt(volumeSlider.value) });
        });
    }
    
    const ledBrightnessSlider = document.getElementById('led_brightness_slider');
    if (ledBrightnessSlider) {
        ledBrightnessSlider.addEventListener('input', (e) => {
            const val = parseInt(e.target.value);
            document.getElementById('ledBrightnessVal').textContent = val;
        });
        ledBrightnessSlider.addEventListener('change', () => {
            sendConfig({ ledBrightness: parseInt(ledBrightnessSlider.value) });
        });
    }
});

// Config API Functions
async function getConfig() {
    if (!state.connected) return;
    try {
        const resp = await fetch('/api/config');
        const data = await resp.json();
        if (data.status === 'ok') {
            applyConfig(data);
        }
    } catch (e) {
        console.error('Failed to get config:', e);
    }
}

async function sendConfig(config) {
    if (!state.connected) return;
    try {
        await fetch('/api/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });
    } catch (e) {
        console.error('Failed to send config:', e);
    }
}

function applyConfig(config) {
    if (config.audioVolume !== undefined) {
        const slider = document.getElementById('volume_slider');
        const label = document.getElementById('volumeVal');
        if (slider) slider.value = config.audioVolume;
        if (label) label.textContent = config.audioVolume;
    }
    if (config.ledBrightness !== undefined) {
        const slider = document.getElementById('led_brightness_slider');
        const label = document.getElementById('ledBrightnessVal');
        if (slider) slider.value = config.ledBrightness;
        if (label) label.textContent = config.ledBrightness;
    }
}

// =============================================================================
// EVENT HANDLERS
// =============================================================================
function handleSliderChange(e) {
    const id = e.target.id;
    const value = parseInt(e.target.value);
    
    // Update display value
    document.getElementById(id + 'Val').textContent = value + 'px';
    
    // Update state
    state.settings[id] = value;
    
    // Debounced send
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(() => {
        sendSettings();
    }, 50);
    
    // Update preview
    renderPreview();
}

function handleOptionSelect(e, groupId, settingKey) {
    const group = document.getElementById(groupId);
    group.querySelectorAll('button').forEach(b => b.classList.remove('active'));
    e.target.classList.add('active');
    
    state.settings[settingKey] = parseInt(e.target.dataset.value);
    sendSettings();
    renderPreview();
}

function handleToggleChange(e) {
    state.settings[e.target.id] = e.target.checked;
    sendSettings();
    renderPreview();
}

function handleActionClick(e) {
    const action = e.target.dataset.action;
    triggerAction(action);
    
    // Visual feedback - flash the preview
    if (action === 'blink') {
        animateBlink();
    }
}

// =============================================================================
// UI UPDATES
// =============================================================================
function setConnected(connected) {
    state.connected = connected;
    
    const status = document.getElementById('status');
    const statusText = document.getElementById('statusText');
    const btn = document.getElementById('connectBtn');
    
    if (connected) {
        status.className = 'status connected';
        statusText.textContent = 'Connected';
        btn.textContent = 'Disconnect';
    } else {
        status.className = 'status disconnected';
        statusText.textContent = 'Disconnected';
        btn.textContent = 'Connect';
    }
    
    // Enable/disable controls
    document.querySelectorAll('.action-btn').forEach(btn => {
        btn.disabled = !connected;
    });
}

function applySettings(settings) {
    // Update state
    Object.assign(state.settings, settings);
    
    // Update sliders
    ['width', 'height', 'radius', 'spacing'].forEach(key => {
        if (settings[key] !== undefined) {
            const slider = document.getElementById(key + '_slider');
            const valDisplay = document.getElementById(key + 'Val');
            if (slider) slider.value = settings[key];
            if (valDisplay) valDisplay.textContent = settings[key] + 'px';
        }
    });
    
    // Update mood buttons
    document.querySelectorAll('#moodGroup .option-btn').forEach(btn => {
        btn.classList.toggle('active', parseInt(btn.dataset.value) === settings.mood);
    });
    
    // Update position buttons
    document.querySelectorAll('#posGrid .pos-btn').forEach(btn => {
        btn.classList.toggle('active', parseInt(btn.dataset.value) === settings.position);
    });
    
    // Update toggles
    const toggleMap = {
        'autoBlink': 'toggle_blink',
        'idleMode': 'toggle_idle',
        'sweat': 'toggle_sweat',
        'curiosity': 'toggle_curiosity',
        'cyclops': 'toggle_cyclops'
    };
    
    Object.keys(toggleMap).forEach(key => {
        const id = toggleMap[key];
        const toggle = document.getElementById(id);
        if (toggle && settings[key] !== undefined) {
            toggle.checked = settings[key];
        }
    });
    
    renderPreview();
}

// =============================================================================
// ANIMATION LOOP
// =============================================================================
function animationLoop(timestamp) {
    if (!simState.lastUpdate) simState.lastUpdate = timestamp;
    // Calculate delta time
    const dt = timestamp - simState.lastUpdate;
    simState.lastUpdate = timestamp;
    
    // Smooth Interpolation for 3D Model (Lerp)
    // Factor 0.1 gives smooth motion, adjust as needed
    const lerp = (start, end, amt) => (1 - amt) * start + amt * end;
    
    if (simState.mpu) {
        // Initializes current values if not set
        if (simState.mpu.currRoll === undefined) {
             simState.mpu.currRoll = simState.mpu.targetRoll || 180;
             simState.mpu.currPitch = simState.mpu.targetPitch || 0;
             simState.mpu.currYaw = simState.mpu.targetYaw || 0;
        }

        const targetRoll = simState.mpu.targetRoll !== undefined ? simState.mpu.targetRoll : 180;
        const targetPitch = simState.mpu.targetPitch !== undefined ? simState.mpu.targetPitch : 0;
        const targetYaw = simState.mpu.targetYaw !== undefined ? simState.mpu.targetYaw : 0;

        // Interpolate
        simState.mpu.currRoll = lerp(simState.mpu.currRoll, targetRoll, 0.2);
        simState.mpu.currPitch = lerp(simState.mpu.currPitch, targetPitch, 0.2);
        simState.mpu.currYaw = lerp(simState.mpu.currYaw, targetYaw, 0.2);
        
        // Update Model-Viewer
        const model = document.getElementById('mpu-model');
        if (model) {
            model.orientation = `${simState.mpu.currRoll}deg ${simState.mpu.currPitch}deg ${simState.mpu.currYaw}deg`;
        }
        
        // Update Axis Helper (match orientation)
        const axisBase = document.getElementById('axis-base');
        if (axisBase) {
             // CSS 3D Rotation Order: usually Z Y X or similar.
             // ModelViewer orientation="roll pitch yaw" -> X Y Z extrinsic? or intrinsic?
             // Documentation says: "roll pitch yaw" in degrees.
             // To match Visuals:
             // We need to construct a CSS transform that does the same.
             // Try: rotateX(roll) rotateY(pitch) rotateZ(yaw)?
             // Or rotateZ(yaw) rotateY(pitch) rotateX(roll)?
             // Standard Euler: rotateZ * rotateY * rotateX.
             axisBase.style.transform = `rotateX(${simState.mpu.currRoll}deg) rotateY(${simState.mpu.currPitch}deg) rotateZ(${simState.mpu.currYaw}deg)`; 
        }
    }

    // Existing OLED Animation logic...
    updateSimulation(timestamp);
    renderPreview();
    
    requestAnimationFrame(animationLoop);
}

function updateSimulation(now) {
    const s = state.settings;
    
    // --- BLINK LOGIC ---
    if (s.autoBlink) {
        if (!simState.isBlinking && now > simState.nextBlinkTime) {
            // Start blink
            simState.isBlinking = true;
            simState.blinkEndTime = now + 150; // 150ms blink duration
        } else if (simState.isBlinking && now > simState.blinkEndTime) {
            // End blink
            simState.isBlinking = false;
            // Schedule next (3s +/- random)
            simState.nextBlinkTime = now + 3000 + (Math.random() * 2000);
        }
    } else {
        simState.isBlinking = false;
        simState.nextBlinkTime = now + 1000;
    }
    
    // --- IDLE LOGIC ---
    if (s.idleMode) {
        if (now > simState.nextIdleTime) {
            // Change position
            // Firmware logic: 50% chance center, 50% random
            if (Math.random() > 0.5) {
                simState.idlePosition = 0;
            } else {
                simState.idlePosition = Math.floor(Math.random() * 9);
            }
            simState.nextIdleTime = now + 2000 + (Math.random() * 2000);
        }
    } else {
        simState.idlePosition = s.position; // Follow manual setting
    }
    
    // --- SWEAT ANIMATION ---
    if (s.sweat) {
        // Simple bobbing
        simState.sweatY += 0.2 * simState.sweatDir;
        if (simState.sweatY > 3) simState.sweatDir = -1;
        if (simState.sweatY < 0) simState.sweatDir = 1;
    }
}

// =============================================================================
// OLED PREVIEW RENDERER
// =============================================================================
function renderPreview() {
    const canvas = document.getElementById('oledCanvas');
    const ctx = oledCtx;
    const s = state.settings;
    
    // Determine effective render state
    // Use manual setting if not in idle mode, else use simulated idle position
    // BUT: If user explicitly clicks position, we usually want to see it. 
    // For faithful preview, we show simulated state if Idle is ON.
    const activePos = s.idleMode ? simState.idlePosition : s.position;
    
    // Effective Height (0 if blinking)
    // Cyclops handles blinking separately? No, unified for simplicity unless specific
    let effectiveH = s.height;
    if (simState.isBlinking) effectiveH = 2; // Almost closed
    
    // NATIVE 128x64 resolution (CSS handles visual scaling)
    const screenW = 128;
    const screenH = 64;
    
    // Clear canvas (black background like OLED)
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    
    // Base eye parameters
    let leftEyeW = s.width;
    let leftEyeH = s.height;
    let rightEyeW = s.width;
    let rightEyeH = s.height;
    const radius = s.radius;
    const spacing = s.spacing;
    
    // Update Height for Blink
    leftEyeH = effectiveH;
    rightEyeH = effectiveH;

    // Curiosity effect: outer eye gets larger when looking sideways
    // The further from center, the more size difference
    if (s.curiosity && !s.cyclops && !simState.isBlinking) {
        const posOffsets = {
            0: [0, 0],       // Center - no effect
            1: [0, -10],     // N - no horizontal offset
            2: [15, -10],    // NE - looking right, right eye larger
            3: [15, 0],      // E - looking right, right eye larger
            4: [15, 10],     // SE - looking right, right eye larger
            5: [0, 10],      // S - no horizontal offset
            6: [-15, 10],    // SW - looking left, left eye larger
            7: [-15, 0],     // W - looking left, left eye larger
            8: [-15, -10]    // NW - looking left, left eye larger
        };
        
        const [ox, _] = posOffsets[activePos] || [0, 0];
        
        // Calculate curiosity multiplier based on how far eyes are from center
        // More offset = more size difference (up to 30%)
        const curiosityFactor = Math.abs(ox) / 15; // 0 to 1
        const sizeIncrease = 1 + (curiosityFactor * 0.3); // up to 30% larger
        const sizeDecrease = 1 - (curiosityFactor * 0.15); // up to 15% smaller
        
        if (ox > 0) {
            // Looking right - right eye (outer) gets larger
            rightEyeW = Math.round(s.width * sizeIncrease);
            rightEyeH = Math.round(s.height * sizeIncrease);
            leftEyeW = Math.round(s.width * sizeDecrease);
            leftEyeH = Math.round(s.height * sizeDecrease);
        } else if (ox < 0) {
            // Looking left - left eye (outer) gets larger
            leftEyeW = Math.round(s.width * sizeIncrease);
            leftEyeH = Math.round(s.height * sizeIncrease);
            rightEyeW = Math.round(s.width * sizeDecrease);
            rightEyeH = Math.round(s.height * sizeDecrease);
        }
    }
    
    // Calculate positions (centered)
    let leftEyeX, leftEyeY, rightEyeX, rightEyeY;
    
    if (s.cyclops) {
        // Single eye centered
        leftEyeX = (screenW - leftEyeW) / 2;
        leftEyeY = (screenH - leftEyeH) / 2;
    } else {
        // Two eyes - account for potentially different sizes
        const totalW = leftEyeW + spacing + rightEyeW;
        leftEyeX = (screenW - totalW) / 2;
        leftEyeY = (screenH - leftEyeH) / 2;
        rightEyeX = leftEyeX + leftEyeW + spacing;
        rightEyeY = (screenH - rightEyeH) / 2;
    }
    
    // Apply position offset
    const posOffsets = {
        0: [0, 0],       // Center
        1: [0, -10],     // N
        2: [15, -10],    // NE
        3: [15, 0],      // E
        4: [15, 10],     // SE
        5: [0, 10],      // S
        6: [-15, 10],    // SW
        7: [-15, 0],     // W
        8: [-15, -10]    // NW
    };
    
    const [ox, oy] = posOffsets[activePos] || [0, 0];
    leftEyeX += ox;
    leftEyeY += oy;
    if (!s.cyclops) {
        rightEyeX += ox;
        rightEyeY += oy;
    }
    
    // Clamp to screen
    leftEyeX = Math.max(0, Math.min(screenW - leftEyeW - (s.cyclops ? 0 : rightEyeW + spacing), leftEyeX));
    leftEyeY = Math.max(0, Math.min(screenH - leftEyeH, leftEyeY));
    if (!s.cyclops) {
        rightEyeX = leftEyeX + leftEyeW + spacing;
        rightEyeY = Math.max(0, Math.min(screenH - rightEyeH, rightEyeY));
    }
    
    // Draw eyes (WHITE color for white OLED)
    ctx.fillStyle = '#fff';
    
    // Draw left eye (no scale multiplier - native resolution)
    drawRoundedRect(ctx, leftEyeX, leftEyeY, leftEyeW, leftEyeH, radius);
    
    // Draw right eye (if not cyclops)
    if (!s.cyclops) {
        drawRoundedRect(ctx, rightEyeX, rightEyeY, rightEyeW, rightEyeH, radius);
    }
    
    // Draw mood overlays
    if (s.mood === 1) {
        // Tired - top eyelids
        drawTiredEyelids(ctx, leftEyeX, leftEyeY, leftEyeW, leftEyeH, rightEyeX, rightEyeY, rightEyeW, rightEyeH, s.cyclops);
    } else if (s.mood === 2) {
        // Angry - angled top eyelids
        drawAngryEyelids(ctx, leftEyeX, leftEyeY, leftEyeW, leftEyeH, rightEyeX, rightEyeY, rightEyeW, rightEyeH, s.cyclops);
    } else if (s.mood === 3) {
        // Happy - curved bottom eyelids
        drawHappyEyelids(ctx, leftEyeX, leftEyeY, leftEyeW, leftEyeH, rightEyeX, rightEyeY, rightEyeW, rightEyeH, s.cyclops);
    }
    
    // Draw sweat drops
    if (s.sweat) {
        drawSweatDrops(ctx, simState.sweatY);
    }
}

function drawRoundedRect(ctx, x, y, w, h, r) {
    r = Math.min(r, w / 2, h / 2);
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.lineTo(x + w - r, y);
    ctx.quadraticCurveTo(x + w, y, x + w, y + r);
    ctx.lineTo(x + w, y + h - r);
    ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
    ctx.lineTo(x + r, y + h);
    ctx.quadraticCurveTo(x, y + h, x, y + h - r);
    ctx.lineTo(x, y + r);
    ctx.quadraticCurveTo(x, y, x + r, y);
    ctx.closePath();
    ctx.fill();
}

function drawTiredEyelids(ctx, lx, ly, lw, lh, rx, ry, rw, rh, cyclops) {
    ctx.fillStyle = '#000';
    const leftLidH = lh * 0.4;
    const rightLidH = rh * 0.4;
    
    // Left eye - triangle from top-left to top-right to mid-left
    ctx.beginPath();
    ctx.moveTo(lx, ly);
    ctx.lineTo(lx + lw, ly);
    ctx.lineTo(lx, ly + leftLidH);
    ctx.closePath();
    ctx.fill();
    
    if (!cyclops) {
        // Right eye - triangle from top-left to top-right to mid-right
        ctx.beginPath();
        ctx.moveTo(rx, ry);
        ctx.lineTo(rx + rw, ry);
        ctx.lineTo(rx + rw, ry + rightLidH);
        ctx.closePath();
        ctx.fill();
    }
}

function drawAngryEyelids(ctx, lx, ly, lw, lh, rx, ry, rw, rh, cyclops) {
    ctx.fillStyle = '#000';
    const leftLidH = lh * 0.4;
    const rightLidH = rh * 0.4;
    
    // Left eye - triangle from top-left to top-right to mid-right (opposite of tired)
    ctx.beginPath();
    ctx.moveTo(lx, ly);
    ctx.lineTo(lx + lw, ly);
    ctx.lineTo(lx + lw, ly + leftLidH);
    ctx.closePath();
    ctx.fill();
    
    if (!cyclops) {
        // Right eye - triangle from top-left to top-right to mid-left
        ctx.beginPath();
        ctx.moveTo(rx, ry);
        ctx.lineTo(rx + rw, ry);
        ctx.lineTo(rx, ry + rightLidH);
        ctx.closePath();
        ctx.fill();
    }
}

function drawHappyEyelids(ctx, lx, ly, lw, lh, rx, ry, rw, rh, cyclops) {
    ctx.fillStyle = '#000';
    
    // Draw curved bottom eyelid for left eye (smile shape)
    const leftCurveY = ly + lh * 0.55;  // Start curve from 55% down
    
    ctx.beginPath();
    // Start from bottom-left of eye
    ctx.moveTo(lx - 2, ly + lh + 2);
    // Draw curve across bottom of eye (convex upward = smile)
    ctx.quadraticCurveTo(
        lx + lw / 2,    // Control point X (center)
        leftCurveY,      // Control point Y (pulls curve up)
        lx + lw + 2,     // End X
        ly + lh + 2      // End Y
    );
    // Close the shape by going down and back
    ctx.lineTo(lx + lw + 2, ly + lh + 10);
    ctx.lineTo(lx - 2, ly + lh + 10);
    ctx.closePath();
    ctx.fill();
    
    if (!cyclops) {
        // Draw curved bottom eyelid for right eye
        const rightCurveY = ry + rh * 0.55;
        
        ctx.beginPath();
        ctx.moveTo(rx - 2, ry + rh + 2);
        ctx.quadraticCurveTo(
            rx + rw / 2,
            rightCurveY,
            rx + rw + 2,
            ry + rh + 2
        );
        ctx.lineTo(rx + rw + 2, ry + rh + 10);
        ctx.lineTo(rx - 2, ry + rh + 10);
        ctx.closePath();
        ctx.fill();
    }
}

function drawSweatDrops(ctx, offsetY = 0) {
    ctx.fillStyle = '#fff';  // White for white OLED
    
    // Draw 3 small drops at top with bobbing offset
    const drops = [[20, 8], [64, 5], [108, 10]];
    drops.forEach(([x, y]) => {
        ctx.beginPath();
        ctx.ellipse(x, y + offsetY, 3, 5, 0, 0, Math.PI * 2);
        ctx.fill();
    });
}

function animateBlink() {
    // Trigger simulation blink
    simState.isBlinking = true;
    simState.blinkEndTime = performance.now() + 150;
}

// =============================================================================
// LOGGING
// =============================================================================
async function pollLogs() {
    if (!state.connected || !document.getElementById('term-out')) return;
    
    try {
        const res = await fetch('/api/logs');
        const data = await res.json();
        
        if (data.status === 'ok') {
            const term = document.getElementById('term-out');
            const autoScroll = document.getElementById('auto-scroll').checked;
            
            // Replace content with latest buffer
            term.innerHTML = '';
            
            data.logs.forEach(line => {
                const d = document.createElement('div');
                d.className = 'log-line';
                d.textContent = line;
                term.appendChild(d);
            });
            
            if (autoScroll) {
                term.scrollTop = term.scrollHeight;
            }
        }
    } catch(e) {}
}

// Init Log Polling
document.addEventListener('DOMContentLoaded', () => {
    setInterval(pollLogs, 500);
    
    const clearBtn = document.getElementById('clearLogsBtn');
    if(clearBtn) {
        clearBtn.addEventListener('click', () => {
             const term = document.getElementById('term-out');
             if(term) term.innerHTML = '> LOGS CLEARED...';
        });
    }
});

// Helper: Quaternion to Euler (Degrees)
function getEuler(q) {
    const norm = Math.sqrt(q.w*q.w + q.x*q.x + q.y*q.y + q.z*q.z);
    const w = q.w/norm, x = q.x/norm, y = q.y/norm, z = q.z/norm;

    const sinr_cosp = 2 * (w * x + y * z);
    const cosr_cosp = 1 - 2 * (x * x + y * y);
    const roll = Math.atan2(sinr_cosp, cosr_cosp);

    const sinp = 2 * (w * y - z * x);
    let pitch;
    if (Math.abs(sinp) >= 1) pitch = Math.sign(sinp) * Math.PI / 2; 
    else pitch = Math.asin(sinp);

    const siny_cosp = 2 * (w * z + x * y);
    const cosy_cosp = 1 - 2 * (y * y + z * z);
    const yaw = Math.atan2(siny_cosp, cosy_cosp);

    return {
        roll: roll * (180/Math.PI),
        pitch: pitch * (180/Math.PI),
        yaw: yaw * (180/Math.PI)
    };
}
