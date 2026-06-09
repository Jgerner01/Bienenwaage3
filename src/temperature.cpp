// Bienenwaage3 – DS18B20 Temperatursensor (nicht-blockierend, 12-Bit)
// Autor: Johann Gerner

#include "temperature.h"
#include "hx711_multi.h"

extern Hx711Multi hx711;

void TemperatureSensor::begin() {
    _sensors.begin();
    _sensors.setResolution(DS18B20_RESOLUTION);
    _sensors.setWaitForConversion(false); // nicht-blockierend

    // Erste Wandlung anstoßen
    _sensors.requestTemperatures();
    _convRequested = true;
    _convStartMs   = millis();

#ifdef DEBUG_SERIAL
    Serial.printf("[TEMP] %d Sensor(en) gefunden\n", _sensors.getDeviceCount());
#endif
}

void TemperatureSensor::loop() {
    if (!_convRequested) {
        _sensors.requestTemperatures();
        _convRequested = true;
        _convStartMs   = millis();
        return;
    }

    if (millis() - _convStartMs < DS18B20_CONVERT_DELAY_MS) return;

    float t = _sensors.getTempCByIndex(0);
    _convRequested = false;

    // -127 °C = kein Sensor, 85 °C = Fehler (Power-On-Reset-Wert)
    if (t <= TEMP_ERROR_VALUE || t >= 84.0f) {
        _online = false;
#ifdef DEBUG_SERIAL
        Serial.printf("[TEMP] Sensorfehler: %.2f\n", t);
#endif
    } else {
        _currentTemp = t;
        _online      = true;
        hx711.setTemperature(t);
    }
}
