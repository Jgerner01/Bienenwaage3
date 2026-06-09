# Bienenwaage3 – Projektplan

**Hardware:** WT32-ETH01-V1.4 (ESP32 + LAN8720 Ethernet)  
**Zweck:** Multi-Kanal-Stockwaage mit bis zu 7 HX711-Modulen, Temperaturkompensation,
Weboberfläche, MQTT/Home Assistant, I2C-LCD, Vor-Ort-Bedienung  

---

## 1. Hardware-Übersicht

| Komponente         | Typ / Bibliothek              | Anzahl |
|--------------------|-------------------------------|--------|
| Mikrocontroller    | WT32-ETH01 (ESP32 + LAN8720)  | 1      |
| Wäge-ADC           | HX711                         | 1–7    |
| Temperatursensor   | DS18B20 (OneWire, 12-Bit)     | 1      |
| Display            | I2C-LCD 16×2 (PCF8574, 0x27)  | 1      |
| Taster             | Drucktaster (aktiv HIGH)      | 2      |
| Netzwerk           | Ethernet (primär) + WiFi (AP) | –      |

---

## 2. Pinbelegung WT32-ETH01-V1.4

### Durch LAN8720 belegte Pins (NICHT verfügbar)
| GPIO | Funktion (Ethernet RMII) |
|------|--------------------------|
| 0    | EMAC REF CLK (50 MHz OSC)|
| 16   | ETH_PHY_POWER            |
| 18   | EMAC MDIO                |
| 19   | EMAC TXD0                |
| 21   | EMAC TX_EN               |
| 22   | EMAC TXD1                |
| 23   | EMAC MDC                 |
| 25   | EMAC RXD0                |
| 26   | EMAC RXD1                |
| 27   | EMAC CRS_DV              |

### Vorgeschlagene Pinbelegung (Projekt)

| GPIO | Richtung  | Funktion                     | Hinweis                                          |
|------|-----------|------------------------------|--------------------------------------------------|
| 33   | Bi        | I2C SDA (LCD)                | Wire.begin(33, 32)                               |
| 32   | Bi        | I2C SCL (LCD)                |                                                  |
| 4    | Bi        | DS18B20 OneWire              | 4,7 kΩ Pull-Up nach 3,3 V                       |
| 5    | Bi        | HX711 SCK (shared, alle 7)   | Gemeinsamer Taktausgang                          |
| 14   | Bi        | HX711 DOUT Modul 1           |                                                  |
| 13   | Bi        | HX711 DOUT Modul 2           |                                                  |
| 17   | Bi        | HX711 DOUT Modul 3           |                                                  |
| 15   | Bi        | HX711 DOUT Modul 4           | Teilt sich Pin mit Power-LED (unkritisch)        |
| 2    | Bi        | HX711 DOUT Modul 5           | Teilt sich Pin mit ETH-LED (unkritisch)          |
| 34   | Nur-Input | HX711 DOUT Modul 6           | Kein int. Pull-Up, HX711 treibt aktiv            |
| 35   | Nur-Input | HX711 DOUT Modul 7           | Kein int. Pull-Up, HX711 treibt aktiv            |
| 36   | Nur-Input | Taster 1 – Modul wählen      | Ext. 10 kΩ Pull-Up nach 3,3 V; aktiv HIGH       |
| 39   | Nur-Input | Taster 2 – Funktion          | Ext. 10 kΩ Pull-Up nach 3,3 V; aktiv HIGH       |
| 1    | Bi        | UART0 TX (Debug-Serial)      | USB-Serial, nicht für Peripherie nutzen          |
| 3    | Bi        | UART0 RX (Debug-Serial)      | USB-Serial, nicht für Peripherie nutzen          |

> **Boot-Hinweise:**
> - GPIO12 wird NICHT genutzt (Boot-Strapping: HIGH → 1,8 V Flash = fehlerhaft)
> - GPIO0 ist ETH-Takt und darf NICHT als GPIO verwendet werden
> - GPIO34/35/36/39 kein interner Pull-Up → externe Widerstände erforderlich

---

## 3. Software-Architektur

```
src/
  main.cpp              – setup(), loop(), globaler Zustandsautomat
  hx711_multi.cpp/.h    – Synchrones Multi-Kanal HX711 Management
  filter.cpp/.h         – Median-Filter, Outlier-Erkennung, Sigma-Berechnung
  temperature.cpp/.h    – DS18B20 nicht-blockierend (12-Bit, millis-Timer)
  lcd_display.cpp/.h    – lcdPrint(), Web-Buffer-Synchronisation
  web_server.cpp/.h     – AsyncWebServer, alle REST-Endpoints, HTML-Ausgabe
  mqtt_client.cpp/.h    – MQTT + Home Assistant Auto-Discovery
  eth_wifi_manager.cpp/.h – Ethernet primär, WiFi AP für Konfiguration
  storage.cpp/.h        – NVS (Preferences), JSON-Datei Import/Export
  button_handler.cpp/.h – Zustandsautomat Taster (Kurz-/Langdruck)
include/
  config.h              – ALLE Konfigurationswerte und Standardwerte
data/
  index.html            – Weboberfläche (LittleFS)
  style.css             – CSS für Tabs, LCD-Animation, Statusanzeigen
  app.js                – AJAX-Updates, Slider, Bestätigungsdialoge
```

---

## 4. Modulbeschreibungen

### 4.1 HX711 Multi-Manager (`hx711_multi`)

**Bibliothek:** `HX711_ADC` von Olav Kallhovd  
PlatformIO: `olkal/HX711_ADC @ ^1.2.9`

Alle 7 HX711-Module teilen sich **einen SCK-Pin (GPIO5)**.
Die DOUT-Pins werden per `GPIO.in` / `GPIO.in1.val` **atomar im selben Takt** gelesen.

#### Messwerte pro Modul

```
Rohwerte-Ring-Buffer (100 Werte)
  → Outlier-Filter: Sprünge > 100 g/s werden verworfen
  → Sortierung → untere 20 + obere 20 Werte verwerfen
  → Median der verbleibenden 60 Werte = Hauptmesswert
  → Temperaturkompensation: w_korr = w + a2·T² + a1·T + a0
  → Sigma-Berechnung aus den 60 Werten (Standardabweichung)
```

#### Schnellmessung (pro Modul, parallel zur Hauptmessung)
```
Eigener Ring-Buffer (20 Werte)
  → Outlier-Filter (gleiche 100 g/s Grenze)
  → Gleitender Median (20 Werte)
```

#### Tara-Varianten
| Tara-Typ          | Median-Tiefe | Speicher  | Auslöser                  |
|-------------------|--------------|-----------|---------------------------|
| Grundtara         | 50 Werte     | NVS       | Web-Button / Taster       |
| Ertragstara       | 50 Werte     | NVS       | Langer Tastendruck (5 s)  |
| Schnellmess-Tara  | 10 Werte     | RAM       | Web-Button                |

#### Konfigurierbare Parameter pro Modul (über Web)
- Tara-Offset (Grundtara, Ertragstara)
- Kalibrierfaktor (g pro ADC-Einheit)
- Temperaturdrift-Polynom: a2, a1, a0
- Medianfenster-Größe (Hauptmessung, Standard 100)
- Outlier-Schwelle [g/s] (Standard 100)
- Aktiv/Inaktiv (Modul deaktivieren)

### 4.2 Filter (`filter`)

```cpp
class MedianFilter {
  // Ring-Buffer, konfigurierbare Fenstergröße
  // Sortierung und Trim (Outlier-Verwerfung: lower%, upper%)
  float getMedian(int trimLow, int trimHigh);
  float getSigma();  // Standardabweichung der Trim-Werte
};

class OutlierFilter {
  // Verwirft Messwert wenn |delta_g / delta_t_s| > threshold_g_per_s
  bool isValid(float newValue, float lastValue, float dt_s);
};
```

### 4.3 Temperatursensor (`temperature`)

- DS18B20, 12-Bit-Auflösung (0,0625 °C)
- Nicht-blockierend: `requestTemperatures()` → 800 ms später lesen
- Mehrere Sensoren werden erkannt und nummeriert
- Fehlerbehandlung: −127 °C / 85 °C → Fehlercode `TEMP_ERROR`

### 4.4 LCD Display (`lcd_display`)

- I2C-LCD 16×2, Adresse 0x27 (in `config.h` änderbar)
- Zentrale Funktion: `lcdPrint(row, text)` → Hardware + Web-Buffer synchron
- Kein direktes `lcd.print()` im restlichen Code
- Web-Buffer: `String lcd_line[2]` wird per `/data`-JSON mitgeliefert

### 4.5 Ethernet/WiFi Manager (`eth_wifi_manager`)

**Primär:** Ethernet (LAN8720, automatisch beim Boot)  
**Sekundär:** WiFi Access Point (für Erstkonfiguration und Fallback)

```
Zustände:
  ETH_CONNECTED       – Ethernet verbunden, Normalbetrieb
  ETH_CONNECTING      – Verbindungsaufbau (max. 10 s)
  ETH_RECONNECTING    – Alle 2 min Wiederverbindungsversuch
    → nach 20 min → AP_CONFIG_MODE
  AP_CONFIG_MODE      – ESP öffnet WiFi-AP (SSID: "Bienenwaage3-Setup")
    → Weboberfläche für Ethernet/MQTT-Konfiguration
    → nach 10 min ohne Eingabe → RESET und Neustart
  WIFI_CONNECTED      – Fallback WiFi aktiv (falls konfiguriert)
```

Alle Netzwerkparameter in NVS gespeichert.

### 4.6 Webserver (`web_server`)

**Bibliothek:** `ESP Async WebServer`

#### REST-Endpoints

| Methode | Pfad          | Funktion                                    |
|---------|---------------|---------------------------------------------|
| GET     | `/`           | Weboberfläche (aus LittleFS)                |
| GET     | `/data`       | JSON: alle Messwerte, LCD-Inhalt, Status    |
| POST    | `/set`        | Parameter setzen (JSON-Body)                |
| POST    | `/tare`       | Tara auslösen `{module: N, type: "main"}`  |
| POST    | `/calibrate`  | Kalibrierung `{module: N, known_weight: X}` |
| GET     | `/export`     | Alle Parameter als JSON-Datei download      |
| POST    | `/import`     | Parameter aus JSON-Datei importieren        |
| POST    | `/reset`      | Werkseinstellungen (NVS löschen + Neustart) |
| GET     | `/status`     | Netzwerk, MQTT, Modulstatus als JSON        |

#### `/data` JSON-Struktur
```json
{
  "modules": [
    {
      "id": 0,
      "active": true,
      "weight_g": 25431.2,
      "weight_quick_g": 25428.1,
      "sigma_g": 1.4,
      "temp_compensation_g": -12.3,
      "tare_main_g": 20000.0,
      "tare_yield_g": 0.0,
      "online": true
    }
  ],
  "temperature": { "value_c": 18.75, "online": true },
  "lcd": { "line1": "Modul 1: 25.43kg", "line2": "T: 18.75C  s=1.4g" },
  "network": { "eth_ip": "192.168.1.50", "mqtt_connected": true },
  "uptime_s": 3600
}
```

### 4.7 MQTT / Home Assistant (`mqtt_client`)

#### Topics (Beispiel für Modul 1)
```
bienenwaage3/modul1/gewicht        → 25431.2  [g]
bienenwaage3/modul1/gewicht_schnell → 25428.1 [g]
bienenwaage3/modul1/sigma          → 1.4      [g]
bienenwaage3/modul1/online         → true
bienenwaage3/temperatur            → 18.75    [°C]
bienenwaage3/status                → JSON
```

#### Home Assistant Auto-Discovery
```
homeassistant/sensor/bienenwaage3_m1_gewicht/config → {...}
homeassistant/sensor/bienenwaage3_temperatur/config → {...}
```

### 4.8 Speicherung (`storage`)

**NVS (Preferences)** für alle persistenten Parameter:
- Kalibrierung, Tara-Werte, Polynomkoeffizienten pro Modul
- Netzwerkeinstellungen (Ethernet-IP, MQTT-Broker)
- Medianfenster-Größen, Outlier-Schwellen

**JSON-Datei Import/Export:**
- `GET /export` → lädt `bienenwaage3_params.json`
- `POST /import` → liest JSON, validiert, speichert in NVS

### 4.9 Taster-Handler (`button_handler`)

#### Taster 1 – Modul wählen (GPIO36)
- Jeder Kurzdruck (< 1 s): nächstes aktives Modul auswählen
- Ausgewähltes Modul wird auf LCD angezeigt

#### Taster 2 – Funktion (GPIO39)
```
Kurzdruck (< 1 s):    Schnellmessung starten/anzeigen
Langdruck (≥ 5 s):    Ertragstarierung starten
  → LCD zeigt "Ertragstara?" + 5 s Bestätigungszeit
  → Taster 2 nochmals drücken = Bestätigung → Tara ausführen
  → Keine Bestätigung nach 5 s = Abbruch
```

---

## 5. Weboberfläche – Tab-Struktur

| Tab          | Inhalt                                                               |
|--------------|----------------------------------------------------------------------|
| **Status**   | Live-Anzeige aller Module (Gewicht, Sigma, Schnellwert), Temp, LCD-Animation, Verbindungsstatus-Animationen |
| **Waagen**   | Pro-Modul-Konfiguration: Kalibrierung, Tara-Buttons, Polynomkoeff.  |
| **WLAN/ETH** | Ethernet-IP (statisch/DHCP), WiFi SSID/PW für AP-Modus              |
| **MQTT**     | Broker, Port, User, PW, Topic-Präfix, HA Auto-Discovery Ein/Aus      |
| **LCD**      | 16×2-LCD-Animation (HTML/CSS), Inhalt spiegelt Hardware              |
| **Parameter**| Alle Einstellungen, Import/Export-Buttons, Werkseinstellungen-Button |

### Status-Tab: Verbindungsanimationen
```
[●] Modul 1 – 25.431 kg  σ=1.4g    ← grüner Kreis = online
[●] Modul 2 – 12.850 kg  σ=0.9g
[○] Modul 3 – OFFLINE               ← grauer Kreis = offline
...
[●] Temperatur – 18.75 °C           ← grüner Kreis = online
[○] Temperatur – OFFLINE            ← grauer Kreis = offline
```

### AJAX-Update-Strategie (kein Flackern)
- `setInterval(updateData, 2000)` ruft `GET /data` auf
- JavaScript aktualisiert nur die Texte in bestehenden DOM-Elementen
- Eingabefelder werden NICHT überschrieben, wenn sie fokussiert sind
  (`document.activeElement !== field`)
- LCD-Animation: nur `textContent` der Zeilen-`<span>`-Elemente ändern

---

## 6. Bibliotheken (platformio.ini)

```ini
lib_deps =
    olkal/HX711_ADC @ ^1.2.9         ; HX711 Mehrkanal, shared SCK
    esphome/AsyncTCP-esphome @ ^2.0.0 ; Async TCP für ESP32
    mathieucarbou/ESPAsyncWebServer @ ^3.1.0
    knolleary/PubSubClient @ ^2.8.0   ; MQTT
    milesburton/DallasTemperature @ ^3.11.0
    paulstoffregen/OneWire @ ^2.3.8
    marcoschwartz/LiquidCrystal_I2C @ ^1.1.4
    bblanchon/ArduinoJson @ ^7.0.0
    ; ESP32 ETH integriert in Arduino-ESP32 Core (kein extra lib)
```

---

## 7. Zustandsautomaten

### 7.1 Ethernet/WiFi Zustandsautomat

```
┌─────────────────────────────────────────────────────────────┐
│  BOOT                                                        │
│   → ETH_CONNECTING (max 10 s)                               │
│     ✓ → ETH_CONNECTED (Normalbetrieb)                       │
│     ✗ → ETH_RECONNECTING                                    │
│                                                              │
│  ETH_RECONNECTING:                                           │
│   → alle 2 min: ETH_CONNECTING versuchen                    │
│   → nach 20 min ohne Erfolg: AP_CONFIG_MODE                 │
│                                                              │
│  AP_CONFIG_MODE:                                             │
│   → WiFi-AP "Bienenwaage3-Setup" öffnen                     │
│   → Weboberfläche für neue Konfiguration verfügbar          │
│   → Eingabe empfangen: NVS speichern → RESET → BOOT         │
│   → 10 min Timeout ohne Eingabe: RESET → BOOT               │
└─────────────────────────────────────────────────────────────┘
```

### 7.2 Taster 2 Zustandsautomat

```
IDLE
  → Taster gedrückt (↑): Zeitstempel merken
  → Taster losgelassen (↓):
      < 1 s:  SCHNELLMESSUNG auslösen
      ≥ 5 s:  CONFIRM_YIELD_TARE (5 s Bestätigungsfenster)
        → Taster 2 nochmals kurz: Ertragstara ausführen
        → 5 s abgelaufen ohne Bestätigung: ABBRUCH
```

### 7.3 HX711 Mess-Zyklus (non-blocking)

```
IDLE
  → DOUT eines Moduls geht LOW (Daten bereit)
    → alle Module abfragen ob bereit
    → SCK-Burst: 24 Takte → alle DOUT gleichzeitig lesen
    → Werte in Ring-Buffer eintragen (mit Outlier-Check)
    → Buffer voll: Median + Sigma berechnen
    → Ruhezustand
```

---

## 8. HX711 Synchrones Lesen – Implementierungsdetail

Da alle HX711-Module denselben SCK-Pin teilen, werden alle DOUT-Pins
**simultan im selben Takt** gelesen. ESP32 erlaubt atomares GPIO-Lesen:

```cpp
// Für GPIO 0–31:
uint32_t bits_low = GPIO.in;
// Für GPIO 32–39:
uint32_t bits_high = GPIO.in1.val;

// Beispiel: Bit aus einem bestimmten GPIO extrahieren
bool dout_modul1 = (bits_low >> 14) & 1;  // GPIO14
bool dout_modul6 = (bits_high >> 2)  & 1; // GPIO34 (34-32=2)
```

Der SCK-Burst wird per direkter Register-Manipulation durchgeführt:
```cpp
GPIO.out_w1ts = (1 << SCK_PIN);  // SCK HIGH (GPIO.out_w1ts setzt einzelne Bits)
// kurze Pause (ESP32: ca. 50 ns ausreichend für HX711 bei 10 MHz SPI)
GPIO.out_w1tc = (1 << SCK_PIN);  // SCK LOW
```

> **WICHTIG:** SCK darf maximal 60 µs HIGH bleiben (HX711 Power-Down-Trigger).
> Bei 24 + 1 Pulsen und ~100 ns pro Puls: Gesamtzeit << 60 µs → unkritisch.

---

## 9. Dateistruktur

```
Bienenwaage3/
  Plan.md                    ← dieser Plan
  platformio.ini
  CLAUDE.md                  ← projektspezifische Regeln
  include/
    config.h                 ← alle Pins, Standardwerte, Flags
  src/
    main.cpp
    hx711_multi.cpp / .h
    filter.cpp / .h
    temperature.cpp / .h
    lcd_display.cpp / .h
    web_server.cpp / .h
    mqtt_client.cpp / .h
    eth_wifi_manager.cpp / .h
    storage.cpp / .h
    button_handler.cpp / .h
  data/
    index.html
    style.css
    app.js
```

---

## 10. Offene Punkte / Hinweise

| # | Thema | Hinweis |
|---|-------|---------|
| 1 | GPIO2/15 teilen sich mit LEDs | LEDs flackern während HX711-Kommunikation – unkritisch |
| 2 | GPIO36/39 kein int. Pull-Up | Externe 10 kΩ nach 3,3 V ZWINGEND erforderlich |
| 3 | GPIO12 nicht belegt | Reserviert, kein Einsatz wegen Boot-Strapping-Konflikt |
| 4 | HX711_ADC Sync | Bibliothek verwendet eigene Takt-Logik; für shared-SCK wird eine Wrapper-Klasse `Hx711Multi` implementiert |
| 5 | LittleFS | `data/`-Ordner wird mit `pio run --target uploadfs` hochgeladen |
| 6 | MQTT Reconnect | PubSubClient benötigt eigene Reconnect-Logik im loop() |
| 7 | WT32-ETH01 Arduino-Core | `board = wt32-eth01` in platformio.ini korrekt setzen |
| 8 | Kalibrierung | Bekanntes Gewicht über Weboberfläche eingeben → Faktor berechnen |
| 9 | Temperaturkoeffizienten | Werden pro Modul separat in NVS gespeichert |
| 10 | JSON Import/Export | Format-Version in JSON speichern für zukünftige Kompatibilität |
