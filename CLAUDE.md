# Bienenwaage3 – Projektregeln

## Hardware
- **Board:** WT32-ETH01-V1.4 (ESP32 + LAN8720 Ethernet)
- **PlatformIO:** `board = wt32-eth01`
- Netzwerk: Ethernet primär, WiFi nur als AP-Konfigurationsmodus

## Pinbelegung (NICHT ändern ohne Plan.md zu aktualisieren)
| GPIO | Funktion                    |
|------|-----------------------------|
| 33   | I2C SDA (LCD)               |
| 32   | I2C SCL (LCD)               |
| 4    | DS18B20 OneWire             |
| 5    | HX711 SCK (shared)          |
| 14   | HX711 DOUT Modul 0          |
| 13   | HX711 DOUT Modul 1          |
| 17   | HX711 DOUT Modul 2          |
| 15   | HX711 DOUT Modul 3          |
| 2    | HX711 DOUT Modul 4          |
| 34   | HX711 DOUT Modul 5 (IN)     |
| 35   | HX711 DOUT Modul 6 (IN)     |
| 36   | Taster 1 – Modul wählen     |
| 39   | Taster 2 – Funktion         |
| 12   | FREI – NICHT BELEGEN!       |

## Wichtige Regeln
- `lcdPrint(row, text)` immer statt `lcd.print()` – synchronisiert Web-Buffer
- Alle HX711 DOUT-Pins werden per `GPIO.in` / `GPIO.in1.val` atomar gelesen
- SCK niemals > 60 µs HIGH halten (HX711 Power-Down!)
- DS18B20: immer 12-Bit-Auflösung, immer nicht-blockierend mit millis()-Timer
- NVS (Preferences) für alle persistenten Werte, kein EEPROM
- Weboberfläche in `data/` (LittleFS), nicht als String im Sketch

## Bibliotheken
Siehe `platformio.ini` – nicht ohne Prüfung gegen `Plan.md` ändern.

## Architektur
Alle Module als eigene .cpp/.h-Klassen, `main.cpp` nur setup/loop/Koordination.
Vollständige Architekturbeschreibung: siehe `Plan.md`.
