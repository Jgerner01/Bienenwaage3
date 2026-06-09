// Bienenwaage3 – Ethernet + WiFi-AP Zustandsautomat
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

    WiFi.onEvent(_onEthEvent);

    _enterState(State::ETH_CONNECTING);
    _startEthConnect();
}

void EthWifiManager::loop() {
    unsigned long now = millis();

    switch (_state) {
        case State::ETH_CONNECTING:
            if (now - _stateEnteredMs > ETH_CONNECT_TIMEOUT_MS) {
                // Timeout → Wiederverbinden
#ifdef DEBUG_SERIAL
                Serial.println("[ETH] Verbindungs-Timeout");
#endif
                _enterState(State::ETH_RECONNECTING);
            }
            break;

        case State::ETH_CONNECTED:
            // Verbindungsverlust wird per Event behandelt
            break;

        case State::ETH_RECONNECTING:
            if (now - _stateEnteredMs > ETH_MAX_RECONNECT_MS) {
                // Zu lange kein Erfolg → AP-Modus
                _enterState(State::AP_CONFIG_MODE);
                _startApMode();
            } else if (now - _lastReconnectMs > ETH_RECONNECT_INTERVAL_MS) {
                _lastReconnectMs = now;
                _reconnectCount++;
#ifdef DEBUG_SERIAL
                Serial.printf("[ETH] Reconnect-Versuch %d\n", _reconnectCount);
#endif
                _startEthConnect();
            }
            break;

        case State::AP_CONFIG_MODE:
            if (now - _stateEnteredMs > AP_CONFIG_TIMEOUT_MS) {
#ifdef DEBUG_SERIAL
                Serial.println("[ETH] AP-Timeout – Neustart");
#endif
                ESP.restart();
            }
            break;
    }
}

String EthWifiManager::getLocalIp() const {
    if (_state == State::ETH_CONNECTED) return ETH.localIP().toString();
    if (_state == State::AP_CONFIG_MODE) return WiFi.softAPIP().toString();
    return "0.0.0.0";
}

void EthWifiManager::applyNetworkConfig(const NetworkConfig& cfg) {
    _cfg = cfg;
    storage.saveNetworkConfig(cfg);
    // Neustart damit neue IP-Einstellungen wirksam werden
    delay(500);
    ESP.restart();
}

// ── Privat ─────────────────────────────────────────────────────────────────────

void EthWifiManager::_enterState(State s) {
    _state          = s;
    _stateEnteredMs = millis();
#ifdef DEBUG_SERIAL
    const char* names[] = {"ETH_CONNECTING","ETH_CONNECTED","ETH_RECONNECTING","AP_CONFIG_MODE"};
    Serial.printf("[ETH] → %s\n", names[(int)s]);
#endif
}

void EthWifiManager::_startEthConnect() {
    _enterState(State::ETH_CONNECTING);

    // WT32-ETH01: LAN8720 an RMII, Takt von GPIO0 (externer 50-MHz-Oszillator)
    ETH.begin(
        ETH_PHY_ADDR,
        ETH_PHY_POWER,
        ETH_PHY_MDC,
        ETH_PHY_MDIO,
        ETH_PHY_LAN8720,
        ETH_CLOCK_GPIO0_IN
    );

    if (!_cfg.useDhcp && _cfg.staticIp.length() > 0) {
        IPAddress ip, gw, sn;
        ip.fromString(_cfg.staticIp);
        gw.fromString(_cfg.gateway);
        sn.fromString(_cfg.subnet);
        ETH.config(ip, gw, sn);
    }

    lcdPrint(1, "ETH verbindet...");
}

void EthWifiManager::_startApMode() {
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
    String ip = WiFi.softAPIP().toString();

    lcdPrint(0, "AP: " + String(WIFI_AP_SSID));
    lcdPrint(1, ip);

#ifdef DEBUG_SERIAL
    Serial.printf("[ETH] AP-Modus aktiv – SSID: %s  IP: %s\n", WIFI_AP_SSID, ip.c_str());
#endif
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
    String ip = ETH.localIP().toString();
    lcdPrint(1, ip);
#ifdef DEBUG_SERIAL
    Serial.printf("[ETH] Verbunden – IP: %s\n", ip.c_str());
#endif
}

void EthWifiManager::_handleEthDisconnected() {
    if (_state == State::ETH_CONNECTED) {
        lcdPrint(1, "ETH getrennt");
        _enterState(State::ETH_RECONNECTING);
        _lastReconnectMs = millis();
    }
}
