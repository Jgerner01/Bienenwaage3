// Bienenwaage3 – NVS-Persistenz (Preferences) und JSON Import/Export
// Autor: Johann Gerner

#include "storage.h"
#include "hx711_multi.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

void StorageManager::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[Storage] FEHLER: LittleFS konnte nicht gemountet werden!");
        return;
    }
    Serial.printf("[Storage] LittleFS OK – %u Bytes frei\n",
                  LittleFS.totalBytes() - LittleFS.usedBytes());
    if (!LittleFS.exists("/index.html")) {
        Serial.println("[Storage] WARNUNG: /index.html fehlt – bitte 'uploadfs' ausfuehren!");
    }
}

// ── Modulparameter ─────────────────────────────────────────────────────────────

void StorageManager::loadModuleParams(uint8_t idx, ModuleData& mod) {
    Preferences prefs;
    char ns[16];
    snprintf(ns, sizeof(ns), "bw3m%d", idx);
    prefs.begin(ns, true); // read-only

    mod.active          = prefs.getBool("active",   false);
    mod.calibFactor     = prefs.getFloat("calib",   1.0f);
    mod.tareMain_g      = prefs.getFloat("tareM",   0.0f);
    mod.tareYield_g     = prefs.getFloat("tareY",   0.0f);
    mod.polyA2          = prefs.getFloat("polyA2",  TEMP_POLY_A2_DEFAULT);
    mod.polyA1          = prefs.getFloat("polyA1",  TEMP_POLY_A1_DEFAULT);
    mod.polyA0          = prefs.getFloat("polyA0",  TEMP_POLY_A0_DEFAULT);
    mod.mainBufferSize  = prefs.getInt("bufSize",   HX711_MAIN_BUFFER_SIZE);
    mod.outlierThresh   = prefs.getFloat("outlier", HX711_OUTLIER_THRESHOLD);
    prefs.end();
}

void StorageManager::saveModuleParams(uint8_t idx, ModuleData& mod) {
    Preferences prefs;
    char ns[16];
    snprintf(ns, sizeof(ns), "bw3m%d", idx);
    prefs.begin(ns, false);

    prefs.putBool("active",  mod.active);
    prefs.putFloat("calib",  mod.calibFactor);
    prefs.putFloat("tareM",  mod.tareMain_g);
    prefs.putFloat("tareY",  mod.tareYield_g);
    prefs.putFloat("polyA2", mod.polyA2);
    prefs.putFloat("polyA1", mod.polyA1);
    prefs.putFloat("polyA0", mod.polyA0);
    prefs.putInt("bufSize",  mod.mainBufferSize);
    prefs.putFloat("outlier",mod.outlierThresh);
    prefs.end();
}

// ── Netzwerkkonfiguration ─────────────────────────────────────────────────────

void StorageManager::loadNetworkConfig(NetworkConfig& cfg) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE_NET, true);

    cfg.useDhcp      = prefs.getBool("dhcp",      true);
    cfg.staticIp     = prefs.getString("ip",       "");
    cfg.gateway      = prefs.getString("gw",       "");
    cfg.subnet       = prefs.getString("sn",       "255.255.255.0");
    cfg.apSsid       = prefs.getString("apSsid",   WIFI_AP_SSID);
    cfg.apPassword   = prefs.getString("apPass",   WIFI_AP_PASSWORD);
    cfg.mqttServer   = prefs.getString("mqttSrv",  "");
    cfg.mqttPort     = prefs.getInt("mqttPort",    MQTT_DEFAULT_PORT);
    cfg.mqttUser     = prefs.getString("mqttUser", "");
    cfg.mqttPassword = prefs.getString("mqttPw",   "");
    cfg.mqttPrefix   = prefs.getString("mqttPfx",  MQTT_TOPIC_PREFIX);
    cfg.haDiscovery  = prefs.getBool("haDisc",     true);
    cfg.mqttRetain   = prefs.getBool("retain",     true);
    prefs.end();
}

void StorageManager::saveNetworkConfig(const NetworkConfig& cfg) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE_NET, false);

    prefs.putBool("dhcp",      cfg.useDhcp);
    prefs.putString("ip",      cfg.staticIp);
    prefs.putString("gw",      cfg.gateway);
    prefs.putString("sn",      cfg.subnet);
    prefs.putString("apSsid",  cfg.apSsid);
    prefs.putString("apPass",  cfg.apPassword);
    prefs.putString("mqttSrv", cfg.mqttServer);
    prefs.putInt("mqttPort",   cfg.mqttPort);
    prefs.putString("mqttUser",cfg.mqttUser);
    prefs.putString("mqttPw",  cfg.mqttPassword);
    prefs.putString("mqttPfx", cfg.mqttPrefix);
    prefs.putBool("haDisc",    cfg.haDiscovery);
    prefs.putBool("retain",    cfg.mqttRetain);
    prefs.end();
}

// ── Werkseinstellungen ─────────────────────────────────────────────────────────

void StorageManager::factoryReset() {
    Preferences prefs;

    prefs.begin(NVS_NAMESPACE_NET, false);
    prefs.clear();
    prefs.end();

    for (uint8_t i = 0; i < HX711_MAX_MODULES; i++) {
        char ns[16];
        snprintf(ns, sizeof(ns), "bw3m%d", i);
        prefs.begin(ns, false);
        prefs.clear();
        prefs.end();
    }

#ifdef DEBUG_SERIAL
    Serial.println("[Storage] Werkseinstellungen wiederhergestellt");
#endif
}

// ── JSON Export ────────────────────────────────────────────────────────────────

bool StorageManager::exportToFile(const String& path) {
    JsonDocument doc;
    doc["format_version"] = PARAM_FORMAT_VER;

    JsonArray mods = doc["modules"].to<JsonArray>();
    for (uint8_t i = 0; i < HX711_MAX_MODULES; i++) {
        ModuleData tmp;
        loadModuleParams(i, tmp);
        JsonObject o = mods.add<JsonObject>();
        o["id"]      = i;
        o["active"]  = tmp.active;
        o["calib"]   = tmp.calibFactor;
        o["tareM"]   = tmp.tareMain_g;
        o["tareY"]   = tmp.tareYield_g;
        o["polyA2"]  = tmp.polyA2;
        o["polyA1"]  = tmp.polyA1;
        o["polyA0"]  = tmp.polyA0;
        o["bufSize"] = tmp.mainBufferSize;
        o["outlier"] = tmp.outlierThresh;
    }

    NetworkConfig net;
    loadNetworkConfig(net);
    doc["network"]["dhcp"]     = net.useDhcp;
    doc["network"]["ip"]       = net.staticIp;
    doc["network"]["gw"]       = net.gateway;
    doc["network"]["sn"]       = net.subnet;
    doc["network"]["mqttSrv"]  = net.mqttServer;
    doc["network"]["mqttPort"] = net.mqttPort;
    doc["network"]["mqttUser"] = net.mqttUser;
    doc["network"]["mqttPfx"]  = net.mqttPrefix;
    doc["network"]["haDisc"]   = net.haDiscovery;
    doc["network"]["retain"]   = net.mqttRetain;

    File f = LittleFS.open(path, "w");
    if (!f) return false;
    serializeJson(doc, f);
    f.close();
    return true;
}

// ── JSON Import ────────────────────────────────────────────────────────────────

bool StorageManager::importFromFile(const String& path) {
    File f = LittleFS.open(path, "r");
    if (!f) return false;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return false;

    if (doc["format_version"].as<int>() != PARAM_FORMAT_VER) return false;

    for (JsonObject o : doc["modules"].as<JsonArray>()) {
        uint8_t idx = o["id"].as<uint8_t>();
        if (idx >= HX711_MAX_MODULES) continue;
        ModuleData tmp;
        loadModuleParams(idx, tmp);
        tmp.active         = o["active"]  | tmp.active;
        tmp.calibFactor    = o["calib"]   | tmp.calibFactor;
        tmp.tareMain_g     = o["tareM"]   | tmp.tareMain_g;
        tmp.tareYield_g    = o["tareY"]   | tmp.tareYield_g;
        tmp.polyA2         = o["polyA2"]  | tmp.polyA2;
        tmp.polyA1         = o["polyA1"]  | tmp.polyA1;
        tmp.polyA0         = o["polyA0"]  | tmp.polyA0;
        tmp.mainBufferSize = o["bufSize"] | tmp.mainBufferSize;
        tmp.outlierThresh  = o["outlier"] | tmp.outlierThresh;
        saveModuleParams(idx, tmp);
    }

    if (doc.containsKey("network")) {
        NetworkConfig net;
        loadNetworkConfig(net);
        JsonVariant n = doc["network"];
        net.useDhcp   = n["dhcp"]     | net.useDhcp;
        net.staticIp  = n["ip"].as<String>();
        net.gateway   = n["gw"].as<String>();
        net.subnet    = n["sn"].as<String>();
        net.mqttServer= n["mqttSrv"].as<String>();
        net.mqttPort  = n["mqttPort"] | net.mqttPort;
        net.mqttUser  = n["mqttUser"].as<String>();
        net.mqttPrefix= n["mqttPfx"].as<String>();
        net.haDiscovery= n["haDisc"] | net.haDiscovery;
        net.mqttRetain = n["retain"] | net.mqttRetain;
        saveNetworkConfig(net);
    }
    return true;
}
