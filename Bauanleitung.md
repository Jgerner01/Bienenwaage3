# Bienenwaage3 – Bauanleitung

**Hardware:** WT32-ETH01-V1.4 (ESP32 + LAN8720 Ethernet)  
**Firmware:** Bienenwaage3 v1.1.0

---

## 1. Stückliste

### Pflichtkomponenten

| Anzahl | Bezeichnung | Hinweis |
|--------|-------------|---------|
| 1 | WT32-ETH01-V1.4 | ESP32-Modul mit integriertem LAN8720 |
| 1 | LCD-Display 16×2, I2C (PCF8574) | I2C-Adresse 0x27 |
| 1 | DS18B20 Temperatursensor | wasserdichte Ausführung empfohlen |
| 2 | Taster (Schließer) | mit externem Pull-up-Widerstand |
| 2 | Widerstand 10 kΩ | Pull-down für Taster (GPIO 36 + 39) |
| 1 | Widerstand 4,7 kΩ | Pull-up für OneWire (DS18B20) |
| 1 | Netzteil 5 V / mind. 1 A | stabil, mit ausreichend Reserveleistung |
| 1 | RJ45-Buchse / Patchkabel | für Ethernet-Anschluss |

### Waagmodule (1–7 Stück)

| Anzahl | Bezeichnung | Hinweis |
|--------|-------------|---------|
| 1–7 | HX711 Wägezellen-Verstärker | ein Modul pro Wägezelle |
| 1–7 | Wägezelle (Load Cell) | passend zum jeweiligen Gewichtsbereich |

> **Hinweis:** Es können 0 bis 7 HX711-Module betrieben werden. Nicht bestückte Module werden in der Software einfach als inaktiv markiert.

### Empfohlenes Zubehör

- Hutschienen-Gehäuse oder Aufputzgehäuse (IP54 oder besser für Außenbereich)
- Kabeldurchführungen / Kabelverschraubungen
- Schraubklemmen-Platine zur einfachen Verdrahtung
- USB-UART-Adapter (CH340, CP2102 o. ä.) für die Erstprogrammierung

---

## 2. Pinbelegung

### Pflichtanschlüsse

| GPIO | Funktion | Beschreibung |
|------|----------|--------------|
| 33 | I2C SDA | LCD-Display |
| 32 | I2C SCL | LCD-Display |
| 4 | OneWire | DS18B20 Temperatursensor |
| 36 | Taster 1 | Modul wählen (Input-Only!) |
| 39 | Taster 2 | Funktion (Input-Only!) |

> **Wichtig:** GPIO 36 und 39 sind Input-Only-Pins ohne internen Pull-up.  
> Externe 10-kΩ-Widerstände nach 3,3 V sind **zwingend erforderlich**.

### HX711-Module

| GPIO | Funktion |
|------|----------|
| 5 | HX711 SCK (gemeinsamer Takt, **alle** Module) |
| 14 | HX711 DOUT Modul 1 |
| 13 | HX711 DOUT Modul 2 |
| 17 | HX711 DOUT Modul 3 |
| 15 | HX711 DOUT Modul 4 |
| 2 | HX711 DOUT Modul 5 |
| 34 | HX711 DOUT Modul 6 (Input-Only) |
| 35 | HX711 DOUT Modul 7 (Input-Only) |

> **GPIO 12 ist reserviert und darf nicht belegt werden** (beeinflusst Boot-Modus des ESP32).

### Spannungsversorgung

| Pin WT32-ETH01 | Anschluss |
|----------------|-----------|
| 5V | 5V Netzteil + |
| GND | Netzteil – |

Die 3,3-V-Logik wird intern vom Modul erzeugt. HX711-Module können wahlweise mit 3,3 V oder 5 V betrieben werden (AVDD-Anschluss); bei 5 V ist der analoge Referenzbereich größer.

---

## 3. Schaltungsaufbau

### Taster-Beschaltung (gleich für beide Taster)

Da die Software `HIGH` als gedrückt erkennt, ist eine **Pull-down**-Beschaltung erforderlich:

```
3,3 V ──┬── 10 kΩ ──┬── GPIO (36 oder 39)
        │            │
       GND         Taster
                    │
                   GND
```

Der Taster zieht den GPIO-Pin bei Betätigung auf GND. Die Software erkennt HIGH = nicht gedrückt, LOW = gedrückt.

> **Achtung:** Im Quellcode ist `digitalRead() == HIGH` als „gedrückt" definiert.  
> Das bedeutet: Taster als **Öffner** mit Pull-up ODER Taster als **Schließer** von GPIO nach 3,3 V  
> (ohne Pull-up, dann 3,3 V direkt am GPIO wenn gedrückt).  
> Die einfachere Variante ist: **Schließer von GPIO nach 3,3 V**, kein Pull-up nötig.

### DS18B20 Anschluss

```
DS18B20 VDD ── 3,3 V
DS18B20 GND ── GND
DS18B20 DQ  ──┬── GPIO 4
              └── 4,7 kΩ ── 3,3 V
```

### LCD I2C Anschluss

```
LCD VCC ── 5 V  (oder 3,3 V je nach Modul)
LCD GND ── GND
LCD SDA ── GPIO 33
LCD SCL ── GPIO 32
```

Interne Pull-ups des ESP32 I2C-Busses werden von der Wire-Bibliothek aktiviert. Bei langen Leitungen (> 30 cm) empfehlen sich externe 4,7-kΩ-Pull-ups nach 3,3 V.

### HX711 Anschluss

```
HX711 VCC  ── 3,3 V (oder 5 V)
HX711 GND  ── GND
HX711 SCK  ── GPIO 5   (alle Module gemeinsam)
HX711 DOUT ── GPIO xx  (modulspezifisch, siehe Tabelle)
```

Wägezelle → HX711:
```
Rote Ader   ── E+   (Erregung +)
Schwarze    ── E–   (Erregung –)
Weiße       ── A–   (Signal –)
Grüne       ── A+   (Signal +)
```

> Die Farbcodierung kann je nach Hersteller abweichen. Datenblatt der Wägezelle beachten.

---

## 4. Erstprogrammierung (USB/UART)

Die Erstprogrammierung des WT32-ETH01 erfolgt **einmalig** über einen USB-UART-Adapter. Danach sind alle weiteren Updates per OTA (Over-the-Air) möglich.

### Benötigte Software

- [PlatformIO](https://platformio.org/) (VS Code Extension oder CLI)
- USB-UART-Treiber (CH340/CP2102 je nach Adapter)

### Verdrahtung USB-UART-Adapter ↔ WT32-ETH01

| USB-UART | WT32-ETH01 |
|----------|------------|
| TX | RX (GPIO3) |
| RX | TX (GPIO1) |
| GND | GND |
| 3,3 V | 3,3 V (nur wenn keine eigene Spannungsversorgung) |

**Boot-Modus für Flash:**
- GPIO 0 mit GND verbinden (vor dem Einschalten)
- Gerät einschalten / Reset drücken
- Flash-Vorgang starten
- Nach dem Flash: GPIO 0 von GND trennen, Reset

### Flash-Befehl

```bash
pio run -t upload
```

PlatformIO erkennt den seriellen Port automatisch. Falls nicht:

```bash
pio run -t upload --upload-port COM3    # Windows
pio run -t upload --upload-port /dev/ttyUSB0  # Linux
```

---

## 5. Erstinbetriebnahme

Nach dem ersten Flash:

1. GPIO 0 von GND trennen, Gerät neu starten
2. LCD zeigt kurz **„LAN"** / „verbinde..." und dann **„WiFi AP"** / `192.168.4.1`
3. Mit WLAN-Gerät (Smartphone/Laptop) mit dem Access-Point verbinden:
   - SSID: `Bienenwaage3-Setup`
   - Passwort: `12345678`
4. Browser öffnen: `http://192.168.4.1`
5. Im Reiter **WLAN/ETH** Ethernet-Einstellungen konfigurieren
6. Im Reiter **MQTT** den Broker eintragen (optional)
7. **Speichern & Neustart** – das Gerät verbindet sich nun per Ethernet

---

## 6. Kalibrierung der Wägezellen

Siehe Bedienungsanleitung, Abschnitt 5.

---

## 7. Wichtige Hinweise

- **SCK niemals länger als 60 µs HIGH** lassen – der HX711 schaltet in den Power-Down-Modus
- **GPIO 12** frei lassen – beeinflusst den Flash-Voltage-Strapping des ESP32
- **GPIO 34/35** sind Input-Only, eignen sich nur für HX711-DOUT (kein Ausgang möglich)
- Das Ethernet-Interface des WT32-ETH01 benötigt beim Start ca. 10–15 Sekunden bis zur IP-Vergabe per DHCP
- Der WiFi-Access-Point ist **immer aktiv** (auch bei bestehender Ethernet-Verbindung) und dient als Konfigurationszugang
