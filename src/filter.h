// Bienenwaage3 – Median-Filter und Ausreisser-Erkennung

#pragma once
#include <Arduino.h>
#include <algorithm>

#include "config.h"

// Gleitender Median-Filter mit konfigurierbarem Ring-Buffer
class MedianFilter {
public:
    explicit MedianFilter(int size = 100) { resize(size); }
    void  resize(int size);
    void  push(float value);
    float getMedian(int trimLow = 0, int trimHigh = 0);
    float getSigma(int trimLow = 0, int trimHigh = 0);
    bool  isFull() const { return _count >= _size; }
    void  reset()        { _count = 0; _head = 0; }
    int   count() const  { return _count; }

private:
    static constexpr int MAX_SIZE = 200;
    float  _buf[MAX_SIZE] = {};
    int    _size  = 100;
    int    _head  = 0;
    int    _count = 0;
};

// Ausreisser-Filter: Verwirft Werte mit Sprung > threshold_g_per_s
class OutlierFilter {
public:
    explicit OutlierFilter(float threshold = HX711_OUTLIER_THRESHOLD)
        : _threshold(threshold) {}

    // Gibt true zurück wenn Wert gültig (kein Ausreisser)
    bool isValid(float newValue_g, unsigned long now_ms);
    void reset() { _lastValid = 0.0f; _lastTime = 0; _initialized = false; }
    void setThreshold(float t) { _threshold = t; }

private:
    float         _threshold    = 100.0f;
    float         _lastValid    = 0.0f;
    unsigned long _lastTime     = 0;
    bool          _initialized  = false;
};
