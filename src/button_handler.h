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

    uint8_t getSelectedModule() const { return _selectedModule; }
    bool    popTareYieldConfirmed();

    // Web-Simulation: löst Kurzdruckverhalten aus
    void simulateBtn1();
    void simulateBtn2();

private:
    enum class Phase    { STARTUP_ETH, STARTUP_AP, NORMAL };
    enum class BtnState { IDLE, PRESSED, LONG_TRIGGERED, CONFIRM_WAIT };

    Phase         _phase        = Phase::STARTUP_ETH;
    unsigned long _phaseMs      = 0;
    unsigned long _lastUpdateMs = 0;

    uint8_t  _selectedModule   = 0;
    BtnState _btn1State        = BtnState::IDLE;
    BtnState _btn2State        = BtnState::IDLE;

    unsigned long _btn1PressMs = 0;
    unsigned long _btn2PressMs = 0;
    unsigned long _confirmMs   = 0;

    bool _pendingYieldConfirm = false;
    bool _yieldConfirmed      = false;

    void _handleStartup();
    void _handleBtn1();
    void _handleBtn2();
    void _advanceModule();
    void _showModule();
    void _enterNormal();
};
