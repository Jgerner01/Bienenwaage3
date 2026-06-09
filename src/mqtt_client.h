// Bienenwaage3 – MQTT + Home Assistant Auto-Discovery
// PubSubClient mit eigener Reconnect-Logik (kein WiFiClient-Polling)
// Autor: Johann Gerner

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "storage.h"

class MqttManager {
public:
    void begin();
    void loop();

    bool isConnected() { return _client.connected(); }
    void publish(const String& topic, const String& payload, bool retain = true);

private:
    WiFiClient    _wifiClient;
    PubSubClient  _client{_wifiClient};
    NetworkConfig _cfg;

    unsigned long _lastReconnectMs = 0;
    bool          _haDiscoverySent = false;

    void _connect();
    void _publishAllModules();
    void _publishHaDiscovery();
    String _moduleTopic(uint8_t idx, const char* suffix);
};
