// Bienenwaage3 – AsyncWebServer mit REST-Endpoints und OTA-Upload
// Weboberfläche aus LittleFS, AJAX /data JSON, kein Seiten-Reload
// Autor: Johann Gerner

#pragma once
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "config.h"

class WebServerManager {
public:
    void begin();

private:
    AsyncWebServer _server{WEB_SERVER_PORT};

    void _setupRoutes();

    // REST-Handler
    void _handleData(AsyncWebServerRequest* req);
    void _handleConfig(AsyncWebServerRequest* req);
    void _handleSet(AsyncWebServerRequest* req, uint8_t* data, size_t len);
    void _handleTare(AsyncWebServerRequest* req, uint8_t* data, size_t len);
    void _handleCalibrate(AsyncWebServerRequest* req, uint8_t* data, size_t len);
    void _handleExport(AsyncWebServerRequest* req);
    void _handleImport(AsyncWebServerRequest* req, uint8_t* data, size_t len);
    void _handleReset(AsyncWebServerRequest* req, uint8_t* data, size_t len);
    void _handleStatus(AsyncWebServerRequest* req);

    // OTA-Upload Firmware (HTTP)
    void _handleOtaResponse(AsyncWebServerRequest* req);
    void _handleOtaUpload(AsyncWebServerRequest* req, const String& filename,
                          size_t index, uint8_t* data, size_t len, bool final);

    // OTA-Upload LittleFS-Image (HTTP)
    void _handleFsResponse(AsyncWebServerRequest* req);
    void _handleFsUpload(AsyncWebServerRequest* req, const String& filename,
                         size_t index, uint8_t* data, size_t len, bool final);
};
