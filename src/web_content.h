// Bienenwaage3 – Web-Inhalte als PROGMEM (aus data/ generiert)
// Automatisch eingebettet – kein separates LittleFS-Image nötig
#pragma once
#include <pgmspace.h>

static const char WEB_INDEX_HTML[] PROGMEM = R"HTMLEOF(
<!DOCTYPE html>
<html lang="de">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Bienenwaage3</title>
<link rel="stylesheet" href="style.css">
</head>
<body>

<header class="site-header">
  <span class="header-title">Bienenwaage3</span>
  <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 80 42" width="80" height="42" aria-label="Biene auf Waage">
    <!-- Fuß & Stiel -->
    <rect x="36" y="33" width="8" height="2.5" rx="1" fill="#b8730a"/>
    <rect x="39" y="17" width="2" height="16" fill="#b8730a"/>
    <!-- Balken -->
    <rect x="8" y="14" width="64" height="3" rx="1.5" fill="#e89010"/>
    <circle cx="40" cy="15.5" r="2.2" fill="#b8730a"/>
    <!-- Linke Ketten & Schale -->
    <line x1="13" y1="17" x2="9"  y2="27" stroke="#b8730a" stroke-width="0.8"/>
    <line x1="13" y1="17" x2="17" y2="27" stroke="#b8730a" stroke-width="0.8"/>
    <path d="M6,27 Q13,34 20,27" stroke="#e89010" stroke-width="1.5" fill="rgba(232,144,16,0.1)" stroke-linecap="round"/>
    <!-- Rechte Ketten & Schale -->
    <line x1="67" y1="17" x2="63" y2="27" stroke="#b8730a" stroke-width="0.8"/>
    <line x1="67" y1="17" x2="71" y2="27" stroke="#b8730a" stroke-width="0.8"/>
    <path d="M60,27 Q67,34 74,27" stroke="#e89010" stroke-width="1.5" fill="rgba(232,144,16,0.1)" stroke-linecap="round"/>
    <!-- Flügel (hinter Körper) -->
    <ellipse cx="10" cy="18" rx="5" ry="2.5" fill="rgba(210,238,255,0.82)" stroke="#aacce0" stroke-width="0.45" transform="rotate(-28 10 18)"/>
    <ellipse cx="16" cy="18.5" rx="4.5" ry="2" fill="rgba(210,238,255,0.82)" stroke="#aacce0" stroke-width="0.45" transform="rotate(-8 16 18.5)"/>
    <!-- Bienenleib -->
    <ellipse cx="13" cy="23" rx="5.2" ry="2.8" fill="#FFD700"/>
    <rect x="10.8" y="20.4" width="1.2" height="5.2" rx="0.6" fill="#111" opacity="0.42"/>
    <rect x="12.9" y="20.1" width="1.3" height="5.8" rx="0.6" fill="#111" opacity="0.42"/>
    <rect x="15"   y="20.4" width="1.2" height="5.2" rx="0.6" fill="#111" opacity="0.42"/>
    <!-- Kopf -->
    <circle cx="18.2" cy="21.4" r="2.3" fill="#FFD700"/>
    <circle cx="19"   cy="20.5" r="0.65" fill="#111"/>
    <!-- Fühler -->
    <line x1="17.5" y1="19.2" x2="16.5" y2="17" stroke="#111" stroke-width="0.7" stroke-linecap="round"/>
    <circle cx="16.5" cy="17" r="0.7" fill="#111"/>
    <line x1="18.5" y1="19.1" x2="19.5" y2="17" stroke="#111" stroke-width="0.7" stroke-linecap="round"/>
    <circle cx="19.5" cy="17" r="0.7" fill="#111"/>
    <!-- Honigwabe: 3 Sechsecke (pointy-top, r=3) -->
    <polygon points="64.4,19.5 67,21 67,24 64.4,25.5 61.8,24 61.8,21" fill="#FFD700" stroke="#b8730a" stroke-width="0.7"/>
    <polygon points="69.6,19.5 72.2,21 72.2,24 69.6,25.5 67,24 67,21"  fill="#FFC433" stroke="#b8730a" stroke-width="0.7"/>
    <polygon points="67,24 69.6,25.5 69.6,28.5 67,30 64.4,28.5 64.4,25.5" fill="#FFB800" stroke="#b8730a" stroke-width="0.7"/>
    <!-- Honigtropfen -->
    <path d="M65.5,30 Q67,36 68.5,30 Z" fill="#FFB800" opacity="0.72"/>
    <!-- Glanzpunkte -->
    <circle cx="64.4" cy="22.5" r="0.9" fill="#fff" opacity="0.28"/>
    <circle cx="69.6" cy="22.5" r="0.9" fill="#fff" opacity="0.28"/>
    <circle cx="67"   cy="27"   r="0.9" fill="#fff" opacity="0.28"/>
  </svg>
  <span id="fw-info" class="fw-badge">Firmware v-- / Build --</span>
</header>

<div class="tabs">
  <button class="tab-btn active" data-tab="status">Status</button>
  <button class="tab-btn" data-tab="waagen">Waagen</button>
  <button class="tab-btn" data-tab="netz">WLAN/ETH</button>
  <button class="tab-btn" data-tab="mqtt">MQTT</button>
  <button class="tab-btn" data-tab="lcd">LCD</button>
  <button class="tab-btn" data-tab="param">Parameter</button>
  <button class="tab-btn" data-tab="firmware">Firmware</button>
</div>

<!-- ── Status ──────────────────────────────────────────────────────────── -->
<div id="tab-status" class="tab-content active">
  <div class="card">
    <h3>Module</h3>
    <div id="module-list"></div>
  </div>
  <div class="card">
    <h3>Temperatur</h3>
    <div class="status-item">
      <span class="dot" id="temp-dot"></span>
      <span class="temp-value" id="temp-value">--</span>
    </div>
  </div>
  <div class="card">
    <h3>Verbindung</h3>
    <div class="status-bar">
      <div class="status-item">ETH: <span id="eth-ip">--</span></div>
      <div class="status-item"><span class="dot" id="mqtt-dot"></span> MQTT</div>
    </div>
  </div>
</div>

<!-- ── Waagen ──────────────────────────────────────────────────────────── -->
<div id="tab-waagen" class="tab-content">
  <div id="waagen-config">
    <script>
    for (let i = 0; i < 7; i++) {
      document.write(`
      <div class="card">
        <h3>Modul ${i+1} (GPIO ${[14,13,17,15,2,34,35][i]})</h3>
        <div class="form-row">
          <label>Aktiv</label>
          <input type="checkbox" id="m${i}-active" onchange="sendSet({module:${i},active:this.checked})">
        </div>
        <div class="form-row">
          <label>Kalibrierfaktor</label>
          <input type="number" id="m${i}-calib" step="0.0001" placeholder="g/ADC">
          <button class="btn secondary" onclick="sendSet({module:${i},calibFactor:parseFloat(document.getElementById('m${i}-calib').value)})">Übernehmen</button>
        </div>
        <div class="form-row">
          <label>Bekanntes Gewicht [g]</label>
          <input type="number" id="m${i}-known" step="1" placeholder="z.B. 5000">
          <button class="btn" onclick="sendCalibrate(${i}, document.getElementById('m${i}-known').value)">Kalibrieren</button>
        </div>
        <div class="form-row">
          <button class="btn secondary" onclick="sendTare(${i},'main')">Grundtara</button>
          <button class="btn secondary" onclick="sendTare(${i},'yield')">Ertragstara</button>
          <button class="btn secondary" onclick="sendTare(${i},'clear_yield')">Ertragstara löschen</button>
        </div>
        <div class="form-row">
          <label>Poly a2 (g/°C²)</label>
          <input type="number" id="m${i}-a2" step="0.001">
          <label>a1 (g/°C)</label>
          <input type="number" id="m${i}-a1" step="0.01">
          <label>a0 (g)</label>
          <input type="number" id="m${i}-a0" step="0.1">
          <button class="btn secondary" onclick="sendSet({module:${i},polyA2:parseFloat(document.getElementById('m${i}-a2').value),polyA1:parseFloat(document.getElementById('m${i}-a1').value),polyA0:parseFloat(document.getElementById('m${i}-a0').value)})">Speichern</button>
        </div>
      </div>`);
    }
    </script>
  </div>
</div>

<!-- ── WLAN/ETH ─────────────────────────────────────────────────────────── -->
<div id="tab-netz" class="tab-content">

  <div class="card">
    <h3>Aktueller Netzwerk-Status</h3>
    <div class="eth-status-grid">
      <span class="lbl">ETH IP-Adresse</span><span id="cur-eth-ip" class="val">--</span>
      <span class="lbl">Gateway</span>        <span id="cur-eth-gw"  class="val">--</span>
      <span class="lbl">Subnetz</span>        <span id="cur-eth-sn"  class="val">--</span>
      <span class="lbl">Verbindung</span>     <span id="cur-eth-state" class="val">--</span>
      <span class="lbl">Access-Point IP</span><span id="cur-ap-ip"   class="val">--</span>
      <span class="lbl">ETH MAC-Adresse</span><span id="cur-eth-mac" class="val">--</span>
    </div>
  </div>

  <div class="card">
    <h3>Ethernet / Statische IP</h3>
    <form id="net-form">
      <div class="form-row"><label>DHCP</label><input type="checkbox" id="use-dhcp" checked></div>
      <div class="form-row"><label>Statische IP</label><input type="text" id="static-ip" placeholder="192.168.1.50"></div>
      <div class="form-row"><label>Gateway</label><input type="text" id="gateway" placeholder="192.168.1.1"></div>
      <div class="form-row"><label>Subnetz</label><input type="text" id="subnet" placeholder="255.255.255.0"></div>
      <button type="submit" class="btn">Speichern &amp; Neustart</button>
    </form>
  </div>

  <div class="card">
    <h3>WLAN Access Point (Konfigurationsmodus)</h3>
    <p class="note">Wird aktiviert wenn kein Ethernet verfügbar ist. IP: 192.168.4.1</p>
    <form id="ap-form">
      <div class="form-row"><label>SSID</label><input type="text" id="ap-ssid" placeholder="Bienenwaage3-Setup"></div>
      <div class="form-row"><label>Passwort</label><input type="password" id="ap-password" placeholder="min. 8 Zeichen"></div>
      <button type="submit" class="btn">Speichern &amp; Neustart</button>
    </form>
  </div>

</div>

<!-- ── MQTT ────────────────────────────────────────────────────────────── -->
<div id="tab-mqtt" class="tab-content">
  <div class="card">
    <h3>MQTT-Broker</h3>
    <form id="net-form-mqtt" onsubmit="event.preventDefault(); saveNetworkFromMqttForm()">
      <div class="form-row"><label>Broker-IP</label><input type="text" id="mqtt-server" placeholder="192.168.1.x"></div>
      <div class="form-row"><label>Port</label><input type="number" id="mqtt-port" value="1883"></div>
      <div class="form-row"><label>Benutzer</label><input type="text" id="mqtt-user"></div>
      <div class="form-row"><label>Passwort</label><input type="password" id="mqtt-password"></div>
      <div class="form-row"><label>Topic-Präfix</label><input type="text" id="mqtt-prefix" value="bienenwaage3"></div>
      <div class="form-row"><label>HA Auto-Discovery</label><input type="checkbox" id="ha-discovery" checked></div>
      <div class="form-row"><label>Retain</label><input type="checkbox" id="mqtt-retain" checked></div>
      <button type="submit" class="btn">Speichern &amp; Neustart</button>
    </form>
  </div>
</div>
<script>
function saveNetworkFromMqttForm() {
  sendSet({ network: {
    mqttServer:   document.getElementById('mqtt-server').value,
    mqttPort:     parseInt(document.getElementById('mqtt-port').value),
    mqttUser:     document.getElementById('mqtt-user').value,
    mqttPassword: document.getElementById('mqtt-password').value,
    mqttPrefix:   document.getElementById('mqtt-prefix').value,
    haDiscovery:  document.getElementById('ha-discovery').checked,
    mqttRetain:   document.getElementById('mqtt-retain').checked
  }}).then(() => alert('Gespeichert – Neustart...'));
}
</script>

<!-- ── LCD ─────────────────────────────────────────────────────────────── -->
<div id="tab-lcd" class="tab-content">
  <div class="card">
    <h3>LCD-Anzeige (live)</h3>
    <div class="lcd-frame">
      <span class="lcd-line" id="lcd-line1">                </span>
      <span class="lcd-line" id="lcd-line2">                </span>
    </div>
  </div>
  <div class="card">
    <h3>Taster</h3>
    <div class="form-row">
      <button class="btn" onclick="pressBtn(1)">Taster 1 – Modul wählen</button>
      <button class="btn secondary" onclick="pressBtn(2)">Taster 2 – Funktion</button>
    </div>
  </div>
</div>

<!-- ── Parameter ──────────────────────────────────────────────────────── -->
<div id="tab-param" class="tab-content">
  <div class="card">
    <h3>Import / Export</h3>
    <button class="btn secondary" onclick="exportParams()">Parameter exportieren</button>
    <button class="btn secondary" onclick="importParams()">Parameter importieren</button>
    <input type="file" id="import-file" accept=".json" style="display:none">
  </div>
  <div class="card">
    <h3>Werkseinstellungen</h3>
    <p class="note">Alle Parameter und Kalibrierungen werden gelöscht und auf Standardwerte zurückgesetzt.</p>
    <button class="btn danger" onclick="factoryReset()">Werkseinstellungen wiederherstellen</button>
  </div>
</div>

<!-- ── Firmware ────────────────────────────────────────────────────────── -->
<div id="tab-firmware" class="tab-content">
  <div class="card">
    <h3>OTA Firmware-Update (Browser)</h3>
    <p class="note">Binary aus: <code>.pio/build/wt32-eth01/firmware.bin</code></p>
    <form id="ota-form">
      <input type="file" id="ota-file" accept=".bin" required>
      <button type="submit" class="btn">Upload &amp; Neustart</button>
    </form>
    <div id="ota-progress"><progress id="ota-bar" value="0" max="100"></progress></div>
  </div>
</div>

<script src="app.js"></script>
</body>
</html>
)HTMLEOF";

static const char WEB_STYLE_CSS[] PROGMEM = R"CSSEOF(
/* Bienenwaage3 – Stylesheet (Light Mode) */
* { box-sizing: border-box; margin: 0; padding: 0; }
body { font-family: monospace; background: #f0f2f5; color: #333; font-size: 14px; }

/* ── Header ──────────────────────────────────────────────────────────────── */
.site-header {
  display: grid; grid-template-columns: 1fr auto 1fr; align-items: center;
  background: #fff; padding: 6px 16px;
  border-bottom: 2px solid #e89010;
  box-shadow: 0 1px 3px rgba(0,0,0,0.08);
}
.header-title { font-size: 16px; font-weight: bold; color: #e89010; letter-spacing: 1px; }
.fw-badge {
  font-size: 11px; color: #666; background: #fdf6e8;
  border: 1px solid #e0c888; padding: 3px 10px; border-radius: 3px;
  justify-self: end;
}

/* ── Tabs ────────────────────────────────────────────────────────────────── */
.tabs { display: flex; background: #fff; border-bottom: 2px solid #e0e0e0; overflow-x: auto; }
.tab-btn { padding: 10px 16px; background: none; border: none; color: #777;
           cursor: pointer; font: inherit; font-size: 13px; white-space: nowrap; }
.tab-btn.active { color: #e89010; border-bottom: 2px solid #e89010;
                  margin-bottom: -2px; font-weight: bold; }
.tab-btn:hover { color: #333; background: #fafafa; }
.tab-content { display: none; padding: 16px; max-width: 820px; }
.tab-content.active { display: block; }

/* ── Karten ──────────────────────────────────────────────────────────────── */
.card { background: #fff; border: 1px solid #e4e4e4; border-radius: 8px;
        padding: 14px; margin-bottom: 12px;
        box-shadow: 0 1px 4px rgba(0,0,0,0.06); }
.card h3 { color: #e89010; margin-bottom: 10px; font-size: 11px;
           text-transform: uppercase; letter-spacing: 1px;
           border-bottom: 1px solid #f5ead8; padding-bottom: 6px; }

/* ── Modulzeilen ─────────────────────────────────────────────────────────── */
.module-row { display: flex; align-items: center; gap: 10px;
              padding: 6px 0; border-bottom: 1px solid #f4f4f4; }
.module-row:last-child { border-bottom: none; }
.dot { width: 10px; height: 10px; border-radius: 50%; background: #ccc; flex-shrink: 0; }
.dot.online { background: #3a3; }
.module-name { width: 80px; color: #777; }
.module-weight { width: 110px; font-size: 16px; font-weight: bold; color: #222; }
.module-sigma { width: 80px; color: #999; font-size: 12px; }
.module-quick { width: 100px; color: #777; font-size: 12px; }

/* ── LCD-Simulation (scharf, kein Glow/Blur) ─────────────────────────────── */
.lcd-frame {
  display: inline-block;
  background: #1c3a1c;
  border: 4px solid #0d1f0d;
  border-radius: 3px;
  padding: 12px 18px;
  font-family: 'Courier New', Courier, monospace;
  box-shadow: 0 3px 10px rgba(0,0,0,0.4), inset 0 0 0 1px rgba(255,255,255,0.04);
}
.lcd-line {
  display: block;
  color: #33ff22;
  font-size: 21px;
  font-weight: bold;
  letter-spacing: 3.5px;
  line-height: 1.85;
  white-space: pre;
  width: 16ch;
  text-shadow: none;
  -webkit-font-smoothing: antialiased;
}

/* ── ETH-Status-Raster ───────────────────────────────────────────────────── */
.eth-status-grid {
  display: grid;
  grid-template-columns: 160px 1fr;
  row-gap: 6px;
  align-items: center;
}
.eth-status-grid .lbl { color: #888; font-size: 13px; }
.eth-status-grid .val { color: #222; font-weight: bold; font-size: 13px; font-family: monospace; }

/* ── Formulare ───────────────────────────────────────────────────────────── */
label { display: inline-block; width: 160px; color: #666; margin: 4px 0; }
input[type=text], input[type=number], input[type=password] {
  background: #fafafa; border: 1px solid #ccc; color: #333;
  padding: 5px 8px; border-radius: 4px; width: 180px; font: inherit;
}
input[type=text]:focus, input[type=number]:focus, input[type=password]:focus {
  outline: none; border-color: #e89010;
  box-shadow: 0 0 0 2px rgba(232,144,16,0.15);
}
input[type=checkbox] { margin-right: 6px; width: auto; vertical-align: middle; }
.form-row { margin: 6px 0; }
.note { color: #999; font-size: 12px; margin-bottom: 10px; line-height: 1.4; }

/* ── Buttons ─────────────────────────────────────────────────────────────── */
button.btn { background: #e89010; border: none; color: #fff; padding: 7px 16px;
             border-radius: 4px; cursor: pointer; font: inherit; font-weight: bold;
             margin: 4px 2px; }
button.btn:hover { background: #cf7a00; }
button.btn.danger { background: #c33; color: #fff; }
button.btn.danger:hover { background: #b02020; }
button.btn.secondary { background: #ebebeb; color: #444; border: 1px solid #d0d0d0; }
button.btn.secondary:hover { background: #d8d8d8; }

/* ── Verbindungsstatus ───────────────────────────────────────────────────── */
.status-bar { display: flex; gap: 16px; align-items: center; padding: 6px 0; }
.status-item { display: flex; align-items: center; gap: 5px; font-size: 13px; color: #666; }
.status-item .dot { width: 8px; height: 8px; }

/* ── Fortschrittsbalken OTA ──────────────────────────────────────────────── */
#ota-progress { display: none; margin-top: 8px; }
progress { width: 100%; height: 12px; }

/* ── Temperaturanzeige ───────────────────────────────────────────────────── */
.temp-value { font-size: 22px; color: #2277cc; font-weight: bold; }
)CSSEOF";

static const char WEB_APP_JS[] PROGMEM = R"JSEOF(
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
)JSEOF";

