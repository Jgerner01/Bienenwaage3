// Bienenwaage3 – HX711 Multi-Kanal Manager
// Synchrones Lesen aller Module über gemeinsamen SCK (GPIO5)
// DOUT-Pins werden per GPIO.in / GPIO.in1.val atomar gelesen

#pragma once
#include <Arduino.h>
#include "config.h"
#include "filter.h"

struct ModuleData {
    // Messwerte
    float weightMain_g      = 0.0f;  // Hauptmessung (Median, temperaturkomp.)
    float weightQuick_g     = 0.0f;  // Schnellmessung (Median 20 Werte)
    float sigma_g           = 0.0f;  // Standardabweichung Hauptmessung
    float rawAdc            = 0.0f;  // Letzter Roh-ADC-Wert
    bool  online            = false; // Modul erreichbar

    // Konfiguration (wird aus NVS geladen)
    bool    active          = false;
    float   calibFactor     = 1.0f;  // g pro ADC-Einheit
    float   tareMainAdc     = 0.0f;  // Grundtara (ADC-Counts, einheitenkonsistent)
    float   tareYieldAdc    = 0.0f;  // Ertragstara (ADC-Counts, persistent)
    float   polyA2          = TEMP_POLY_A2_DEFAULT;
    float   polyA1          = TEMP_POLY_A1_DEFAULT;
    float   polyA0          = TEMP_POLY_A0_DEFAULT;
    int     mainBufferSize  = HX711_MAIN_BUFFER_SIZE;
    float   outlierThresh   = HX711_OUTLIER_THRESHOLD;

    // Interne Filter
    MedianFilter mainFilter;
    MedianFilter quickFilter;
    OutlierFilter outlierFilter;
};

class Hx711Multi {
public:
    void begin();
    void loop();

    // Tara-Funktionen
    void startTareMain(uint8_t module);    // Grundtara (50-Werte-Median)
    void startTareYield(uint8_t module);   // Ertragstara (50-Werte-Median, persistent)
    void startTareQuick(uint8_t module);   // Schnellmess-Tara (10 Werte)
    void clearTareYield(uint8_t module);   // Ertragstara zurücksetzen

    // Kalibrierung (nicht-blockierend: sammelt Roh-Median, dann Faktor)
    void calibrate(uint8_t module, float knownWeight_g);

    // Geänderte Parameter (Web/Import) übernehmen: async-sicher, wird im
    // loop()-Kontext angewandt (Filter-resize läuft sonst parallel zur Messung)
    void applyModuleParams(uint8_t module, const ModuleData& params);

    // Datenzugriff
    const ModuleData& getModule(uint8_t idx) const { return _modules[idx]; }
    uint8_t getModuleCount() const { return HX711_MAX_MODULES; }

    // Temperatur für Kompensation setzen (wird von TemperatureSensor gesetzt)
    void setTemperature(float temp_c) { _currentTemp = temp_c; }

private:
    ModuleData _modules[HX711_MAX_MODULES];
    float      _currentTemp = 20.0f;

    // Synchrones Lesen aller DOUT-Pins via GPIO-Register
    void     _readAllChannels();
    int32_t  _extractBit(uint8_t gpio, uint32_t reg_low, uint32_t reg_high);
    float    _applyTempCompensation(uint8_t idx, float raw_g);

    void _processTara();

    // Aus async-Kontext angeforderte Parameteränderungen im loop() übernehmen
    void _applyPendingParams();
    volatile bool _pendingApply[HX711_MAX_MODULES] = {};
    ModuleData    _pendingParams[HX711_MAX_MODULES];

    // Letzter Zeitpunkt mit gültigen Daten je Modul (für Online-Timeout)
    unsigned long _lastReadyMs[HX711_MAX_MODULES] = {};

    // Tara-/Kalibrier-Sampler (non-blocking)
    enum class TaraState { IDLE, COLLECTING_MAIN, COLLECTING_YIELD, COLLECTING_QUICK, COLLECTING_CALIB };
    TaraState _taraState[HX711_MAX_MODULES] = {};
    int       _taraSampleCount[HX711_MAX_MODULES] = {};
    float     _taraSamples[HX711_MAX_MODULES][HX711_TARA_BUFFER_SIZE] = {};
    int       _taraTargetSamples[HX711_MAX_MODULES] = {};
    float     _calibKnownWeight[HX711_MAX_MODULES] = {};  // Sollgewicht für COLLECTING_CALIB
};
