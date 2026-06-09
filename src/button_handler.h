// Bienenwaage3 – Taster-Zustandsautomat (GPIO36 + GPIO39)
// Kurz-/Langdruck ohne delay(), entprellt per millis()
// Autor: Johann Gerner

#pragma once
#include <Arduino.h>
#include "config.h"

class ButtonHandler {
public:
    void begin();
    void loop();

    // Zugriff auf aktuell gewähltes Modul (für LCD und Web)
    uint8_t getSelectedModule() const { return _selectedModule; }

    // Ausstehende Aktionen (werden nach Abfrage automatisch zurückgesetzt)
    bool popTareYieldConfirmed();   // true = Ertragstara bestätigt

private:
    enum class BtnState { IDLE, PRESSED, LONG_TRIGGERED, CONFIRM_WAIT };

    uint8_t  _selectedModule   = 0;
    BtnState _btn1State        = BtnState::IDLE;
    BtnState _btn2State        = BtnState::IDLE;

    unsigned long _btn1PressMs = 0;
    unsigned long _btn2PressMs = 0;
    unsigned long _confirmMs   = 0;

    bool _pendingYieldConfirm = false;
    bool _yieldConfirmed      = false;

    void _handleBtn1();
    void _handleBtn2();
};
