// Bienenwaage3 – DS18B20 Temperatursensor (nicht-blockierend, 12-Bit)
// Hardware: OneWire auf GPIO4
// Autor: Johann Gerner

#pragma once
#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include "config.h"

class TemperatureSensor {
public:
    void  begin();
    void  loop();

    float getTemperature() const { return _currentTemp; }
    bool  isOnline()       const { return _online; }

private:
    OneWire          _oneWire{ONEWIRE_PIN};
    DallasTemperature _sensors{&_oneWire};

    float         _currentTemp  = 20.0f;
    bool          _online       = false;
    bool          _convRequested = false;
    unsigned long _convStartMs  = 0;
};
