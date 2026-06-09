// Bienenwaage3 – NVS-Persistenz (Preferences) und JSON Import/Export
// Autor: Johann Gerner

#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

// Vorwärtsdeklaration – vermeidet zirkulären Include
struct ModuleData;

struct NetworkConfig {
    bool   useDhcp      = true;
    String staticIp     = "";
    String gateway      = "";
    String subnet       = "";
    String apSsid       = WIFI_AP_SSID;
    String apPassword   = WIFI_AP_PASSWORD;
    String mqttServer   = "";
    int    mqttPort     = MQTT_DEFAULT_PORT;
    String mqttUser     = "";
    String mqttPassword = "";
    String mqttPrefix   = MQTT_TOPIC_PREFIX;
    bool   haDiscovery  = true;
    bool   mqttRetain   = true;
};

class StorageManager {
public:
    void begin();

    // Modulparameter
    void loadModuleParams(uint8_t idx, ModuleData& mod);
    void saveModuleParams(uint8_t idx, ModuleData& mod);

    // Netzwerkkonfiguration
    void loadNetworkConfig(NetworkConfig& cfg);
    void saveNetworkConfig(const NetworkConfig& cfg);

    // Alle Daten löschen (Werkseinstellungen)
    void factoryReset();

    // JSON Import/Export (LittleFS)
    bool exportToFile(const String& path);
    bool importFromFile(const String& path);
};
