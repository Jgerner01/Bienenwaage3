// Bienenwaage3 – Konfiguration
// Hardware: WT32-ETH01-V1.4 (ESP32 + LAN8720)
// Autor: Johann Gerner
// Alle Hardwareparameter und Standardwerte – KEIN Hardcoding im Sketch

#pragma once
#include <stdint.h>

// ── Versionierung ──────────────────────────────────────────────────────────────
#define FW_VERSION        "1.1.0"
#define PARAM_FORMAT_VER  1          // JSON-Exportformat-Version

// ── HX711 – Pins ──────────────────────────────────────────────────────────────
#define HX711_SCK_PIN     5          // Gemeinsamer Taktausgang (alle Module)
#define HX711_MAX_MODULES 7

// DOUT-Pins der einzelnen Module (Index 0–6)
constexpr uint8_t HX711_DOUT_PINS[HX711_MAX_MODULES] = {
    14,   // Modul 0
    13,   // Modul 1
    17,   // Modul 2
    15,   // Modul 3
    2,    // Modul 4  (teilt GPIO2 mit ETH-LED)
    34,   // Modul 5  (Input only)
    35    // Modul 6  (Input only)
};

// ── HX711 – Filter-Standardwerte ──────────────────────────────────────────────
#define HX711_MAIN_BUFFER_SIZE   100  // Hauptmessung: Ring-Buffer Größe
#define HX711_MAIN_TRIM_LOW       20  // Untere Werte verwerfen
#define HX711_MAIN_TRIM_HIGH      20  // Obere Werte verwerfen
#define HX711_QUICK_BUFFER_SIZE   20  // Schnellmessung: Ring-Buffer
#define HX711_TARE_MAIN_SAMPLES   50  // Grundtara und Ertragstara: Median
#define HX711_TARE_QUICK_SAMPLES  10  // Schnellmess-Tara: Median
#define HX711_OUTLIER_THRESHOLD  100.0f  // Max. Sprung [g/s]

// ── Temperatur – DS18B20 ───────────────────────────────────────────────────────
#define ONEWIRE_PIN       4
#define DS18B20_RESOLUTION 12        // 12-Bit: 0,0625 °C, ~750 ms Wandlung
#define DS18B20_CONVERT_DELAY_MS 800 // Sicherheitsabstand zur Wandlungszeit
#define TEMP_ERROR_VALUE  -127.0f    // Sensorfehler-Kennwert

// ── I2C – LCD 16×2 ─────────────────────────────────────────────────────────────
#define LCD_SDA_PIN       33
#define LCD_SCL_PIN       32
#define LCD_I2C_ADDRESS   0x27
#define LCD_COLS          16
#define LCD_ROWS          2

// ── Taster ────────────────────────────────────────────────────────────────────
#define BTN_SELECT_PIN    36   // Taster 1: Modul wählen (Input only, ext. PU)
#define BTN_FUNC_PIN      39   // Taster 2: Funktion     (Input only, ext. PU)
#define BTN_DEBOUNCE_MS   50
#define BTN_LONG_PRESS_MS 5000   // Langdruck für Ertragstara
#define BTN_CONFIRM_MS    5000   // Bestätigungsfenster nach Langdruck

// ── Netzwerk – Ethernet (LAN8720) ─────────────────────────────────────────────
#define ETH_PHY_ADDR      1
#define ETH_PHY_MDC       23
#define ETH_PHY_MDIO      18
#define ETH_PHY_POWER     16
// ETH_CLOCK_GPIO0_IN wird in eth_wifi_manager.cpp gesetzt

// ── Netzwerk – WiFi Access Point (Fallback/Konfiguration) ────────────────────
#define WIFI_AP_SSID      "Bienenwaage3-Setup"
#define WIFI_AP_PASSWORD  "12345678"
#define WIFI_AP_IP        "192.168.4.1"

// ── Netzwerk – Reconnect-Timing ───────────────────────────────────────────────
#define ETH_CONNECT_TIMEOUT_MS    10000UL   // 10 s Verbindungsversuch
#define ETH_RECONNECT_INTERVAL_MS 120000UL  // 2 min zwischen Versuchen
#define ETH_MAX_RECONNECT_MS      1200000UL // 20 min bis AP-Modus
#define AP_CONFIG_TIMEOUT_MS      600000UL  // 10 min AP-Wartezeit

// ── MQTT ──────────────────────────────────────────────────────────────────────
#define MQTT_DEFAULT_PORT       1883
#define MQTT_KEEPALIVE_S        60
#define MQTT_RECONNECT_INTERVAL 5000UL
#define MQTT_TOPIC_PREFIX       "bienenwaage3"
#define MQTT_CLIENT_ID          "bienenwaage3"
#define HA_DISCOVERY_PREFIX     "homeassistant"

// ── Webserver ─────────────────────────────────────────────────────────────────
#define WEB_UPDATE_INTERVAL_MS  2000    // Standard AJAX-Intervall
#define WEB_SERVER_PORT         80

// ── OTA-Update ────────────────────────────────────────────────────────────────
#define OTA_HOSTNAME        "bienenwaage3"
#define OTA_PASSWORD        "bienenwaage3ota"   // In NVS überschreibbar
#define OTA_PORT            3232
// HTTP-OTA Endpoint: POST /update  (Browser-Upload, kein PlatformIO nötig)
// Fortschritt wird auf LCD Zeile 2 und per WebSocket angezeigt

// ── NVS Namespace ─────────────────────────────────────────────────────────────
#define NVS_NAMESPACE_NET       "bw3_net"
#define NVS_NAMESPACE_MODULES   "bw3_mod"
#define NVS_NAMESPACE_PARAMS    "bw3_prm"

// ── Standardwerte Temperaturkompensation ──────────────────────────────────────
// w_korr = w + a2*T^2 + a1*T + a0  (Koeffizienten in g/°C^n)
#define TEMP_POLY_A2_DEFAULT   0.0f
#define TEMP_POLY_A1_DEFAULT  -10.0f   // ca. -10 g/°C als Startwert
#define TEMP_POLY_A0_DEFAULT   0.0f
