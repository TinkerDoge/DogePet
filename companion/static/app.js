const socket = io();
let connected = false;

// DOM Elements
const portSelect = document.getElementById('portSelect');
const connectBtn = document.getElementById('connectBtn');
const refreshBtn = document.getElementById('refreshBtn');
const mainContent = document.getElementById('mainContent');
const statusInd = document.getElementById('statusIndicator');
const logsDiv = document.getElementById('logPanel');

// Tab Switching
document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
        btn.classList.add('active');
        document.getElementById(btn.dataset.tab).classList.add('active');
    });
});

// Port Management
function refreshPorts() {
    fetch('/api/ports')
        .then(res => res.json())
        .then(ports => {
            portSelect.innerHTML = '<option value="" disabled selected>Select Port</option>';
            ports.forEach(port => {
                const opt = document.createElement('option');
                opt.value = port;
                opt.innerText = port;
                portSelect.appendChild(opt);
            });
        });
}

refreshBtn.onclick = refreshPorts;
refreshPorts();

connectBtn.onclick = () => {
    if (!connected) {
        const port = portSelect.value;
        if (!port) return alert("Select a port first!");
        
        fetch('/api/connect', {
            method: 'POST',
            headers: {'Content-Type': 'application/json'},
            body: JSON.stringify({port: port})
        })
        .then(res => res.json())
        .then(data => {
            if (data.status === 'ok') {
                connected = true;
                updateUIState();
                // Fetch config after connection
                setTimeout(() => sendSerial({cmd: 'get_config'}), 1500); // Wait for boot/connection
            } else {
                alert("Connection failed: " + data.msg);
            }
        });
    } else {
        fetch('/api/disconnect', {method: 'POST'})
        .then(() => {
            connected = false;
            updateUIState();
        });
    }
};

function updateUIState() {
    if (connected) {
        connectBtn.innerText = "Disconnect";
        mainContent.classList.remove('disabled');
        statusInd.innerText = "Online";
        statusInd.className = "status-online";
    } else {
        connectBtn.innerText = "Connect";
        mainContent.classList.remove('disabled');
        mainContent.classList.add('disabled');
        statusInd.innerText = "Offline";
        statusInd.className = "status-offline";
    }
}

function sendSerial(data) {
    if (!connected) return;
    fetch('/api/send', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(data)
    });
}

// Socket Events
socket.on('disconnect', () => {
    connected = false;
    updateUIState();
});

socket.on('serial_data', (data) => {
    log("RX JSON: " + JSON.stringify(data));
    if (data.type === 'config') {
        populateConfig(data.data);
    }
});

socket.on('serial_log', (data) => {
    log("RX: " + data.msg);
});

function log(msg) {
    const div = document.createElement('div');
    div.innerText = msg;
    const panel = document.getElementById('logPanel');
    panel.prepend(div);
    if (panel.children.length > 50) panel.lastChild.remove();
}

// Config Population
function populateConfig(cfg) {
    if (cfg.firmware) document.getElementById('fwDisplay').innerText = cfg.firmware;
    
    // Face
    if (cfg.face) {
        document.getElementById('eyeWidth').value = cfg.face.width;
        document.getElementById('eyeHeight').value = cfg.face.height;
        document.getElementById('eyeRadius').value = cfg.face.radius;
        document.getElementById('eyeSpacing').value = cfg.face.spacing;
        document.getElementById('autoBlink').checked = cfg.face.auto_blink;
        document.getElementById('blinkInterval').value = cfg.face.blink_interval || 3;
        document.getElementById('idleInterval').value = cfg.face.idle_interval || 4;
        document.getElementById('curiousMode').checked = cfg.face.curious_mode !== undefined ? cfg.face.curious_mode : true;
        document.getElementById('curiousInterval').value = cfg.face.curious_interval || 2;
        document.getElementById('curiousVariation').value = cfg.face.curious_variation || 2;
    }
    // Audio
    if (cfg.audio) {
        document.getElementById('volume').value = cfg.audio.volume;
        document.getElementById('volValue').innerText = cfg.audio.volume;
        document.getElementById('micLog').checked = cfg.audio.mic_log;
    }
    // Haptics
    if (cfg.haptic) {
        document.getElementById('hapticIntensity').value = cfg.haptic.intensity;
        document.getElementById('hapValue').innerText = cfg.haptic.intensity;
    }
    // LED
    if (cfg.led) {
        document.getElementById('ledBright').value = cfg.led.brightness;
        // Convert RGB to Hex
        const r = cfg.led.r.toString(16).padStart(2, '0');
        const g = cfg.led.g.toString(16).padStart(2, '0');
        const b = cfg.led.b.toString(16).padStart(2, '0');
        document.getElementById('ledColor').value = `#${r}${g}${b}`;
    }
    // Motion
    if (cfg.motion) {
        document.getElementById('tiltDeg').value = cfg.motion.tilt_deg;
        document.getElementById('shakeAngry').value = cfg.motion.shake_angry_dps;
        document.getElementById('shakeFurious').value = cfg.motion.shake_furious_dps;
        document.getElementById('tapSpike').value = cfg.motion.tap_spike_dps;
    }
    // Power
    if (cfg.power) {
        document.getElementById('idleTimeout').value = cfg.power.idle_timeout_ms;
        document.getElementById('sleepTimeout').value = cfg.power.sleep_timeout_ms;
    }
    // Pins
    if (cfg.pins) {
        document.getElementById('pinSda').value = cfg.pins.i2c_sda;
        document.getElementById('pinScl').value = cfg.pins.i2c_scl;
        document.getElementById('pinBtn').value = cfg.pins.func_btn;
        document.getElementById('pinChin').value = cfg.pins.touch_chin;
        document.getElementById('pinLed').value = cfg.pins.led;
        document.getElementById('pinVbat').value = cfg.pins.vbat;
        document.getElementById('pinVl').value = cfg.pins.vibro_left;
        document.getElementById('pinVr').value = cfg.pins.vibro_right;
        
        document.getElementById('pinBclk').value = cfg.pins.i2s_bclk;
        document.getElementById('pinLrc').value = cfg.pins.i2s_lrc;
        document.getElementById('pinDo').value = cfg.pins.i2s_do;
        document.getElementById('pinDi').value = cfg.pins.i2s_di;
    }
}

// Sliders Live Update
document.getElementById('volume').oninput = (e) => document.getElementById('volValue').innerText = e.target.value;
document.getElementById('hapticIntensity').oninput = (e) => document.getElementById('hapValue').innerText = e.target.value;

// Save Handlers
document.getElementById('saveAppearance').onclick = () => {
    const data = {
        cmd: 'set_config',
        face: {
            width: parseInt(document.getElementById('eyeWidth').value),
            height: parseInt(document.getElementById('eyeHeight').value),
            radius: parseInt(document.getElementById('eyeRadius').value),
            spacing: parseInt(document.getElementById('eyeSpacing').value),
            auto_blink: document.getElementById('autoBlink').checked,
            blink_interval: parseInt(document.getElementById('blinkInterval').value),
            idle_interval: parseInt(document.getElementById('idleInterval').value),
            curious_mode: document.getElementById('curiousMode').checked,
            curious_interval: parseInt(document.getElementById('curiousInterval').value),
            curious_variation: parseInt(document.getElementById('curiousVariation').value)
        }
    };
    sendSerial(data);
};

document.getElementById('saveAudio').onclick = () => {
    const data = {
        cmd: 'set_config',
        audio: {
            volume: parseInt(document.getElementById('volume').value),
            mic_log: document.getElementById('micLog').checked
        },
        haptic: {
            intensity: parseInt(document.getElementById('hapticIntensity').value)
        }
    };
    sendSerial(data);
};

document.getElementById('saveLed').onclick = () => {
    const hex = document.getElementById('ledColor').value;
    const r = parseInt(hex.substr(1,2), 16);
    const g = parseInt(hex.substr(3,2), 16);
    const b = parseInt(hex.substr(5,2), 16);
    
    const data = {
        cmd: 'set_config',
        led: {
            brightness: parseInt(document.getElementById('ledBright').value),
            r: r, g: g, b: b
        }
    };
    sendSerial(data);
};

document.getElementById('saveMotion').onclick = () => {
    if(!confirm("Reboot required. Continue?")) return;
    const data = {
        cmd: 'set_config',
        motion: {
            tilt_deg: parseFloat(document.getElementById('tiltDeg').value),
            shake_angry_dps: parseFloat(document.getElementById('shakeAngry').value),
            shake_furious_dps: parseFloat(document.getElementById('shakeFurious').value),
            tap_spike_dps: parseFloat(document.getElementById('tapSpike').value)
        },
        power: {
            idle_timeout_ms: parseInt(document.getElementById('idleTimeout').value),
            sleep_timeout_ms: parseInt(document.getElementById('sleepTimeout').value)
        }
    };
    sendSerial(data);
    setTimeout(() => sendSerial({cmd: 'reboot'}), 500);
};

document.getElementById('savePins').onclick = () => {
    if(!confirm("Warning: Incorrect pins can brick the device. Reboot required. Continue?")) return;
    const data = {
        cmd: 'set_config',
        pins: {
            i2c_sda: parseInt(document.getElementById('pinSda').value),
            i2c_scl: parseInt(document.getElementById('pinScl').value),
            func_btn: parseInt(document.getElementById('pinBtn').value),
            touch_chin: parseInt(document.getElementById('pinChin').value),
            led: parseInt(document.getElementById('pinLed').value),
            vbat: parseInt(document.getElementById('pinVbat').value),
            vibro_left: parseInt(document.getElementById('pinVl').value),
            vibro_right: parseInt(document.getElementById('pinVr').value),
            i2s_bclk: parseInt(document.getElementById('pinBclk').value),
            i2s_lrc: parseInt(document.getElementById('pinLrc').value),
            i2s_do: parseInt(document.getElementById('pinDo').value),
            i2s_di: parseInt(document.getElementById('pinDi').value)
        }
    };
    sendSerial(data);
    setTimeout(() => sendSerial({cmd: 'reboot'}), 500);
};

// Test Functions
window.testAudio = (action) => {
    sendSerial({cmd: 'test_audio', action: action});
};

window.testHaptic = (action) => {
    // Protocol doesn't explicitly have test_haptic command yet, 
    // assuming we might add it or user just wanted audio. 
    // Wait, protocol says "Haptic pattern tester: Click...".
    // Let's implement a generic 'test_haptic' if firmware supported it, 
    // or abuse set_config to trigger it? No.
    // Let's check protocol v4.1 again. It mentions "Haptic pattern tester" in UI list but not explicit command.
    // I'll leave it as a placeholder log for now or implement generic test command later.
    log("Test Haptic: " + action + " (Not implemented in FW yet)");
};
