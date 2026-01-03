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
            const val = parseInt(e.target.value);
            
            // Map slider ID to settings key
            let key = id;
            if (id === 'width_slider') key = 'width';
            else if (id === 'height_slider') key = 'height';
            else if (id === 'radius_slider') key = 'radius';
            else if (id === 'spacing_slider') key = 'spacing';

            state.settings[key] = val;
            
            // Update UI label
            const labelEl = document.getElementById(
                id === 'width_slider' ? 'widthVal' : 
                id === 'height_slider' ? 'heightVal' :
                id === 'radius_slider' ? 'radiusVal' : 'spacingVal'
            );
            if (labelEl) labelEl.textContent = val + (id.includes('radius')||id.includes('spacing')?'px':'');
            
            // Debounce send
            sendSettings();
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

    tabs.forEach(tab => {
        tab.addEventListener('click', () => {
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
                    btn: val('pin_btn'), // Might need to add this back to UI or assume fixed
                    led: val('pin_led'), // Assuming this is now "OLED" tab or similar? Wait, user removed specific LED tab? No, LED usually global. Let's keep looking.
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

    // Tab Loop for sensors (Only poll if active)
    setInterval(() => {
        if (!state.connected) return;
        
        // Battery Tab
        if (document.getElementById('tab-bat') && document.getElementById('tab-bat').classList.contains('active')) {
             updateBattery();
        }
        // Mic Tab
        if (document.getElementById('tab-mic') && document.getElementById('tab-mic').classList.contains('active')) {
             updateMic();
        }
        // MPU Tab
        if (document.getElementById('tab-mpu') && document.getElementById('tab-mpu').classList.contains('active')) {
             updateMPU();
        }
    }, 500); // 2Hz poll for responsiveness

    // Connection Controls
    document.getElementById('refreshBtn').addEventListener('click', fetchPorts);
    document.getElementById('connectBtn').addEventListener('click', toggleConnection);
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
            
            const v = data.vbat || 0;
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
            if(document.getElementById('acc-x')) document.getElementById('acc-x').innerText = data.ax;
            if(document.getElementById('acc-y')) document.getElementById('acc-y').innerText = data.ay;
            if(document.getElementById('acc-z')) document.getElementById('acc-z').innerText = data.az;
            
            // 3D Visualization
            const cube = document.getElementById('mpu-cube');
            if(cube) {
                // Map accelerometer to rotation (rough approx)
                // ax -> rotateY, ay -> rotateX
                const rotX = data.ay * 90; 
                const rotY = data.ax * 90;
                // Maintain center pivot
                cube.style.transform = `translateZ(-100px) rotateX(${rotX}deg) rotateY(${rotY}deg)`;
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
                document.getElementById('statusText').textContent = 'SYSTEM ONLINE';
                getPinout(); // Fetch initial pinout on connect
                if(data.settings) applySettings(data.settings); // Sync settings
                alert(data.msg);
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
});

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
// OLED PREVIEW RENDERER
// =============================================================================
function renderPreview() {
    const canvas = document.getElementById('oledCanvas');
    const ctx = oledCtx;
    const s = state.settings;
    
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
    
    // Curiosity effect: outer eye gets larger when looking sideways
    // The further from center, the more size difference
    if (s.curiosity && !s.cyclops) {
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
        
        const [ox, _] = posOffsets[s.position] || [0, 0];
        
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
    
    const [ox, oy] = posOffsets[s.position] || [0, 0];
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
        drawSweatDrops(ctx);
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

function drawSweatDrops(ctx) {
    ctx.fillStyle = '#fff';  // White for white OLED
    
    // Draw 3 small drops at top
    const drops = [[20, 8], [64, 5], [108, 10]];
    drops.forEach(([x, y]) => {
        ctx.beginPath();
        ctx.ellipse(x, y, 3, 5, 0, 0, Math.PI * 2);
        ctx.fill();
    });
}

function animateBlink() {
    const originalH = state.settings.height;
    
    // Close eyes
    state.settings.height = 2;
    renderPreview();
    
    // Open eyes after delay
    setTimeout(() => {
        state.settings.height = originalH;
        renderPreview();
    }, 150);
}
