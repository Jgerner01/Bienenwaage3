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
    // LCD-Zeilen nur setzen, kein Fokus-Problem (keine Eingabefelder)
    if (l1) l1.textContent = lcd.line1.padEnd(16).substring(0, 16);
    if (l2) l2.textContent = lcd.line2.padEnd(16).substring(0, 16);
}

function renderNetworkStatus(net) {
    const el = document.getElementById('eth-ip');
    if (el) el.textContent = net.eth_ip;
    const mqttDot = document.getElementById('mqtt-dot');
    if (mqttDot) mqttDot.className = 'dot' + (net.mqtt_connected ? ' online' : '');
}

// Eingabefeld nur überschreiben wenn nicht fokussiert
function setIfNotFocused(id, value) {
    const el = document.getElementById(id);
    if (el && document.activeElement !== el) el.textContent = value;
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

document.addEventListener('DOMContentLoaded', () => {
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
        otaForm.addEventListener('submit', async (e) => {
            e.preventDefault();
            const file = document.getElementById('ota-file').files[0];
            if (!file) return;
            const prog = document.getElementById('ota-progress');
            if (prog) prog.style.display = 'block';
            const data = await file.arrayBuffer();
            const resp = await fetch('/update', { method: 'POST', body: data });
            const json = await resp.json();
            alert(json.ok ? 'OTA OK – Neustart...' : 'OTA fehlgeschlagen');
        });
    }

    // Netzwerk-Formular
    const netForm = document.getElementById('net-form');
    if (netForm) {
        netForm.addEventListener('submit', (e) => {
            e.preventDefault();
            const cfg = {
                useDhcp:      document.getElementById('use-dhcp').checked,
                staticIp:     document.getElementById('static-ip').value,
                gateway:      document.getElementById('gateway').value,
                subnet:       document.getElementById('subnet').value,
                mqttServer:   document.getElementById('mqtt-server').value,
                mqttPort:     parseInt(document.getElementById('mqtt-port').value),
                mqttUser:     document.getElementById('mqtt-user').value,
                mqttPassword: document.getElementById('mqtt-password').value,
                mqttPrefix:   document.getElementById('mqtt-prefix').value,
                haDiscovery:  document.getElementById('ha-discovery').checked,
                mqttRetain:   document.getElementById('mqtt-retain').checked
            };
            sendSet({ network: cfg }).then(() => alert('Gespeichert – Neustart...'));
        });
    }

    updateData();
    setInterval(updateData, UPDATE_INTERVAL_MS);
});
