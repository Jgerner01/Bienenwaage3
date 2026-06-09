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
    }

#ifdef DEBUG_SERIAL
    Serial.println("[HX711] Initialisierung abgeschlossen");
#endif
}

// ── loop ───────────────────────────────────────────────────────────────────────

void Hx711Multi::loop() {
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

    // Rohdaten verarbeiten und in Filter einspeisen
    unsigned long now = millis();
    for (uint8_t i = 0; i < HX711_MAX_MODULES; i++) {
        if (!ready[i]) { _modules[i].online = false; continue; }
        _modules[i].online = true;

        // 24-Bit Zweierkomplement → int32_t
        if (rawValues[i] & 0x800000) rawValues[i] |= 0xFF000000;

        float raw_g = (float)rawValues[i] * _modules[i].calibFactor;
        float net_g = raw_g - _modules[i].tareMain_g - _modules[i].tareYield_g;
        float comp_g = _applyTempCompensation(i, net_g);

        _modules[i].rawAdc = (float)rawValues[i];

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
    _modules[m].tareYield_g = 0.0f;
    storage.saveModuleParams(m, _modules[m]);
}

void Hx711Multi::calibrate(uint8_t m, float knownWeight_g) {
    if (m >= HX711_MAX_MODULES || knownWeight_g == 0.0f) return;
    float currentRaw = _modules[m].rawAdc;
    float tare       = _modules[m].tareMain_g;
    if ((currentRaw - tare) == 0.0f) return;
    _modules[m].calibFactor = knownWeight_g / (currentRaw - tare);
    storage.saveModuleParams(m, _modules[m]);
}

void Hx711Multi::_processTara() {
    for (uint8_t m = 0; m < HX711_MAX_MODULES; m++) {
        if (_taraState[m] == TaraState::IDLE) continue;
        if (!_modules[m].online) continue;

        float raw_g = _modules[m].rawAdc * _modules[m].calibFactor;
        if (_taraSampleCount[m] < _taraTargetSamples[m]) {
            _taraSamples[m][_taraSampleCount[m]++] = raw_g;
        }

        if (_taraSampleCount[m] >= _taraTargetSamples[m]) {
            // Median der gesammelten Samples
            float sorted[50];
            int n = _taraSampleCount[m];
            memcpy(sorted, _taraSamples[m], n * sizeof(float));
            std::sort(sorted, sorted + n);
            float medianVal = sorted[n / 2];

            if (_taraState[m] == TaraState::COLLECTING_MAIN) {
                _modules[m].tareMain_g = medianVal;
                _modules[m].mainFilter.reset();
                _modules[m].outlierFilter.reset();
                storage.saveModuleParams(m, _modules[m]);
#ifdef DEBUG_SERIAL
                Serial.printf("[HX711] Grundtara Modul %d: %.1f g\n", m, medianVal);
#endif
            } else if (_taraState[m] == TaraState::COLLECTING_YIELD) {
                _modules[m].tareYield_g = medianVal - _modules[m].tareMain_g;
                storage.saveModuleParams(m, _modules[m]);
#ifdef DEBUG_SERIAL
                Serial.printf("[HX711] Ertragstara Modul %d: %.1f g\n", m, medianVal);
#endif
            } else if (_taraState[m] == TaraState::COLLECTING_QUICK) {
                _modules[m].tareMain_g = medianVal;
                // Schnellmess-Tara nur im RAM (kein NVS-Speichern)
            }
            _taraState[m] = TaraState::IDLE;
        }
    }
}
