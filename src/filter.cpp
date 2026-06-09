// Bienenwaage3 – Median-Filter und Ausreisser-Erkennung
// Hardware: WT32-ETH01-V1.4
// Autor: Johann Gerner

#include "filter.h"
#include <cmath>
#include <cstring>

// ── MedianFilter ───────────────────────────────────────────────────────────────

void MedianFilter::resize(int size) {
    if (size < 1) size = 1;
    if (size > MAX_SIZE) size = MAX_SIZE;
    _size  = size;
    _head  = 0;
    _count = 0;
    memset(_buf, 0, sizeof(_buf));
}

void MedianFilter::push(float value) {
    _buf[_head] = value;
    _head = (_head + 1) % _size;
    if (_count < _size) _count++;
}

// Gibt Median der letzten _count Werte zurück, trimLow/trimHigh Enden verwerfen
float MedianFilter::getMedian(int trimLow, int trimHigh) {
    if (_count == 0) return 0.0f;

    float sorted[MAX_SIZE];
    memcpy(sorted, _buf, _count * sizeof(float));
    std::sort(sorted, sorted + _count);

    int lo = trimLow;
    int hi = _count - trimHigh;
    if (lo >= hi) {
        lo = 0;
        hi = _count;
    }
    int usable = hi - lo;
    if (usable <= 0) return sorted[_count / 2];

    if (usable % 2 == 1)
        return sorted[lo + usable / 2];
    return (sorted[lo + usable / 2 - 1] + sorted[lo + usable / 2]) * 0.5f;
}

// Standardabweichung der (nach Trim verbleibenden) Werte
float MedianFilter::getSigma(int trimLow, int trimHigh) {
    if (_count < 2) return 0.0f;

    float sorted[MAX_SIZE];
    memcpy(sorted, _buf, _count * sizeof(float));
    std::sort(sorted, sorted + _count);

    int lo = trimLow;
    int hi = _count - trimHigh;
    if (lo >= hi) { lo = 0; hi = _count; }
    int usable = hi - lo;
    if (usable < 2) return 0.0f;

    float sum = 0.0f;
    for (int i = lo; i < hi; i++) sum += sorted[i];
    float mean = sum / usable;

    float variance = 0.0f;
    for (int i = lo; i < hi; i++) {
        float d = sorted[i] - mean;
        variance += d * d;
    }
    return sqrtf(variance / (usable - 1));
}

// ── OutlierFilter ──────────────────────────────────────────────────────────────

bool OutlierFilter::isValid(float newValue_g, unsigned long now_ms) {
    if (!_initialized) {
        _lastValid   = newValue_g;
        _lastTime    = now_ms;
        _initialized = true;
        return true;
    }

    unsigned long dt_ms = now_ms - _lastTime;
    if (dt_ms == 0) dt_ms = 1;
    float dt_s = dt_ms / 1000.0f;

    float rate = fabsf(newValue_g - _lastValid) / dt_s;
    if (rate > _threshold) return false;

    _lastValid = newValue_g;
    _lastTime  = now_ms;
    return true;
}
