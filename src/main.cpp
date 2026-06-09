// Bienenwaage3 – Hauptprogramm
// Hardware: WT32-ETH01-V1.4 (ESP32 + LAN8720 Ethernet)
// Autor: Johann Gerner
// Alle Initialisierungen in setup(), nur Koordination in loop()

#include <Arduino.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "hx711_multi.h"
#include "temperature.h"
#include "lcd_display.h"
#include "web_server.h"
#include "mqtt_client.h"
#include "eth_wifi_manager.h"
#include "storage.h"
#include "button_handler.h"

// ── Globale Objekte ────────────────────────────────────────────────────────────
Hx711Multi        hx711;
TemperatureSensor tempSensor;
LcdDisplay        lcd;
WebServerManager  webServer;
MqttManager       mqttClient;
EthWifiManager    network;
StorageManager    storage;
ButtonHandler     buttons;

// ── setup ──────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    esp_reset_reason_t reason = esp_reset_reason();
    Serial.printf("\n=== Bienenwaage3 v%s ===  (Reset: %d)\n", FW_VERSION, (int)reason);

    Wire.begin(LCD_SDA_PIN, LCD_SCL_PIN);
    storage.begin();
    lcd.begin();
    lcdPrint(0, "Bienenwaage3");
    lcdPrint(1, "Starte...");

    network.begin();
    Serial.printf("ETH MAC:    %s\n", ETH.macAddress().c_str());
    Serial.printf("WiFi AP MAC:%s\n", WiFi.softAPmacAddress().c_str());
    lcdPrint(1, "Netzwerk OK");

    hx711.begin();
    tempSensor.begin();
    buttons.begin();
    webServer.begin();
    mqttClient.begin();

    lcdPrint(0, "Bereit");
    lcdPrint(1, "");
    Serial.println("Setup abgeschlossen.");
}

// ── loop ───────────────────────────────────────────────────────────────────────
void loop() {
    ArduinoOTA.handle();
    network.loop();
    hx711.loop();
    tempSensor.loop();
    buttons.loop();
    mqttClient.loop();
    // AsyncWebServer ist event-driven – kein expliziter loop-Aufruf nötig
}
