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
        width: 28,
        height: 40,
        radius: 8,
        spacing: 10,
        mood: 0,
        position: 0,
        autoBlink: true,
        idleMode: true,
        cyclops: false,
        curiosity: false,
        sweat: false
    }
};

let debounceTimer = null;
let oledCtx = null;

// =============================================================================
// INITIALIZATION
// =============================================================================
document.addEventListener('DOMContentLoaded', () => {
    initCanvas();
    initEventListeners();
    refreshPorts();
    renderPreview();
    
    // Poll for connection status
    setInterval(checkStatus, 3000);
});

function initCanvas() {
    const canvas = document.getElementById('oledCanvas');
    oledCtx = canvas.getContext('2d');
    oledCtx.imageSmoothingEnabled = false;
}

function initEventListeners() {
    // Refresh ports button
    document.getElementById('refreshBtn').addEventListener('click', refreshPorts);
    
    // Connect button
    document.getElementById('connectBtn').addEventListener('click', toggleConnection);
    
    // Sliders
    document.querySelectorAll('input[type="range"]').forEach(slider => {
        slider.addEventListener('input', handleSliderChange);
    });
    
    // Mood buttons
    document.querySelectorAll('#moodGroup .option-btn').forEach(btn => {
        btn.addEventListener('click', (e) => handleOptionSelect(e, 'moodGroup', 'mood'));
    });
    
    // Position buttons
    document.querySelectorAll('#posGrid .pos-btn').forEach(btn => {
        btn.addEventListener('click', (e) => handleOptionSelect(e, 'posGrid', 'position'));
    });
    
    // Toggles
    document.querySelectorAll('.toggle input').forEach(toggle => {
        toggle.addEventListener('change', handleToggleChange);
    });
    
    // Action buttons
    document.querySelectorAll('.action-btn').forEach(btn => {
        btn.addEventListener('click', handleActionClick);
    });
}

// =============================================================================
// API CALLS
// =============================================================================
async function refreshPorts() {
    try {
        const resp = await fetch('/api/ports');
        const data = await resp.json();
        
        const select = document.getElementById('portSelect');
        select.innerHTML = '<option value="">Select COM Port...</option>';
        
        data.ports.forEach(port => {
            const option = document.createElement('option');
            option.value = port.port;
            option.textContent = `${port.port} - ${port.description}`;
            if (port.port === data.suggested) {
                option.textContent += ' (ESP32)';
            }
            select.appendChild(option);
        });
        
        // Auto-select suggested port
        if (data.suggested) {
            select.value = data.suggested;
        }
        
        // Update connection state
        if (data.connected) {
            select.value = data.current_port;
            setConnected(true);
        }
    } catch (e) {
        console.error('Failed to refresh ports:', e);
    }
}

async function toggleConnection() {
    const btn = document.getElementById('connectBtn');
    
    if (state.connected) {
        // Disconnect
        try {
            await fetch('/api/disconnect', { method: 'POST' });
            setConnected(false);
        } catch (e) {
            console.error('Disconnect failed:', e);
        }
    } else {
        // Connect
        const port = document.getElementById('portSelect').value;
        btn.textContent = 'Connecting...';
        btn.disabled = true;
        
        try {
            const resp = await fetch('/api/connect', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ port })
            });
            const data = await resp.json();
            
            if (data.status === 'ok') {
                setConnected(true);
                if (data.settings) {
                    applySettings(data.settings);
                }
            } else {
                alert('Connection failed: ' + (data.msg || 'Unknown error'));
            }
        } catch (e) {
            console.error('Connect failed:', e);
            alert('Connection failed');
        }
        
        btn.disabled = false;
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

async function triggerAction(action) {
    if (!state.connected) return;
    
    try {
        await fetch('/api/action', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ action })
        });
    } catch (e) {
        console.error('Failed to trigger action:', e);
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
            const slider = document.getElementById(key);
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
    ['autoBlink', 'idleMode', 'cyclops', 'curiosity', 'sweat'].forEach(key => {
        const toggle = document.getElementById(key);
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
