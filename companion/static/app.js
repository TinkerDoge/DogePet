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
    const data = await res.json();
    
    // Serial manager often wraps in data.data or returns object directly
    const cfg = data.data || data; 

    setVal('bot_name', cfg.bot_name);
    setVal('eye.width', cfg.eye?.width);
    setVal('eye.height', cfg.eye?.height);
    setVal('eye.radius', cfg.eye?.radius);
    setVal('eye.spacing', cfg.eye?.spacing);
    setCheck('eye.auto_blink', cfg.eye?.auto_blink);
    setCheck('eye.idle_mode', cfg.eye?.idle_mode);
    setCheck('eye.sweat', cfg.eye?.sweat);
    setCheck('eye.cyclops', cfg.eye?.cyclops);
    setVal('eye.blink_int', cfg.eye?.blink_int);
    setVal('eye.idle_int', cfg.eye?.idle_int);
    
    setVal('motion.tilt', cfg.motion?.tilt);
    setVal('motion.shake', cfg.motion?.shake);
    setVal('motion.furious', cfg.motion?.furious);
    setVal('motion.tap', cfg.motion?.tap);
    
    setVal('audio.vol', cfg.audio?.vol);
    setVal('audio.mic_t', cfg.audio?.mic_t);
    setVal('haptic_int', cfg.haptic_int);
    
    setVal('led.bri', cfg.led?.bri);
    setVal('led.r', cfg.led?.r);
    setVal('led.g', cfg.led?.g);
    setVal('led.b', cfg.led?.b);
    
    setVal('screen.con', cfg.screen?.con);
    setCheck('screen.flip', cfg.screen?.flip);
    
    setVal('power.idle_ms', cfg.power?.idle_ms);
    setVal('power.sleep_ms', cfg.power?.sleep_ms);
    
    // Trigger range displays
    document.querySelectorAll('input[type=range]').forEach(el => el.dispatchEvent(new Event('input')));
}

// Saving
els.btnSave.addEventListener('click', async () => {
    const config = {
        cmd: "set_config",
        bot_name: getVal('bot_name'),
        eye: {
            width: getVal('eye.width'),
            height: getVal('eye.height'),
            radius: getVal('eye.radius'),
            spacing: getVal('eye.spacing'),
            auto_blink: getCheck('eye.auto_blink'),
            idle_mode: getCheck('eye.idle_mode'),
            sweat: getCheck('eye.sweat'),
            cyclops: getCheck('eye.cyclops'),
            blink_int: getVal('eye.blink_int'),
            idle_int: getVal('eye.idle_int'),
        },
        motion: {
            tilt: getVal('motion.tilt'),
            shake: getVal('motion.shake'),
            furious: getVal('motion.furious'),
            tap: getVal('motion.tap'),
        },
        power: {
            idle_ms: getVal('power.idle_ms'),
            sleep_ms: getVal('power.sleep_ms'),
        },
        audio: {
            vol: getVal('audio.vol'),
            mic_t: getVal('audio.mic_t')
        },
        led: {
            bri: getVal('led.bri'),
            r: getVal('led.r'),
            g: getVal('led.g'),
            b: getVal('led.b'),
        },
        screen: {
            con: getVal('screen.con'),
            flip: getCheck('screen.flip')
        },
        haptic_int: getVal('haptic_int')
    };
    
    const originalText = els.btnSave.innerHTML;
    els.btnSave.textContent = "SYNCING...";
    els.btnSave.disabled = true;
    
    await fetch(`${API}/config`, {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(config)
    });
    
    setTimeout(() => {
        els.btnSave.textContent = "REBOOTING...";
        setTimeout(() => {
            els.btnSave.innerHTML = originalText;
            els.btnSave.disabled = false;
        }, 3000);
    }, 1000);
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
