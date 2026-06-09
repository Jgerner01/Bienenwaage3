# Bienenwaage3 – Bedienungsanleitung

**Firmware:** Bienenwaage3 v1.1.0  
**Weboberfläche:** Ethernet (primär) oder WiFi AP `192.168.4.1`

---

## 1. Übersicht

Die Bienenwaage3 misst das Gewicht von bis zu **7 Bienenvölkern** gleichzeitig und erfasst zusätzlich die **Außentemperatur**. Alle Daten werden über eine Weboberfläche angezeigt und können per **MQTT** an ein Smart-Home-System (z. B. Home Assistant) übertragen werden.

**Funktionen:**
- Gewichtsmessung mit Temperaturkompensation (Polynom 2. Grades)
- Grund- und Ertragstara pro Modul
- Schnellmessung (reduzierter Puffer für schnelle Ablesung)
- MQTT-Publish mit Home Assistant Auto-Discovery
- Firmware-Update per Browser (OTA)
- Parameter-Import und -Export als JSON

---

## 2. Startverhalten und LCD-Anzeige

Nach dem Einschalten durchläuft das Gerät folgende Sequenz:

| Phase | Dauer | LCD Zeile 1 | LCD Zeile 2 |
|-------|-------|-------------|-------------|
| Start | 5 s | `LAN` | ETH-IP-Adresse (sobald verfügbar) oder `verbinde...` |
| WLAN-Info | 5 s | `WiFi AP` | `192.168.4.1` |
| Betrieb | dauerhaft | `Modul 1` | *(leer)* |

Nach 10 Sekunden wechselt die Anzeige in den Modulwahl-Modus. Das aktuelle Modul wird in Zeile 1 angezeigt (`Modul 1` bis `Modul 7`).

### LCD-Anzeige im Betrieb

| Zustand | LCD Zeile 1 | LCD Zeile 2 |
|---------|-------------|-------------|
| Modulwahl | `Modul X` | *(leer)* |
| Schnellmessung läuft | `Modul X` | `Schnellmessung..` |
| Ertragstara-Abfrage | `Ertragstara?` | `Nochmal drueck.` |
| Ertragstara aktiv | `Ertragstara...` | *(leer)* |
| Bestätigung abgelaufen | `Abgebrochen` | *(leer)* |
| OTA-Upload | `OTA Upload...` | Fortschritt in KB |
| OTA fertig | `OTA OK` | `Neustart...` |

Die LCD-Anzeige wird auch auf der Weboberfläche im Reiter **LCD** live gespiegelt.

---

## 3. Taster

Das Gerät hat zwei physische Taster. Beide Funktionen stehen auch auf der Weboberfläche im Reiter **LCD** zur Verfügung.

### Taster 1 – Modul wählen (GPIO 36)

| Aktion | Ergebnis |
|--------|----------|
| Kurzdruck | Nächstes Modul auswählen (1 → 2 → … → 7 → 1) |

Das gewählte Modul ist der Bezugspunkt für **Taster 2**.

### Taster 2 – Funktion (GPIO 39)

| Aktion | Ergebnis |
|--------|----------|
| Kurzdruck (< 5 s) | Schnellmessung-Tara für das aktuelle Modul starten |
| Langdruck (≥ 5 s) | Ertragstara-Bestätigung anfordern |
| Nochmaliger Druck innerhalb 5 s | Ertragstara wird gesetzt und gespeichert |
| Kein Druck innerhalb 5 s | Abbruch, keine Änderung |

> **Ertragstara** ist persistent – sie bleibt auch nach einem Neustart erhalten und zeigt das Gewicht relativ zum Zeitpunkt der Tara-Setzung an.

---

## 4. Weboberfläche

Die Weboberfläche ist erreichbar unter:
- `http://<ETH-IP>` – wenn das Gerät per Ethernet verbunden ist
- `http://192.168.4.1` – immer über den integrierten WiFi-Access-Point

### Reiter Status

Zeigt den Betriebszustand aller aktiven Module in Echtzeit (Aktualisierung alle 2 Sekunden):

| Anzeige | Beschreibung |
|---------|--------------|
| ● grün / grau | Modul online / offline |
| `X.XXX kg` | Hauptgewicht (Median, temperaturkompensiert) |
| `σ=X.Xg` | Standardabweichung der Hauptmessung |
| `~X.XXX kg` | Schnellmessung (kleiner Puffer, reaktionsschneller) |
| Temperatur | DS18B20-Messwert in °C |
| ETH IP | Aktuelle Ethernet-IP-Adresse |
| MQTT ● | Verbindungsstatus zum MQTT-Broker |

### Reiter Waagen

Konfiguration und Kalibrierung pro Modul (1–7):

| Feld | Beschreibung |
|------|--------------|
| Aktiv | Modul aktivieren/deaktivieren |
| Kalibrierfaktor | Gramm pro ADC-Einheit (wird durch Kalibrierung automatisch gesetzt) |
| Bekanntes Gewicht | Referenzgewicht für die automatische Kalibrierung |
| Grundtara | Tara setzen (leere Beute, Waagbrett etc.) |
| Ertragstara | Tara für Ertragserfassung setzen |
| Ertragstara löschen | Ertragstara zurücksetzen |
| Poly a2 / a1 / a0 | Koeffizienten der Temperaturkompensation |

### Reiter WLAN/ETH

| Feld | Beschreibung |
|------|--------------|
| DHCP | Automatische IP-Adresse beziehen |
| Statische IP / Gateway / Subnetz | Feste IP-Konfiguration (nur wenn DHCP deaktiviert) |
| Access-Point SSID | Name des WiFi-Konfigurationsnetzes |
| Access-Point Passwort | Passwort (min. 8 Zeichen) |

Änderungen werden nach **Speichern & Neustart** aktiv.

### Reiter MQTT

| Feld | Beschreibung |
|------|--------------|
| Broker-IP | IP-Adresse des MQTT-Brokers |
| Port | Standard: 1883 |
| Benutzer / Passwort | Optional, je nach Broker-Konfiguration |
| Topic-Präfix | Basis-Topic, Standard: `bienenwaage3` |
| HA Auto-Discovery | Home Assistant MQTT Discovery aktivieren |
| Retain | Nachrichten mit Retain-Flag senden |

### Reiter LCD

- **LCD-Anzeige (live):** Spiegelung der aktuellen LCD-Anzeige im Browser
- **Taster 1 – Modul wählen:** Simuliert einen Kurzdruck auf Taster 1
- **Taster 2 – Funktion:** Simuliert einen Kurzdruck auf Taster 2 (Schnellmessung-Tara)

### Reiter Parameter

| Schaltfläche | Aktion |
|--------------|--------|
| Parameter exportieren | Alle Einstellungen als JSON-Datei herunterladen |
| Parameter importieren | JSON-Datei hochladen und alle Einstellungen wiederherstellen |
| Werkseinstellungen | **Alle** Parameter löschen und Gerät neu starten |

> **Achtung:** Werkseinstellungen löscht alle Kalibrierungen, Tara-Werte und Netzwerkeinstellungen unwiderruflich.

### Reiter Firmware

OTA-Firmware-Update direkt über den Browser:

1. PlatformIO-Projekt neu bauen: `pio run`
2. Erzeugte Datei: `.pio/build/wt32-eth01/firmware.bin`
3. Im Reiter Firmware: Datei auswählen → **Upload & Neustart**
4. Fortschrittsbalken zeigt den Uploadfortschritt
5. Das Gerät startet nach erfolgreichem Upload automatisch neu

---

## 5. Kalibrierung der Wägezellen

Die Kalibrierung bestimmt den Umrechnungsfaktor vom ADC-Rohwert in Gramm.

### Voraussetzungen

- Modul muss im Reiter **Waagen** als **aktiv** markiert sein
- Wägezelle ist montiert und angeschlossen

### Ablauf

1. **Grundtara setzen** (Reiter Waagen → Grundtara):
   - Waagbrett / leere Beute ohne Bienen aufstellen
   - „Grundtara"-Schaltfläche drücken
   - Warten bis LCD „Modul X" zeigt (Tara-Messung dauert ca. 5 Sekunden)

2. **Kalibrierobjekt auflegen:**
   - Objekt mit **bekanntem Gewicht** auf die Waage legen
   - Empfehlung: mind. 5 kg, besser 10–20 kg für bessere Genauigkeit

3. **Bekanntes Gewicht eingeben** (Reiter Waagen → „Bekanntes Gewicht [g]"):
   - Gewicht in **Gramm** eingeben (z. B. `10000` für 10 kg)

4. **Kalibrieren** drücken:
   - Der neue Kalibrierfaktor wird berechnet und gespeichert
   - Die Gewichtsanzeige im Status-Tab zeigt nun das korrekte Gewicht

5. **Ergebnis prüfen:**
   - Kalibrierobjekt wegnehmen → Anzeige sollte nahe 0 kg liegen
   - Kalibrierobjekt wieder auflegen → Anzeige sollte dem bekannten Gewicht entsprechen

### Temperaturkompensation (optional)

Bei Messungen über einen größeren Temperaturbereich kann das Gewicht temperaturbeeinflusst sein. Die Kompensation erfolgt nach der Formel:

```
w_korr = w_roh + a2 × T² + a1 × T + a0
```

- **a1** (g/°C): Linearer Temperatureinfluss, typisch ca. −10 g/°C (Standardwert)
- **a2** (g/°C²): Quadratischer Einfluss, meist 0
- **a0** (g): Konstanter Offset, meist 0

Die Koeffizienten werden durch Messungen bei verschiedenen Temperaturen ermittelt und im Reiter **Waagen** pro Modul eingetragen.

---

## 6. MQTT-Datenformat

### Topic-Struktur

```
<prefix>/<hostname>/modul_<N>/gewicht_kg    → Hauptgewicht in kg
<prefix>/<hostname>/modul_<N>/gewicht_schnell_kg → Schnellmessung in kg
<prefix>/<hostname>/modul_<N>/sigma_g       → Standardabweichung in g
<prefix>/<hostname>/modul_<N>/tara_kg       → Grundtara in kg
<prefix>/<hostname>/modul_<N>/ertrag_kg     → Ertragstara-Bezugswert in kg
<prefix>/<hostname>/temperatur_c            → Temperatur in °C
<prefix>/<hostname>/status                  → Online-Status
```

Standard-Prefix: `bienenwaage3`, Hostname: `bienenwaage3`

### Home Assistant Auto-Discovery

Wenn **HA Auto-Discovery** aktiviert ist, registriert sich das Gerät beim Start automatisch bei Home Assistant. Alle Sensoren erscheinen ohne manuelle Konfiguration in der HA-Oberfläche.

---

## 7. Netzwerkzugang

### Ethernet (Primär)

Das Gerät verbindet sich beim Start automatisch per DHCP. Eine statische IP kann im Reiter **WLAN/ETH** konfiguriert werden.

Die Ethernet-IP wird beim Start 5 Sekunden lang auf dem LCD angezeigt.

### WiFi Access-Point (immer verfügbar)

| Parameter | Standardwert |
|-----------|--------------|
| SSID | `Bienenwaage3-Setup` |
| Passwort | `12345678` |
| IP-Adresse | `192.168.4.1` |

Der Access-Point ist **dauerhaft aktiv**, unabhängig vom Ethernet-Status. Er dient als Konfigurationszugang und als Fallback wenn kein Ethernet verfügbar ist.

### OTA via PlatformIO (Netzwerk)

Für Firmware-Updates ohne Browser kann PlatformIO direkt über das Netzwerk uploaden:

```bash
# Windows:
set OTA_IP=192.168.1.xxx
set OTA_PASS=bienenwaage3ota
pio run -e wt32-eth01-ota -t upload

# Linux/macOS:
export OTA_IP=192.168.1.xxx OTA_PASS=bienenwaage3ota
pio run -e wt32-eth01-ota -t upload
```

---

## 8. Fehlerbehebung

| Problem | Mögliche Ursache | Lösung |
|---------|-----------------|--------|
| LCD zeigt „verbinde..." dauerhaft | Kein Ethernet-Kabel / DHCP-Server nicht erreichbar | Kabel prüfen, Router-DHCP prüfen, ggf. statische IP setzen |
| Modul zeigt „OFFLINE" | HX711 nicht angeschlossen oder defekt | Verdrahtung prüfen, HX711 tauschen |
| Gewicht stark schwankend | Wägezelle vibrationsempfindlich oder Outlier-Schwelle zu hoch | Montagepunkt prüfen, Outlier-Threshold anpassen |
| Weboberfläche nicht erreichbar | Gerät hat keine IP | Über WiFi AP `192.168.4.1` verbinden |
| MQTT verbindet nicht | Falsche Broker-IP oder Zugangsdaten | Im Reiter MQTT prüfen, Broker-Log kontrollieren |
| Temperatur zeigt –127 °C | DS18B20 nicht angeschlossen oder defekter Sensor | Verdrahtung prüfen, Pull-up-Widerstand (4,7 kΩ) prüfen |
| OTA-Upload schlägt fehl | Falsches Passwort oder IP | `OTA_PASS` und `OTA_IP` prüfen |
| Nach Werkseinstellungen kein WLAN | SSID/Passwort zurückgesetzt | Mit AP `Bienenwaage3-Setup` / `12345678` verbinden und neu konfigurieren |

---

## 9. Technische Daten

| Parameter | Wert |
|-----------|------|
| Mikrocontroller | ESP32-D0WDQ6 (auf WT32-ETH01) |
| Ethernet | LAN8720 (10/100 Mbit/s) |
| WiFi | 802.11 b/g/n (AP-Modus) |
| Max. Wägezellen | 7 |
| Messrate HX711 | ~10 Hz (80 SPS-Modus) |
| Hauptfilter | Median über 100 Werte (konfigurierbar) |
| Schnellfilter | Median über 20 Werte |
| Temperaturauflösung | 0,0625 °C (12 Bit) |
| Versorgungsspannung | 5 V DC |
| Webserver-Port | 80 (HTTP) |
| OTA-Port | 3232 |
| MQTT-Standardport | 1883 |
