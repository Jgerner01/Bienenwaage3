// Bienenwaage3 – MQTT + Home Assistant Auto-Discovery
// Autor: Johann Gerner

#include "mqtt_client.h"
#include "hx711_multi.h"
#include "temperature.h"
#include "eth_wifi_manager.h"
#include <ArduinoJson.h>

extern Hx711Multi        hx711;
extern TemperatureSensor tempSensor;
extern EthWifiManager    network;
extern StorageManager    storage;

static unsigned long _lastPublishMs = 0;
static constexpr unsigned long PUBLISH_INTERVAL_MS = 10000UL; // 10 s

void MqttManager::begin() {
    storage.loadNetworkConfig(_cfg);

    if (_cfg.mqttServer.length() == 0) return;

    _client.setServer(_cfg.mqttServer.c_str(), _cfg.mqttPort);
    _client.setKeepAlive(MQTT_KEEPALIVE_S);
    // Kurzer Socket-Timeout: blockiert den loop() bei nicht erreichbarem Broker
    // nur kurz statt der Default-15 s (verhindert Messung-/Webserver-Stocken)
    _client.setSocketTimeout(MQTT_SOCKET_TIMEOUT_S);
    // Verbindungsaufbau erfolgt in loop() sobald ETH verbunden ist
}

void MqttManager::loop() {
    if (_cfg.mqttServer.length() == 0) return;
    if (!network.isConnected()) return;

    if (!_client.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectMs > MQTT_RECONNECT_INTERVAL) {
            _lastReconnectMs = now;
            _connect();
        }
        return;
    }

    _client.loop();

    // HA Discovery einmalig nach Verbindungsaufbau senden
    if (!_haDiscoverySent && _cfg.haDiscovery) {
        _publishHaDiscovery();
        _haDiscoverySent = true;
    }

    // Messwerte periodisch veröffentlichen
    if (millis() - _lastPublishMs >= PUBLISH_INTERVAL_MS) {
        _lastPublishMs = millis();
        _publishAllModules();
    }
}

void MqttManager::publish(const String& topic, const String& payload, bool retain) {
    if (!_client.connected()) return;
    _client.publish(topic.c_str(), payload.c_str(), retain);
}

// ── Verbindung ─────────────────────────────────────────────────────────────────

void MqttManager::_connect() {
    bool ok;
    if (_cfg.mqttUser.length() > 0)
        ok = _client.connect(MQTT_CLIENT_ID, _cfg.mqttUser.c_str(), _cfg.mqttPassword.c_str());
    else
        ok = _client.connect(MQTT_CLIENT_ID);

#ifdef DEBUG_SERIAL
    Serial.printf("[MQTT] %s → %s:%d\n",
        ok ? "Verbunden" : "Fehler",
        _cfg.mqttServer.c_str(), _cfg.mqttPort);
#endif

    if (ok) _haDiscoverySent = false; // Discovery nach Reconnect erneut senden
}

// ── Messwerte ─────────────────────────────────────────────────────────────────

void MqttManager::_publishAllModules() {
    bool retain = _cfg.mqttRetain;

    for (uint8_t i = 0; i < hx711.getModuleCount(); i++) {
        const ModuleData& m = hx711.getModule(i);
        if (!m.active || !m.online) continue;

        publish(_moduleTopic(i, "gewicht"),         String(m.weightMain_g,  1), retain);
        publish(_moduleTopic(i, "gewicht_schnell"),  String(m.weightQuick_g, 1), retain);
        publish(_moduleTopic(i, "sigma"),            String(m.sigma_g,        2), retain);
        publish(_moduleTopic(i, "online"),           "true",                     retain);
    }

    if (tempSensor.isOnline()) {
        String tempTopic = _cfg.mqttPrefix + "/temperatur";
        publish(tempTopic, String(tempSensor.getTemperature(), 2), retain);
    }
}

String MqttManager::_moduleTopic(uint8_t idx, const char* suffix) {
    return _cfg.mqttPrefix + "/modul" + (idx + 1) + "/" + suffix;
}

// ── Home Assistant Auto-Discovery ─────────────────────────────────────────────

void MqttManager::_publishHaDiscovery() {
    for (uint8_t i = 0; i < hx711.getModuleCount(); i++) {
        if (!hx711.getModule(i).active) continue;

        String uid      = String("bw3_m") + (i + 1);
        String stateTopic = _moduleTopic(i, "gewicht");

        JsonDocument doc;
        doc["name"]              = String("Bienenwaage3 Modul ") + (i + 1);
        doc["unique_id"]         = uid + "_gewicht";
        doc["state_topic"]       = stateTopic;
        doc["unit_of_measurement"] = "g";
        doc["device_class"]      = "weight";
        doc["value_template"]    = "{{ value }}";
        doc["retain"]            = _cfg.mqttRetain;

        JsonObject dev = doc["device"].to<JsonObject>();
        dev["identifiers"].add("bienenwaage3");
        dev["name"]              = "Bienenwaage3";
        dev["model"]             = "WT32-ETH01";
        dev["manufacturer"]      = "DIY";

        String cfgTopic = String(HA_DISCOVERY_PREFIX) + "/sensor/" + uid + "_gewicht/config";
        String payload;
        serializeJson(doc, payload);
        _client.publish(cfgTopic.c_str(), payload.c_str(), true);
    }

    // Temperatur
    {
        JsonDocument doc;
        doc["name"]               = "Bienenwaage3 Temperatur";
        doc["unique_id"]          = "bw3_temperatur";
        doc["state_topic"]        = _cfg.mqttPrefix + "/temperatur";
        doc["unit_of_measurement"]= "°C";
        doc["device_class"]       = "temperature";
        doc["value_template"]     = "{{ value }}";

        JsonObject dev = doc["device"].to<JsonObject>();
        dev["identifiers"].add("bienenwaage3");
        dev["name"]               = "Bienenwaage3";

        String cfgTopic = String(HA_DISCOVERY_PREFIX) + "/sensor/bw3_temperatur/config";
        String payload;
        serializeJson(doc, payload);
        _client.publish(cfgTopic.c_str(), payload.c_str(), true);
    }

#ifdef DEBUG_SERIAL
    Serial.println("[MQTT] HA Auto-Discovery gesendet");
#endif
}
