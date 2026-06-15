// Bienenwaage3 – HX711 Multi-Kanal Manager
// Synchrones Lesen aller Module über gemeinsamen SCK (GPIO5)
// Autor: Johann Gerner

#include "hx711_multi.h"
#include "storage.h"
#include <Arduino.h>
#include <soc/gpio_struct.h>

extern StorageManager storage;

// ── begin ──────────────────────────────────────────────────────────────────────

void Hx711Multi::begin() {
    pinMode(HX711_SCK_PIN, OUTPUT);
    digitalWrite(HX711_SCK_PIN, LOW);

    for (uint8_t i = 0; i < HX711_MAX_MODULES; i++) {
        uint8_t pin = HX711_DOUT_PINS[i];
        // GPIO34-39 sind Input-Only, kein Pull-Up möglich
        if (pin < 34)
            pinMode(pin, INPUT_PULLUP);
        else
            pinMode(pin, INPUT);

        storage.loadModuleParams(i, _modules[i]);

        _modules[i].mainFilter.resize(_modules[i].mainBufferSize);
        _modules[i].quickFilter.resize(HX711_QUICK_BUFFER_SIZE);
        _modules[i].outlierFilter.setThreshold(_modules[i].outlierThresh);
        _modules[i].online = false;
        _lastReadyMs[i]    = 0;
    }

#ifdef DEBUG_SERIAL
    Serial.println("[HX711] Initialisierung abgeschlossen");
#endif
}

// ── loop ───────────────────────────────────────────────────────────────────────

void Hx711Multi::loop() {
    // Im async-Kontext (Web/Import) angeforderte Parameteränderungen anwenden
    _applyPendingParams();

    // Prüfen ob mindestens ein Modul bereit (DOUT LOW = Daten fertig)
    bool anyReady = false;
    for (uint8_t i = 0; i < HX711_MAX_MODULES; i++) {
        if (!_modules[i].active) continue;
        if (digitalRead(HX711_DOUT_PINS[i]) == LOW) {
            anyReady = true;
            break;
        }
    }
    if (!anyReady) return;

    _readAllChannels();
    _processTara();
}

// ── Synchrones Lesen aller DOUT-Pins ──────────────────────────────────────────

void Hx711Multi::_readAllChannels() {
    int32_t rawValues[HX711_MAX_MODULES] = {};
    bool    ready[HX711_MAX_MODULES]     = {};

    // Bereitschaft aller aktiven Module prüfen
    for (uint8_t i = 0; i < HX711_MAX_MODULES; i++) {
        if (!_modules[i].active) continue;
        ready[i] = (digitalRead(HX711_DOUT_PINS[i]) == LOW);
    }

    // Bit-Burst gegen Interrupts schützen: ein WiFi/ETH-IRQ während eines
    // SCK-HIGH-Pulses könnte diesen > 60 µs strecken → HX711 Power-Down.
    static portMUX_TYPE hx711Mux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&hx711Mux);

    // 24 Datenbits atomar über gemeinsamen SCK lesen
    for (int bit = 23; bit >= 0; bit--) {
        // SCK HIGH über direktes Registerbit (kein digitalWriteFunc-Overhead)
        GPIO.out_w1ts = (1UL << HX711_SCK_PIN);
        delayMicroseconds(1);

        // Alle GPIO-Register in einem Zug lesen
        uint32_t reg_low  = GPIO.in;      // GPIO  0–31
        uint32_t reg_high = GPIO.in1.val; // GPIO 32–39

        GPIO.out_w1tc = (1UL << HX711_SCK_PIN);
        delayMicroseconds(1);

        for (uint8_t i = 0; i < HX711_MAX_MODULES; i++) {
            if (!ready[i]) continue;
            int b = _extractBit(HX711_DOUT_PINS[i], reg_low, reg_high);
            rawValues[i] = (rawValues[i] << 1) | b;
        }
    }

    // 25. Puls: Gain 128 für nächste Messung (HX711-Protokoll)
    GPIO.out_w1ts = (1UL << HX711_SCK_PIN);
    delayMicroseconds(1);
    GPIO.out_w1tc = (1UL << HX711_SCK_PIN);

    portEXIT_CRITICAL(&hx711Mux);

    // Rohdaten verarbeiten und in Filter einspeisen
    unsigned long now = millis();
    for (uint8_t i = 0; i < HX711_MAX_MODULES; i++) {
        if (!_modules[i].active) continue;

        // Online-Status mit Timeout: ein einzelner verpasster Lesetakt
        // (Modul mitten in der Wandlung) markiert das Modul nicht sofort offline.
        if (!ready[i]) {
            if (now - _lastReadyMs[i] > HX711_ONLINE_TIMEOUT_MS)
                _modules[i].online = false;
            continue;
        }
        _modules[i].online = true;
        _lastReadyMs[i]    = now;

        // 24-Bit Zweierkomplement → int32_t
        if (rawValues[i] & 0x800000) rawValues[i] |= 0xFF000000;

        // Einheitenkonsistent: erst Tara in ADC-Counts, dann skalieren
        _modules[i].rawAdc = (float)rawValues[i];
        float net_adc = (float)rawValues[i]
                        - _modules[i].tareMainAdc
                        - _modules[i].tareYieldAdc;
        float net_g  = net_adc * _modules[i].calibFactor;
        float comp_g = _applyTempCompensation(i, net_g);

        if (_modules[i].outlierFilter.isValid(comp_g, now)) {
            _modules[i].mainFilter.push(comp_g);
            _modules[i].quickFilter.push(comp_g);
        }

        if (_modules[i].mainFilter.isFull()) {
            _modules[i].weightMain_g  = _modules[i].mainFilter.getMedian(
                HX711_MAIN_TRIM_LOW, HX711_MAIN_TRIM_HIGH);
            _modules[i].sigma_g       = _modules[i].mainFilter.getSigma(
                HX711_MAIN_TRIM_LOW, HX711_MAIN_TRIM_HIGH);
        }
        _modules[i].weightQuick_g = _modules[i].quickFilter.getMedian();
    }
}

int32_t Hx711Multi::_extractBit(uint8_t gpio, uint32_t reg_low, uint32_t reg_high) {
    if (gpio < 32)
        return (reg_low  >> gpio) & 1;
    else
        return (reg_high >> (gpio - 32)) & 1;
}

float Hx711Multi::_applyTempCompensation(uint8_t idx, float raw_g) {
    float t  = _currentTemp;
    float a2 = _modules[idx].polyA2;
    float a1 = _modules[idx].polyA1;
    float a0 = _modules[idx].polyA0;
    return raw_g + a2 * t * t + a1 * t + a0;
}

// ── Tara ───────────────────────────────────────────────────────────────────────

void Hx711Multi::startTareMain(uint8_t m) {
    if (m >= HX711_MAX_MODULES) return;
    _taraState[m]         = TaraState::COLLECTING_MAIN;
    _taraSampleCount[m]   = 0;
    _taraTargetSamples[m] = HX711_TARE_MAIN_SAMPLES;
    _modules[m].mainFilter.reset();
}

void Hx711Multi::startTareYield(uint8_t m) {
    if (m >= HX711_MAX_MODULES) return;
    _taraState[m]         = TaraState::COLLECTING_YIELD;
    _taraSampleCount[m]   = 0;
    _taraTargetSamples[m] = HX711_TARE_MAIN_SAMPLES;
}

void Hx711Multi::startTareQuick(uint8_t m) {
    if (m >= HX711_MAX_MODULES) return;
    _taraState[m]         = TaraState::COLLECTING_QUICK;
    _taraSampleCount[m]   = 0;
    _taraTargetSamples[m] = HX711_TARE_QUICK_SAMPLES;
}

void Hx711Multi::clearTareYield(uint8_t m) {
    if (m >= HX711_MAX_MODULES) return;
    _modules[m].tareYieldAdc = 0.0f;
    storage.saveModuleParams(m, _modules[m]);
}

void Hx711Multi::calibrate(uint8_t m, float knownWeight_g) {
    if (m >= HX711_MAX_MODULES || knownWeight_g == 0.0f) return;
    // Nicht-blockierend: Roh-ADC-Median sammeln, Faktor in _processTara berechnen
    _calibKnownWeight[m]  = knownWeight_g;
    _taraState[m]         = TaraState::COLLECTING_CALIB;
    _taraSampleCount[m]   = 0;
    _taraTargetSamples[m] = HX711_TARE_MAIN_SAMPLES;
}

void Hx711Multi::applyModuleParams(uint8_t m, const ModuleData& params) {
    if (m >= HX711_MAX_MODULES) return;
    // Async-sicher: Werte hinterlegen, Übernahme erfolgt in _applyPendingParams()
    _pendingParams[m] = params;
    _pendingApply[m]  = true;
}

void Hx711Multi::_applyPendingParams() {
    for (uint8_t m = 0; m < HX711_MAX_MODULES; m++) {
        if (!_pendingApply[m]) continue;
        _pendingApply[m] = false;

        // Nur Konfigurationsfelder übernehmen – Messwerte/Tara bleiben erhalten
        _modules[m].active         = _pendingParams[m].active;
        _modules[m].calibFactor    = _pendingParams[m].calibFactor;
        _modules[m].polyA2         = _pendingParams[m].polyA2;
        _modules[m].polyA1         = _pendingParams[m].polyA1;
        _modules[m].polyA0         = _pendingParams[m].polyA0;
        _modules[m].mainBufferSize = _pendingParams[m].mainBufferSize;
        _modules[m].outlierThresh  = _pendingParams[m].outlierThresh;

        // Live auf die laufenden Filter anwenden (im Mess-Task, keine Race-Condition)
        _modules[m].mainFilter.resize(_modules[m].mainBufferSize);
        _modules[m].outlierFilter.setThreshold(_modules[m].outlierThresh);

        storage.saveModuleParams(m, _modules[m]);
    }
}

void Hx711Multi::_processTara() {
    for (uint8_t m = 0; m < HX711_MAX_MODULES; m++) {
        if (_taraState[m] == TaraState::IDLE) continue;
        if (!_modules[m].online) continue;

        // Tara/Kalibrierung arbeiten einheitenkonsistent in ADC-Counts
        if (_taraSampleCount[m] < _taraTargetSamples[m]) {
            _taraSamples[m][_taraSampleCount[m]++] = _modules[m].rawAdc;
        }
        if (_taraSampleCount[m] < _taraTargetSamples[m]) continue;

        // Median der gesammelten Roh-ADC-Samples
        float sorted[HX711_TARA_BUFFER_SIZE];
        int n = _taraSampleCount[m];
        memcpy(sorted, _taraSamples[m], n * sizeof(float));
        std::sort(sorted, sorted + n);
        float medianAdc = sorted[n / 2];

        switch (_taraState[m]) {
            case TaraState::COLLECTING_MAIN:
                _modules[m].tareMainAdc = medianAdc;
                _modules[m].mainFilter.reset();
                _modules[m].outlierFilter.reset();
                storage.saveModuleParams(m, _modules[m]);
#ifdef DEBUG_SERIAL
                Serial.printf("[HX711] Grundtara Modul %d: %.0f ADC\n", m, medianAdc);
#endif
                break;

            case TaraState::COLLECTING_YIELD:
                _modules[m].tareYieldAdc = medianAdc - _modules[m].tareMainAdc;
                storage.saveModuleParams(m, _modules[m]);
#ifdef DEBUG_SERIAL
                Serial.printf("[HX711] Ertragstara Modul %d: %.0f ADC\n", m, medianAdc);
#endif
                break;

            case TaraState::COLLECTING_QUICK:
                _modules[m].tareMainAdc = medianAdc; // nur RAM, kein NVS
                break;

            case TaraState::COLLECTING_CALIB: {
                float netAdc = medianAdc - _modules[m].tareMainAdc;
                if (netAdc != 0.0f) {
                    _modules[m].calibFactor = _calibKnownWeight[m] / netAdc;
                    storage.saveModuleParams(m, _modules[m]);
#ifdef DEBUG_SERIAL
                    Serial.printf("[HX711] Kalibrierung Modul %d: %.6f g/ADC\n",
                                  m, _modules[m].calibFactor);
#endif
                }
                break;
            }

            default:
                break;
        }
        _taraState[m] = TaraState::IDLE;
    }
}
