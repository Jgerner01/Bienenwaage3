// Bienenwaage3 – AJAX-Steuerung und Tab-Logik

const UPDATE_INTERVAL_MS = 2000;

// ── Tabs ───────────────────────────────────────────────────────────────────────

document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.addEventListener('click', () => {
        document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
        document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
        btn.classList.add('active');
        document.getElementById('tab-' + btn.dataset.tab).classList.add('active');
    });
});

// ── Hilfsfunktionen ────────────────────────────────────────────────────────────

function setText(id, val) {
    const el = document.getElementById(id);
    if (el) el.textContent = val;
}

function setIfNotFocused(id, value) {
    const el = document.getElementById(id);
    if (el && document.activeElement !== el) el.textContent = value;
}

// ── AJAX /data ─────────────────────────────────────────────────────────────────

async function updateData() {
    try {
        const resp = await fetch('/data');
        if (!resp.ok) return;
        const d = await resp.json();
        renderModules(d.modules);
        renderTemperature(d.temperature);
        renderLcd(d.lcd);
        renderNetworkStatus(d.network);
    } catch (_) {}
}

function renderModules(modules) {
    const container = document.getElementById('module-list');
    if (!container) return;

    modules.forEach(m => {
        let row = document.getElementById('mod-row-' + m.id);
        if (!row) {
            row = document.createElement('div');
            row.id = 'mod-row-' + m.id;
            row.className = 'module-row';
            row.innerHTML =
                `<span class="dot" id="dot-${m.id}"></span>` +
                `<span class="module-name">Modul ${m.id + 1}</span>` +
                `<span class="module-weight" id="mw-${m.id}">--</span>` +
                `<span class="module-sigma" id="ms-${m.id}"></span>` +
                `<span class="module-quick" id="mq-${m.id}"></span>`;
            container.appendChild(row);
        }

        if (!m.active) { row.style.display = 'none'; return; }
        row.style.display = 'flex';

        document.getElementById('dot-' + m.id).className = 'dot' + (m.online ? ' online' : '');
        setIfNotFocused('mw-' + m.id, m.online ? (m.weight_g / 1000).toFixed(3) + ' kg' : 'OFFLINE');
        setIfNotFocused('ms-' + m.id, m.online ? ('σ=' + m.sigma_g.toFixed(1) + 'g') : '');
        setIfNotFocused('mq-' + m.id, m.online ? ('~' + (m.weight_quick_g / 1000).toFixed(3) + ' kg') : '');
    });
}

function renderTemperature(t) {
    const el = document.getElementById('temp-value');
    if (el) el.textContent = t.online ? t.value_c.toFixed(2) + ' °C' : 'OFFLINE';
    const dot = document.getElementById('temp-dot');
    if (dot) dot.className = 'dot' + (t.online ? ' online' : '');
}

function renderLcd(lcd) {
    const l1 = document.getElementById('lcd-line1');
    const l2 = document.getElementById('lcd-line2');
    if (l1) l1.textContent = lcd.line1.padEnd(16).substring(0, 16);
    if (l2) l2.textContent = lcd.line2.padEnd(16).substring(0, 16);
}

function renderNetworkStatus(net) {
    // Status-Tab
    setText('eth-ip', net.eth_ip || '--');
    const mqttDot = document.getElementById('mqtt-dot');
    if (mqttDot) mqttDot.className = 'dot' + (net.mqtt_connected ? ' online' : '');

    // WLAN/ETH-Tab – aktueller Status
    setText('cur-eth-ip',    net.eth_ip      || '--');
    setText('cur-eth-gw',    net.eth_gateway || '--');
    setText('cur-eth-sn',    net.eth_subnet  || '--');
    setText('cur-ap-ip',     net.ap_ip       || '--');
    setText('cur-eth-mac',   net.eth_mac     || '--');

    const states = ['Verbinde…', 'Verbunden', 'Getrennt'];
    setText('cur-eth-state', states[net.eth_state] || '--');
}

// ── Tara / Kalibrierung ────────────────────────────────────────────────────────

function sendTare(moduleIdx, type) {
    fetch('/tare', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ module: moduleIdx, type })
    });
}

function sendCalibrate(moduleIdx, knownWeight) {
    fetch('/calibrate', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ module: moduleIdx, known_weight: parseFloat(knownWeight) })
    });
}

function sendSet(payload) {
    return fetch('/set', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
    });
}

// ── Taster-Simulation ─────────────────────────────────────────────────────────

function pressBtn(n) {
    fetch('/btn' + n, { method: 'POST' });
}

// ── Werkseinstellungen ─────────────────────────────────────────────────────────

function factoryReset() {
    if (!confirm('Alle Einstellungen löschen und neu starten?')) return;
    fetch('/reset', { method: 'POST', body: '{}', headers: { 'Content-Type': 'application/json' } });
}

// ── Export / Import ────────────────────────────────────────────────────────────

function exportParams() {
    window.location.href = '/export';
}

function importParams() {
    document.getElementById('import-file').click();
}

// ── DOMContentLoaded ───────────────────────────────────────────────────────────

document.addEventListener('DOMContentLoaded', () => {

    // Import-Datei
    const importFile = document.getElementById('import-file');
    if (importFile) {
        importFile.addEventListener('change', async (e) => {
            const file = e.target.files[0];
            if (!file) return;
            const data = await file.arrayBuffer();
            const resp = await fetch('/import', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: data
            });
            const json = await resp.json();
            alert(json.ok ? 'Import OK – Neustart...' : 'Import fehlgeschlagen');
        });
    }

    // OTA-Upload
    const otaForm = document.getElementById('ota-form');
    if (otaForm) {
        otaForm.addEventListener('submit', (e) => {
            e.preventDefault();
            const file = document.getElementById('ota-file').files[0];
            if (!file) return;
            const prog = document.getElementById('ota-progress');
            const bar  = document.getElementById('ota-bar');
            if (prog) prog.style.display = 'block';
            const fd = new FormData();
            fd.append('file', file);
            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/update');
            xhr.upload.onprogress = (ev) => {
                if (bar && ev.lengthComputable)
                    bar.value = Math.round(100 * ev.loaded / ev.total);
            };
            xhr.onload = () => {
                try {
                    const json = JSON.parse(xhr.responseText);
                    alert(json.ok ? 'OTA OK – Neustart...' : 'OTA fehlgeschlagen');
                } catch (_) {
                    alert(xhr.status === 200 ? 'OTA OK – Neustart...' : 'OTA fehlgeschlagen');
                }
            };
            xhr.send(fd);
        });
    }

    // Netzwerk-Formular (ETH/IP)
    const netForm = document.getElementById('net-form');
    if (netForm) {
        netForm.addEventListener('submit', (e) => {
            e.preventDefault();
            const cfg = {
                useDhcp:  document.getElementById('use-dhcp').checked,
                staticIp: document.getElementById('static-ip').value,
                gateway:  document.getElementById('gateway').value,
                subnet:   document.getElementById('subnet').value
            };
            sendSet({ network: cfg }).then(() => alert('Gespeichert – Neustart...'));
        });
    }

    // AP-Formular
    const apForm = document.getElementById('ap-form');
    if (apForm) {
        apForm.addEventListener('submit', (e) => {
            e.preventDefault();
            sendSet({ network: {
                apSsid:     document.getElementById('ap-ssid').value,
                apPassword: document.getElementById('ap-password').value
            }}).then(() => alert('Gespeichert – Neustart...'));
        });
    }

    // Konfigurationsfelder einmalig befüllen
    fetch('/config').then(r => r.json()).then(cfg => {
        const set = (id, val) => {
            const el = document.getElementById(id);
            if (!el) return;
            if (el.type === 'checkbox') el.checked = val;
            else el.value = val || '';
        };
        set('use-dhcp',      cfg.useDhcp);
        set('static-ip',     cfg.staticIp);
        set('gateway',       cfg.gateway);
        set('subnet',        cfg.subnet);
        set('ap-ssid',       cfg.apSsid);
        set('ap-password',   cfg.apPassword);
        set('mqtt-server',   cfg.mqttServer);
        set('mqtt-port',     cfg.mqttPort);
        set('mqtt-user',     cfg.mqttUser);
        set('mqtt-password', cfg.mqttPassword);
        set('mqtt-prefix',   cfg.mqttPrefix);
        set('ha-discovery',  cfg.haDiscovery);
        set('mqtt-retain',   cfg.mqttRetain);
    }).catch(() => {});

    // Firmware-Version + Build-Nummer aus /status
    fetch('/status').then(r => r.json()).then(s => {
        const el = document.getElementById('fw-info');
        if (el) el.textContent = `Firmware v${s.fw_version} / Build ${s.build}`;
    }).catch(() => {});

    // Erster Datenabruf + periodisches Update
    updateData();
    setInterval(updateData, UPDATE_INTERVAL_MS);
});
