const API = '/api';

const els = {
    portSelect: document.getElementById('portSelect'),
    btnConnect: document.getElementById('btnConnect'),
    statusInd: document.getElementById('statusInd'),
    main: document.getElementById('mainContent'),
    btnSave: document.getElementById('btnSave'),
    btnReboot: document.getElementById('btnReboot'),
    btnReset: document.getElementById('btnReset'),
    log: document.getElementById('logOutput')
};

let connected = false;

// Init
async function init() {
    await loadPorts();
    setInterval(updateStatus, 1000); // Polling status/logs
    setupSyncInputs();
    updateUIState();
}

// Listing Ports
async function loadPorts() {
    const res = await fetch(`${API}/ports`);
    const ports = await res.json();
    
    els.portSelect.innerHTML = ports.map(p => `<option value="${p}">${p}</option>`).join('');
    if (ports.length === 0) els.portSelect.innerHTML = '<option value="">No ports found</option>';
}

// Connecting
els.btnConnect.addEventListener('click', async () => {
    const port = els.portSelect.value;
    if (!port) return;
    
    els.btnConnect.disabled = true;
    els.btnConnect.textContent = "CONNECTING...";
    
    const res = await fetch(`${API}/connect`, {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({ port })
    });
    
    const data = await res.json();
    if (data.status === 'ok') {
        connected = true;
        updateUIState();
        fetchConfig();
    } else {
        alert("Connection Failed");
        els.btnConnect.disabled = false;
        els.btnConnect.textContent = "CONNECT";
    }
});

function updateUIState() {
    if (connected) {
        els.statusInd.textContent = "ONLINE";
        els.statusInd.classList.add('connected');
        els.btnConnect.textContent = "CONNECTED";
        els.btnConnect.classList.add('active');
        els.main.style.opacity = '1';
        els.main.style.pointerEvents = 'auto';
        els.btnSave.disabled = false;
    } else {
        els.statusInd.textContent = "OFFLINE";
        els.statusInd.classList.remove('connected');
        els.btnConnect.textContent = "CONNECT";
        els.btnConnect.classList.remove('active');
        els.btnConnect.disabled = false;
        els.main.style.opacity = '0.5';
        els.main.style.pointerEvents = 'none';
        els.btnSave.disabled = true;
    }
}

// Polling
async function updateStatus() {
    if (!connected) return;
    try {
        const res = await fetch(`${API}/status`);
        const data = await res.json();
        
        if (data.logs && data.logs.length > 0) {
           els.log.innerHTML = data.logs.map(l => `<div>> ${l}</div>`).join('');
           els.log.scrollTop = els.log.scrollHeight;
        }

        if (!data.connected) {
            connected = false;
            updateUIState();
        }
    } catch (e) {
        console.error(e);
    }
}

// Config Helpers
function setVal(name, val) {
    if (val === undefined || val === null) return;
    const inputs = document.getElementsByName(name);
    inputs.forEach(el => {
        if(el.type === 'checkbox') el.checked = !!val;
        else el.value = val;
    });
}

function setCheck(name, val) {
    if (val === undefined || val === null) return;
    const inputs = document.getElementsByName(name);
    inputs.forEach(el => el.checked = !!val);
}

function getVal(name) {
    const el = document.getElementsByName(name)[0];
    if (!el) return null;
    return (el.type === 'number' || el.type === 'range') ? parseFloat(el.value) : el.value;
}

function getCheck(name) {
    const el = document.getElementsByName(name)[0];
    return el ? el.checked : false;
}

// Config Handling
async function fetchConfig() {
    const res = await fetch(`${API}/config`);
    const json = await res.json();
    
    // Protocol v4.0: expects { type: "config", data: { ... } }
    const cfg = json.data || json; 

    setVal('bot_name', cfg.bot_name);
    
    // FACE (NEW dynamic block)
    setVal('face.width', cfg.face?.width);
    setVal('face.height', cfg.face?.height);
    setVal('face.radius', cfg.face?.radius);
    setVal('face.spacing', cfg.face?.spacing);
    setCheck('face.auto_blink', cfg.face?.auto_blink);
    setCheck('face.idle_mode', cfg.face?.idle_mode);
    setVal('face.blink_interval', cfg.face?.blink_interval);
    setVal('face.idle_interval', cfg.face?.idle_interval);
    setVal('face.contrast', cfg.face?.contrast);
    setCheck('face.curious', cfg.face?.curious);
    setCheck('face.sweat', cfg.face?.sweat);
    
    // MOTION (Persistent)
    setVal('motion.tilt_deg', cfg.motion?.tilt_deg);
    setVal('motion.shake_angry_dps', cfg.motion?.shake_angry_dps);
    setVal('motion.shake_furious_dps', cfg.motion?.shake_furious_dps);
    setVal('motion.tap_spike_dps', cfg.motion?.tap_spike_dps);
    
    // PINS (Persistent, optional)
    if (cfg.pins) {
        setVal('pins.i2c_sda', cfg.pins.i2c_sda);
        setVal('pins.i2c_scl', cfg.pins.i2c_scl);
        setVal('pins.func_btn', cfg.pins.func_btn);
        setVal('pins.touch_chin', cfg.pins.touch_chin);
        setVal('pins.i2s_bclk', cfg.pins.i2s_bclk);
        setVal('pins.i2s_lrc', cfg.pins.i2s_lrc);
        setVal('pins.i2s_do', cfg.pins.i2s_do);
        setVal('pins.i2s_di', cfg.pins.i2s_di);
        setVal('pins.led', cfg.pins.led);
        setVal('pins.vbat', cfg.pins.vbat);
        setVal('pins.vibro_left', cfg.pins.vibro_left);
        setVal('pins.vibro_right', cfg.pins.vibro_right);
    }
    
    // AUDIO (Dynamic)
    setVal('audio.volume', cfg.audio?.volume);
    setCheck('audio.mic_log', cfg.audio?.mic_log_enabled);
    
    // HAPTIC (Dynamic)
    setVal('haptic.intensity', cfg.haptic?.intensity);
    
    // LED (Dynamic)
    setVal('led.brightness', cfg.led?.brightness);
    setVal('led.r', cfg.led?.r);
    setVal('led.g', cfg.led?.g);
    setVal('led.b', cfg.led?.b);
    
    // POWER (Persistent)
    setVal('power.idle_timeout_ms', cfg.power?.idle_timeout_ms);
    setVal('power.sleep_timeout_ms', cfg.power?.sleep_timeout_ms);
    
    // Trigger range displays to update their value indicators
    document.querySelectorAll('input[type=range]').forEach(el => el.dispatchEvent(new Event('input')));
    
    // Trigger range displays
    document.querySelectorAll('input[type=range]').forEach(el => el.dispatchEvent(new Event('input')));
}

// Debounce helper
function debounce(func, wait) {
    let timeout;
    return function(...args) {
        clearTimeout(timeout);
        timeout = setTimeout(() => func.apply(this, args), wait);
    };
}

// Preview Logic
const sendPreview = debounce(async () => {
    if (!connected) return;
    
    const config = {
        save: false, // Don't write to NVS
        cmd: "set_config",
        face: {
            width: getVal('face.width'),
            height: getVal('face.height'),
            radius: getVal('face.radius'),
            spacing: getVal('face.spacing'),
            auto_blink: getCheck('face.auto_blink'),
            idle_mode: getCheck('face.idle_mode'),
            blink_interval: getVal('face.blink_interval'),
            idle_interval: getVal('face.idle_interval'),
            contrast: getVal('face.contrast')
        },
        audio: {
            volume: getVal('audio.volume'),
            mic_log: getCheck('audio.mic_log')
        },
        haptic: {
            intensity: getVal('haptic.intensity')
        },
        led: {
            brightness: getVal('led.brightness'),
            r: getVal('led.r'),
            g: getVal('led.g'),
            b: getVal('led.b')
        }
    };
    
    await fetch(`${API}/config`, {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(config)
    });
}, 200);

// Bind Preview to Inputs
function setupSyncInputs() {
    const inputs = document.querySelectorAll('input, select');
    inputs.forEach(el => {
        el.addEventListener('input', sendPreview);
    });
}

// Test Face Logic
async function testFace(val) {
    if (!connected) return;
    await fetch(`${API}/config`, { // Re-using config endpoint but sending command object
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({ cmd: "test_face", val: val })
    });
}

// Saving
els.btnSave.addEventListener('click', async () => {
    const config = {
        cmd: "set_config",
        save: true,
        bot_name: getVal('bot_name'),
        face: {
            width: getVal('face.width'),
            height: getVal('face.height'),
            radius: getVal('face.radius'),
            spacing: getVal('face.spacing'),
            auto_blink: getCheck('face.auto_blink'),
            idle_mode: getCheck('face.idle_mode'),
            blink_interval: getVal('face.blink_interval'),
            idle_interval: getVal('face.idle_interval'),
            contrast: getVal('face.contrast'),
            curious: getCheck('face.curious'),
            sweat: getCheck('face.sweat')
        },
        motion: {
            tilt_deg: getVal('motion.tilt_deg'),
            shake_angry_dps: getVal('motion.shake_angry_dps'),
            shake_furious_dps: getVal('motion.shake_furious_dps'),
            tap_spike_dps: getVal('motion.tap_spike_dps')
        },
        pins: {
            i2c_sda: getVal('pins.i2c_sda'),
            i2c_scl: getVal('pins.i2c_scl'),
            func_btn: getVal('pins.func_btn'),
            touch_chin: getVal('pins.touch_chin'),
            i2s_bclk: getVal('pins.i2s_bclk'),
            i2s_lrc: getVal('pins.i2s_lrc'),
            i2s_do: getVal('pins.i2s_do'),
            i2s_di: getVal('pins.i2s_di'),
            led: getVal('pins.led'),
            vbat: getVal('pins.vbat'),
            vibro_left: getVal('pins.vibro_left'),
            vibro_right: getVal('pins.vibro_right')
        },
        power: {
            idle_timeout_ms: getVal('power.idle_timeout_ms'),
            sleep_timeout_ms: getVal('power.sleep_timeout_ms')
        },
        audio: {
            volume: getVal('audio.volume'),
            mic_log: getCheck('audio.mic_log')
        },
        haptic: {
            intensity: getVal('haptic.intensity')
        },
        led: {
            brightness: getVal('led.brightness'),
            r: getVal('led.r'),
            g: getVal('led.g'),
            b: getVal('led.b')
        }
    };
    
    // UI Feedback
    const originalText = els.btnSave.innerHTML;
    els.btnSave.textContent = "SAVING...";
    els.btnSave.disabled = true;
    
    const res = await fetch(`${API}/config`, {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(config)
    });
    
    // Parse response to see if reboot needed
    const data = await res.json();
    
    if (data.msg && data.msg.includes("Reboot required")) {
        els.btnSave.textContent = "REBOOTING...";
        setTimeout(() => {
            // Assume firmware handles reboot if command was sent. 
            // If firmware waits for explicit reboot, we might need another call.
            // But Settings::fromJson (firmware) saves but doesn't auto-reboot unless code changed.
            // DogePet.ino logic: if Settings::pendingReboot -> Serial.print "Reboot required".
            // It does NOT auto-reboot. User must click Reboot.
            
            // Wait, previous logic had auto-reboot. 
            // In DogePet.ino, if pendingReboot was true, it just printed.
            // We should ideally prompt user or just let them click 'Reboot'.
            
            els.btnSave.textContent = "REBOOT NEEDED";
            setTimeout(() => {
                els.btnSave.innerHTML = originalText;
                els.btnSave.disabled = false;
            }, 3000);
        }, 1000);
    } else {
        els.btnSave.textContent = "SAVED";
        setTimeout(() => {
            els.btnSave.innerHTML = originalText;
            els.btnSave.disabled = false;
        }, 1000);
    }
});

// Reboot
els.btnReboot.addEventListener('click', async () => {
    if(confirm("Reboot bot?")) {
        await fetch(`${API}/reboot`, { method: 'POST' });
    }
});

// Reset
els.btnReset.addEventListener('click', async () => {
    if(confirm("Factory Reset? This will wipe all settings.")) {
        await fetch(`${API}/reset`, { method: 'POST' });
    }
});

init();
