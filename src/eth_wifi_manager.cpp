// Bienenwaage3 – Ethernet + WiFi-AP
// ETH.begin() einmalig, WiFi AP danach – mit vollständigem Debug-Output
// Autor: Johann Gerner

#include "eth_wifi_manager.h"
#include "lcd_display.h"
#include <ETH.h>
#include <WiFi.h>

extern StorageManager storage;

EthWifiManager* EthWifiManager::_instance = nullptr;

void EthWifiManager::begin() {
    _instance = this;
    storage.loadNetworkConfig(_cfg);

    Serial.println("[NET] === Netzwerk-Init Start ===");
    Serial.printf("[NET] AP SSID:     '%s'\n", _cfg.apSsid.c_str());
    Serial.printf("[NET] AP Passwort: '%s'\n", _cfg.apPassword.c_str());
    Serial.printf("[NET] useDhcp:     %d\n",   _cfg.useDhcp);
    Serial.printf("[NET] staticIp:    '%s'\n", _cfg.staticIp.c_str());

    WiFi.onEvent(_onEthEvent);

    // ── Schritt 1: WiFi-Modus vor ETH.begin() ─────────────────────────────────
    Serial.printf("[NET] WiFi Mode vor ETH.begin(): %d\n", (int)WiFi.getMode());

    // ── Schritt 2: Statische IP konfigurieren BEVOR ETH gestartet wird ────────
    if (!_cfg.useDhcp && _cfg.staticIp.length() > 0) {
        IPAddress ip, gw, sn;
        ip.fromString(_cfg.staticIp);
        gw.fromString(_cfg.gateway);
        sn.fromString(_cfg.subnet);
        ETH.config(ip, gw, sn);
        Serial.printf("[NET] Statische IP konfiguriert: %s\n", _cfg.staticIp.c_str());
    }

    // ── Schritt 3: ETH einmalig initialisieren ─────────────────────────────────
    Serial.println("[NET] Rufe ETH.begin() auf...");
    bool ethBeginOk = ETH.begin(
        ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO,
        ETH_PHY_LAN8720, ETH_CLOCK_GPIO0_IN
    );
    Serial.printf("[NET] ETH.begin() = %d\n", (int)ethBeginOk);
    Serial.printf("[NET] WiFi Mode nach ETH.begin(): %d\n", (int)WiFi.getMode());

    // 100 ms warten bis ETH-Init intern abgeschlossen
    delay(100);
    Serial.printf("[NET] WiFi Mode nach delay: %d\n", (int)WiFi.getMode());

    // ── Schritt 3: WiFi Modus auf AP setzen ───────────────────────────────────
    Serial.println("[NET] Setze WiFi.mode(WIFI_AP)...");
    bool modeOk = WiFi.mode(WIFI_AP);
    Serial.printf("[NET] WiFi.mode(WIFI_AP) = %d, Mode danach: %d\n",
                  (int)modeOk, (int)WiFi.getMode());
    delay(100);

    // ── Schritt 4: SoftAP starten ─────────────────────────────────────────────
    Serial.printf("[NET] Rufe WiFi.softAP('%s', '%s') auf...\n",
                  _cfg.apSsid.c_str(), _cfg.apPassword.c_str());
    bool apOk = WiFi.softAP(_cfg.apSsid.c_str(), _cfg.apPassword.c_str());
    Serial.printf("[NET] WiFi.softAP() = %d\n", (int)apOk);
    delay(500); // warten bis AP vollständig hochgefahren

    // ── Schritt 5: Ergebnis prüfen ────────────────────────────────────────────
    String apIp   = WiFi.softAPIP().toString();
    uint8_t mode  = (uint8_t)WiFi.getMode();
    Serial.printf("[NET] AP IP:    %s\n", apIp.c_str());
    Serial.printf("[NET] WiFi Mode final: %d\n", (int)mode);
    Serial.printf("[NET] Verbundene Clients: %d\n", WiFi.softAPgetStationNum());

    if (apIp == "0.0.0.0") {
        Serial.println("[NET] FEHLER: AP-IP ist 0.0.0.0 – softAP nicht gestartet!");
    } else {
        Serial.printf("[NET] WiFi AP bereit: SSID=%s  PW=%s  IP=%s\n",
                      _cfg.apSsid.c_str(), _cfg.apPassword.c_str(), apIp.c_str());
    }

    Serial.println("[NET] === Netzwerk-Init Ende ===");
    _enterState(State::ETH_CONNECTING);
}

void EthWifiManager::loop() {
    unsigned long now = millis();

    switch (_state) {
        case State::ETH_CONNECTING:
            if (now - _stateEnteredMs > ETH_CONNECT_TIMEOUT_MS) {
                Serial.println("[ETH] Timeout – kein Link, warte auf Kabel...");
                _enterState(State::ETH_RECONNECTING);
                _lastReconnectMs = now;
            }
            break;

        case State::ETH_CONNECTED:
            break;

        case State::ETH_RECONNECTING:
            if (now - _lastReconnectMs > ETH_RECONNECT_INTERVAL_MS) {
                _lastReconnectMs = now;
                _reconnectCount++;
                Serial.printf("[ETH] Warte auf ETH-Link (%d)...\n", _reconnectCount);
            }
            break;
    }
}

String EthWifiManager::getLocalIp() const {
    if (_state == State::ETH_CONNECTED) return ETH.localIP().toString();
    return WiFi.softAPIP().toString();
}

void EthWifiManager::applyNetworkConfig(const NetworkConfig& cfg) {
    _cfg = cfg;
    storage.saveNetworkConfig(cfg);
    delay(500);
    ESP.restart();
}

// ── Privat ─────────────────────────────────────────────────────────────────────

void EthWifiManager::_enterState(State s) {
    _state          = s;
    _stateEnteredMs = millis();
    const char* names[] = {"ETH_CONNECTING", "ETH_CONNECTED", "ETH_RECONNECTING"};
    Serial.printf("[ETH] Zustand: %s\n", names[(int)s]);
}

// ── Statischer Event-Handler ───────────────────────────────────────────────────

void EthWifiManager::_onEthEvent(arduino_event_id_t event, arduino_event_info_t /*info*/) {
    if (!_instance) return;
    switch (event) {
        case ARDUINO_EVENT_ETH_GOT_IP:
            _instance->_handleEthConnected();
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
        case ARDUINO_EVENT_ETH_STOP:
            _instance->_handleEthDisconnected();
            break;
        default:
            break;
    }
}

void EthWifiManager::_handleEthConnected() {
    _enterState(State::ETH_CONNECTED);
    _reconnectCount = 0;
    Serial.printf("[ETH] Verbunden – ETH-IP: %s  AP-IP: %s\n",
                  ETH.localIP().toString().c_str(),
                  WiFi.softAPIP().toString().c_str());
}

void EthWifiManager::_handleEthDisconnected() {
    if (_state == State::ETH_CONNECTED) {
        _enterState(State::ETH_RECONNECTING);
        _lastReconnectMs = millis();
        Serial.println("[ETH] Verbindung verloren");
    }
}
