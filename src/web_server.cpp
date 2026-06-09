// Bienenwaage3 – AsyncWebServer
// Autor: Johann Gerner

#include "web_server.h"
#include "web_content.h"
#include "build_number.h"
#include "hx711_multi.h"
#include "temperature.h"
#include "lcd_display.h"
#include "eth_wifi_manager.h"
#include "mqtt_client.h"
#include "storage.h"
#include "button_handler.h"
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

extern Hx711Multi        hx711;
extern TemperatureSensor tempSensor;
extern LcdDisplay        lcd;
extern EthWifiManager    network;
extern MqttManager       mqttClient;
extern StorageManager    storage;
extern ButtonHandler     buttons;

// ── Wiederherstellungsseite (PROGMEM) – wird angezeigt wenn LittleFS leer ist ──
static const char RECOVERY_PAGE[] PROGMEM = R"html(
<!DOCTYPE html><html><head><meta charset="UTF-8">
<title>Bienenwaage3 – Wiederherstellung</title>
<style>
body{font-family:sans-serif;background:#1a1a2e;color:#eee;padding:20px;max-width:520px;margin:auto}
h1{color:#e94560}h3{color:#ccc;margin-top:0}
.card{background:#16213e;border-radius:8px;padding:20px;margin:12px 0}
.warn{color:#f90;margin-bottom:16px}
input[type=file]{display:block;margin:10px 0;color:#eee}
button{background:#e94560;color:#fff;border:none;padding:8px 18px;border-radius:4px;cursor:pointer;font-size:14px}
progress{width:100%;height:10px;margin-top:8px;display:none}
.ok{color:#4caf50}.err{color:#e94560}
</style></head><body>
<h1>Bienenwaage3</h1>
<p class="warn">&#9888; Weboberfläche fehlt (LittleFS leer).<br>
Bitte zuerst <b>littlefs.bin</b> hochladen, danach ist die Seite verfügbar.</p>
<div class="card">
  <h3>Webdateien hochladen &ndash; <code>littlefs.bin</code></h3>
  <input type="file" id="fs-file" accept=".bin">
  <button onclick="upload('fs-file','/update-fs','fs-p','fs-s')">LittleFS hochladen &amp; Neustart</button>
  <progress id="fs-p"></progress><div id="fs-s"></div>
</div>
<div class="card">
  <h3>Firmware aktualisieren &ndash; <code>firmware.bin</code></h3>
  <input type="file" id="fw-file" accept=".bin">
  <button onclick="upload('fw-file','/update','fw-p','fw-s')">Firmware hochladen &amp; Neustart</button>
  <progress id="fw-p"></progress><div id="fw-s"></div>
</div>
<script>
function upload(fileId,url,barId,statusId){
  var f=document.getElementById(fileId).files[0];
  if(!f){alert('Keine Datei gewählt');return;}
  var fd=new FormData();fd.append('file',f);
  var xhr=new XMLHttpRequest();
  xhr.open('POST',url);
  xhr.upload.onprogress=function(e){
    var b=document.getElementById(barId);
    b.style.display='block';b.value=e.loaded;b.max=e.total;
  };
  xhr.onload=function(){
    var s=document.getElementById(statusId);
    if(xhr.status===200){s.innerHTML='<span class="ok">&#10003; Erfolgreich – ESP startet neu…</span>';}
    else{s.innerHTML='<span class="err">Fehler: '+xhr.responseText+'</span>';}
  };
  xhr.send(fd);
}
</script></body></html>
)html";

// ── begin ──────────────────────────────────────────────────────────────────────

void WebServerManager::begin() {
    // ArduinoOTA (Methode A)
    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.setPassword(OTA_PASSWORD);
    ArduinoOTA.onStart([]()   { lcdPrint(0, "OTA Start..."); });
    ArduinoOTA.onEnd([]()     { lcdPrint(0, "OTA OK-Neustart"); delay(1000); });
    ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
        char buf[16];
        snprintf(buf, sizeof(buf), "OTA %3d%%", (int)(100 * p / t));
        lcdPrint(1, buf);
    });
    ArduinoOTA.onError([](ota_error_t e) {
        lcdPrint(0, "OTA Fehler!");
        lcdPrint(1, String(e));
    });
    ArduinoOTA.begin();

    _setupRoutes();
    _server.begin();

#ifdef DEBUG_SERIAL
    Serial.printf("[Web] Server gestartet auf Port %d\n", WEB_SERVER_PORT);
#endif
}

// ── Routen ─────────────────────────────────────────────────────────────────────

void WebServerManager::_setupRoutes() {
    // Webseiten aus PROGMEM – kein LittleFS-Image nötig
    auto sendNoCache = [](AsyncWebServerRequest* req, const char* mime, const char* content) {
        AsyncWebServerResponse* resp = req->beginResponse(200, mime, content);
        resp->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        req->send(resp);
    };
    _server.on("/",          HTTP_GET, [sendNoCache](AsyncWebServerRequest* req) {
        sendNoCache(req, "text/html",             WEB_INDEX_HTML); });
    _server.on("/index.html",HTTP_GET, [sendNoCache](AsyncWebServerRequest* req) {
        sendNoCache(req, "text/html",             WEB_INDEX_HTML); });
    _server.on("/style.css", HTTP_GET, [sendNoCache](AsyncWebServerRequest* req) {
        sendNoCache(req, "text/css",              WEB_STYLE_CSS); });
    _server.on("/app.js",    HTTP_GET, [sendNoCache](AsyncWebServerRequest* req) {
        sendNoCache(req, "application/javascript", WEB_APP_JS); });

    // JSON-Daten für AJAX
    _server.on("/data",   HTTP_GET, [this](AsyncWebServerRequest* r) { _handleData(r); });
    _server.on("/config", HTTP_GET, [this](AsyncWebServerRequest* r) { _handleConfig(r); });
    _server.on("/status", HTTP_GET, [this](AsyncWebServerRequest* r) { _handleStatus(r); });
    _server.on("/export", HTTP_GET, [this](AsyncWebServerRequest* r) { _handleExport(r); });

    // POST-Endpoints mit Body
    auto bodyHandler = [](AsyncWebServerRequest*, uint8_t*, size_t, size_t, bool) {};

    _server.on("/set", HTTP_POST,
        [](AsyncWebServerRequest* r) {},
        nullptr,
        [this](AsyncWebServerRequest* r, uint8_t* d, size_t l, size_t, bool) {
            _handleSet(r, d, l);
        });

    _server.on("/tare", HTTP_POST,
        [](AsyncWebServerRequest* r) {},
        nullptr,
        [this](AsyncWebServerRequest* r, uint8_t* d, size_t l, size_t, bool) {
            _handleTare(r, d, l);
        });

    _server.on("/calibrate", HTTP_POST,
        [](AsyncWebServerRequest* r) {},
        nullptr,
        [this](AsyncWebServerRequest* r, uint8_t* d, size_t l, size_t, bool) {
            _handleCalibrate(r, d, l);
        });

    _server.on("/import", HTTP_POST,
        [](AsyncWebServerRequest* r) {},
        nullptr,
        [this](AsyncWebServerRequest* r, uint8_t* d, size_t l, size_t, bool) {
            _handleImport(r, d, l);
        });

    _server.on("/reset", HTTP_POST,
        [](AsyncWebServerRequest* r) {},
        nullptr,
        [this](AsyncWebServerRequest* r, uint8_t* d, size_t l, size_t, bool) {
            _handleReset(r, d, l);
        });

    // OTA-Upload Firmware (Methode B – Browser)
    _server.on("/update", HTTP_POST,
        [this](AsyncWebServerRequest* r) { _handleOtaResponse(r); },
        [this](AsyncWebServerRequest* r, const String& fn, size_t idx, uint8_t* d, size_t l, bool fin) {
            _handleOtaUpload(r, fn, idx, d, l, fin);
        });

    // OTA-Upload LittleFS-Image (Browser – auch von Wiederherstellungsseite)
    _server.on("/update-fs", HTTP_POST,
        [this](AsyncWebServerRequest* r) { _handleFsResponse(r); },
        [this](AsyncWebServerRequest* r, const String& fn, size_t idx, uint8_t* d, size_t l, bool fin) {
            _handleFsUpload(r, fn, idx, d, l, fin);
        });

    // Web-Simulation der physischen Taster
    _server.on("/btn1", HTTP_POST, [](AsyncWebServerRequest* r) {
        buttons.simulateBtn1();
        r->send(200, "application/json", "{\"ok\":true}");
    });
    _server.on("/btn2", HTTP_POST, [](AsyncWebServerRequest* r) {
        buttons.simulateBtn2();
        r->send(200, "application/json", "{\"ok\":true}");
    });
}

// ── /data ──────────────────────────────────────────────────────────────────────

void WebServerManager::_handleData(AsyncWebServerRequest* req) {
    JsonDocument doc;

    JsonArray mods = doc["modules"].to<JsonArray>();
    for (uint8_t i = 0; i < hx711.getModuleCount(); i++) {
        const ModuleData& m = hx711.getModule(i);
        JsonObject o = mods.add<JsonObject>();
        o["id"]             = i;
        o["active"]         = m.active;
        o["weight_g"]       = m.weightMain_g;
        o["weight_quick_g"] = m.weightQuick_g;
        o["sigma_g"]        = m.sigma_g;
        o["tare_main_g"]    = m.tareMain_g;
        o["tare_yield_g"]   = m.tareYield_g;
        o["online"]         = m.online;
    }

    doc["temperature"]["value_c"] = tempSensor.getTemperature();
    doc["temperature"]["online"]  = tempSensor.isOnline();

    doc["lcd"]["line1"] = lcd.getLine(0);
    doc["lcd"]["line2"] = lcd.getLine(1);

    doc["network"]["eth_ip"]         = ETH.localIP().toString();
    doc["network"]["eth_gateway"]    = ETH.gatewayIP().toString();
    doc["network"]["eth_subnet"]     = ETH.subnetMask().toString();
    doc["network"]["eth_mac"]        = ETH.macAddress();
    doc["network"]["ap_ip"]          = WiFi.softAPIP().toString();
    doc["network"]["eth_state"]      = (int)network.getState();
    doc["network"]["mqtt_connected"] = mqttClient.isConnected();

    doc["uptime_s"] = millis() / 1000;

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

// ── /config ────────────────────────────────────────────────────────────────────

void WebServerManager::_handleConfig(AsyncWebServerRequest* req) {
    const NetworkConfig& cfg = network.getConfig();
    JsonDocument doc;

    doc["useDhcp"]      = cfg.useDhcp;
    doc["staticIp"]     = cfg.staticIp;
    doc["gateway"]      = cfg.gateway;
    doc["subnet"]       = cfg.subnet;
    doc["apSsid"]       = cfg.apSsid;
    doc["apPassword"]   = cfg.apPassword;
    doc["mqttServer"]   = cfg.mqttServer;
    doc["mqttPort"]     = cfg.mqttPort;
    doc["mqttUser"]     = cfg.mqttUser;
    doc["mqttPassword"] = cfg.mqttPassword;
    doc["mqttPrefix"]   = cfg.mqttPrefix;
    doc["haDiscovery"]  = cfg.haDiscovery;
    doc["mqttRetain"]   = cfg.mqttRetain;

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

// ── /status ────────────────────────────────────────────────────────────────────

void WebServerManager::_handleStatus(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["eth_state"]      = (int)network.getState();
    doc["eth_ip"]         = network.getLocalIp();
    doc["mqtt_connected"] = mqttClient.isConnected();
    doc["fw_version"]     = FW_VERSION;
    doc["build"]          = BUILD_NUMBER;
    doc["uptime_s"]       = millis() / 1000;

    String body;
    serializeJson(doc, body);
    req->send(200, "application/json", body);
}

// ── /set ───────────────────────────────────────────────────────────────────────

void WebServerManager::_handleSet(AsyncWebServerRequest* req, uint8_t* data, size_t len) {
    JsonDocument doc;
    if (deserializeJson(doc, data, len)) {
        req->send(400, "application/json", "{\"error\":\"invalid JSON\"}");
        return;
    }

    // Modulparameter
    if (doc.containsKey("module")) {
        uint8_t idx = doc["module"].as<uint8_t>();
        if (idx < HX711_MAX_MODULES) {
            ModuleData mod = hx711.getModule(idx);
            if (doc.containsKey("active"))      mod.active         = doc["active"];
            if (doc.containsKey("calibFactor")) mod.calibFactor    = doc["calibFactor"];
            if (doc.containsKey("polyA2"))      mod.polyA2         = doc["polyA2"];
            if (doc.containsKey("polyA1"))      mod.polyA1         = doc["polyA1"];
            if (doc.containsKey("polyA0"))      mod.polyA0         = doc["polyA0"];
            if (doc.containsKey("bufSize"))     mod.mainBufferSize = doc["bufSize"];
            if (doc.containsKey("outlier"))     mod.outlierThresh  = doc["outlier"];
            storage.saveModuleParams(idx, mod);
        }
    }

    // Netzwerkkonfiguration
    if (doc.containsKey("network")) {
        NetworkConfig cfg = network.getConfig();
        JsonVariant n = doc["network"];
        if (n.containsKey("apSsid"))       cfg.apSsid       = n["apSsid"].as<String>();
        if (n.containsKey("apPassword"))   cfg.apPassword   = n["apPassword"].as<String>();
        if (n.containsKey("useDhcp"))      cfg.useDhcp      = n["useDhcp"];
        if (n.containsKey("staticIp"))     cfg.staticIp     = n["staticIp"].as<String>();
        if (n.containsKey("gateway"))      cfg.gateway      = n["gateway"].as<String>();
        if (n.containsKey("subnet"))       cfg.subnet       = n["subnet"].as<String>();
        if (n.containsKey("mqttServer"))   cfg.mqttServer   = n["mqttServer"].as<String>();
        if (n.containsKey("mqttPort"))     cfg.mqttPort     = n["mqttPort"];
        if (n.containsKey("mqttUser"))     cfg.mqttUser     = n["mqttUser"].as<String>();
        if (n.containsKey("mqttPassword")) cfg.mqttPassword = n["mqttPassword"].as<String>();
        if (n.containsKey("mqttPrefix"))   cfg.mqttPrefix   = n["mqttPrefix"].as<String>();
        if (n.containsKey("haDiscovery"))  cfg.haDiscovery  = n["haDiscovery"];
        if (n.containsKey("mqttRetain"))   cfg.mqttRetain   = n["mqttRetain"];
        network.applyNetworkConfig(cfg); // speichert + startet neu
    }

    req->send(200, "application/json", "{\"ok\":true}");
}

// ── /tare ──────────────────────────────────────────────────────────────────────

void WebServerManager::_handleTare(AsyncWebServerRequest* req, uint8_t* data, size_t len) {
    JsonDocument doc;
    if (deserializeJson(doc, data, len)) { req->send(400); return; }

    uint8_t m     = doc["module"] | 0;
    String  type  = doc["type"]   | "main";

    if      (type == "main")  hx711.startTareMain(m);
    else if (type == "yield") hx711.startTareYield(m);
    else if (type == "quick") hx711.startTareQuick(m);
    else if (type == "clear_yield") hx711.clearTareYield(m);

    req->send(200, "application/json", "{\"ok\":true}");
}

// ── /calibrate ─────────────────────────────────────────────────────────────────

void WebServerManager::_handleCalibrate(AsyncWebServerRequest* req, uint8_t* data, size_t len) {
    JsonDocument doc;
    if (deserializeJson(doc, data, len)) { req->send(400); return; }

    uint8_t m    = doc["module"]       | 0;
    float   known = doc["known_weight"] | 0.0f;
    hx711.calibrate(m, known);
    req->send(200, "application/json", "{\"ok\":true}");
}

// ── /export ────────────────────────────────────────────────────────────────────

void WebServerManager::_handleExport(AsyncWebServerRequest* req) {
    storage.exportToFile("/export.json");
    req->send(LittleFS, "/export.json", "application/json", true);
}

// ── /import ────────────────────────────────────────────────────────────────────

void WebServerManager::_handleImport(AsyncWebServerRequest* req, uint8_t* data, size_t len) {
    File f = LittleFS.open("/import.json", "w");
    if (!f) { req->send(500); return; }
    f.write(data, len);
    f.close();

    if (storage.importFromFile("/import.json")) {
        req->send(200, "application/json", "{\"ok\":true}");
        delay(500);
        ESP.restart();
    } else {
        req->send(400, "application/json", "{\"error\":\"import failed\"}");
    }
}

// ── /reset ─────────────────────────────────────────────────────────────────────

void WebServerManager::_handleReset(AsyncWebServerRequest* req, uint8_t* data, size_t len) {
    storage.factoryReset();
    req->send(200, "application/json", "{\"ok\":true}");
    delay(500);
    ESP.restart();
}

// ── OTA Browser-Upload ─────────────────────────────────────────────────────────

void WebServerManager::_handleOtaResponse(AsyncWebServerRequest* req) {
    bool ok = !Update.hasError();
    req->send(200, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"OTA fehlgeschlagen\"}");
    if (ok) { delay(1000); ESP.restart(); }
}

// ── LittleFS-Image Upload ──────────────────────────────────────────────────────

void WebServerManager::_handleFsResponse(AsyncWebServerRequest* req) {
    bool ok = !Update.hasError();
    req->send(200, "application/json", ok ? "{\"ok\":true}" : "{\"error\":\"FS-Update fehlgeschlagen\"}");
    if (ok) { delay(1000); ESP.restart(); }
}

void WebServerManager::_handleFsUpload(AsyncWebServerRequest* /*req*/,
                                        const String& /*filename*/,
                                        size_t index, uint8_t* data,
                                        size_t len, bool final) {
    if (index == 0) {
        lcdPrint(0, "FS Upload...");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
#ifdef DEBUG_SERIAL
            Update.printError(Serial);
#endif
        }
    }
    Update.write(data, len);
    char buf[16];
    snprintf(buf, sizeof(buf), "%u KB", (unsigned)(index + len) / 1024);
    lcdPrint(1, buf);
    if (final) {
        if (Update.end(true)) {
            lcdPrint(0, "FS OK");
            lcdPrint(1, "Neustart...");
        } else {
            lcdPrint(0, "FS Fehler!");
#ifdef DEBUG_SERIAL
            Update.printError(Serial);
#endif
        }
    }
}

// ── Firmware-Upload ────────────────────────────────────────────────────────────

void WebServerManager::_handleOtaUpload(AsyncWebServerRequest* req,
                                         const String& /*filename*/,
                                         size_t index, uint8_t* data,
                                         size_t len, bool final) {
    if (index == 0) {
        lcdPrint(0, "OTA Upload...");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
#ifdef DEBUG_SERIAL
            Update.printError(Serial);
#endif
        }
    }
    if (Update.write(data, len) != len) {
#ifdef DEBUG_SERIAL
        Update.printError(Serial);
#endif
    }
    char buf[16];
    snprintf(buf, sizeof(buf), "%u KB", (unsigned)(index + len) / 1024);
    lcdPrint(1, buf);

    if (final) {
        if (Update.end(true)) {
            lcdPrint(0, "OTA OK");
            lcdPrint(1, "Neustart...");
        } else {
            lcdPrint(0, "OTA Fehler!");
        }
    }
}
