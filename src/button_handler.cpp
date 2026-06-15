// Bienenwaage3 – Taster-Zustandsautomat
// Autor: Johann Gerner

#include "button_handler.h"
#include "hx711_multi.h"
#include "lcd_display.h"
#include <ETH.h>
#include <WiFi.h>

extern Hx711Multi hx711;

void ButtonHandler::begin() {
    pinMode(BTN_SELECT_PIN, INPUT);
    pinMode(BTN_FUNC_PIN,   INPUT);
    _phase        = Phase::STARTUP_ETH;
    _phaseMs      = millis();
    _lastUpdateMs = millis();
    lcdPrint(0, "LAN");
    lcdPrint(1, "verbinde...");
}

void ButtonHandler::loop() {
    _handleStartup();
    if (_phase != Phase::NORMAL) return;
    _handleBtn1();
    _handleBtn2();
}

bool ButtonHandler::popTareYieldConfirmed() {
    if (_yieldConfirmed) {
        _yieldConfirmed = false;
        return true;
    }
    return false;
}

// ── Web-Simulation ────────────────────────────────────────────────────────────

void ButtonHandler::simulateBtn1() {
    if (_phase != Phase::NORMAL) return;
    _advanceModule();
}

void ButtonHandler::simulateBtn2() {
    if (_phase != Phase::NORMAL) return;
    hx711.startTareQuick(_selectedModule);
    lcdPrint(1, "Schnellmessung..");
}

// ── Startup-Anzeige ───────────────────────────────────────────────────────────

void ButtonHandler::_handleStartup() {
    if (_phase == Phase::NORMAL) return;
    unsigned long now = millis();

    if (_phase == Phase::STARTUP_ETH) {
        // Zeile 1 alle 500 ms aktualisieren bis ETH-IP vorliegt
        if (now - _lastUpdateMs >= 500UL) {
            _lastUpdateMs = now;
            String ip = ETH.localIP().toString();
            lcdPrint(1, (ip == "0.0.0.0") ? "verbinde..." : ip);
        }
        if (now - _phaseMs >= BTN_STARTUP_ETH_MS) {
            _phase        = Phase::STARTUP_AP;
            _phaseMs      = now;
            lcdPrint(0, "WiFi AP");
            lcdPrint(1, WiFi.softAPIP().toString());
        }
    } else {  // STARTUP_AP
        if (now - _phaseMs >= BTN_STARTUP_AP_MS) {
            _enterNormal();
        }
    }
}

// ── Modul-Anzeige ─────────────────────────────────────────────────────────────

void ButtonHandler::_enterNormal() {
    _phase          = Phase::NORMAL;
    _selectedModule = 0;
    _showModule();
}

void ButtonHandler::_showModule() {
    String line = "Modul ";
    line += (_selectedModule + 1);
    lcdPrint(0, line);
    lcdPrint(1, "");
}

void ButtonHandler::_advanceModule() {
    uint8_t count = hx711.getModuleCount();
    // Nächstes AKTIVES Modul wählen; inaktive überspringen (Plan 4.9)
    for (uint8_t step = 0; step < count; step++) {
        _selectedModule = (_selectedModule + 1) % count;
        if (hx711.getModule(_selectedModule).active) break;
    }
    _showModule();
}

// ── Taster 1: Modul wählen ────────────────────────────────────────────────────

void ButtonHandler::_handleBtn1() {
    bool pressed = digitalRead(BTN_SELECT_PIN) == HIGH;

    switch (_btn1State) {
        case BtnState::IDLE:
            if (pressed) {
                _btn1PressMs = millis();
                _btn1State   = BtnState::PRESSED;
            }
            break;

        case BtnState::PRESSED:
            if (!pressed) {
                if (millis() - _btn1PressMs >= BTN_DEBOUNCE_MS) {
                    _advanceModule();
                }
                _btn1State = BtnState::IDLE;
            }
            break;

        default:
            _btn1State = BtnState::IDLE;
            break;
    }
}

// ── Taster 2: Schnellmessung / Ertragstara ────────────────────────────────────

void ButtonHandler::_handleBtn2() {
    bool pressed = digitalRead(BTN_FUNC_PIN) == HIGH;
    unsigned long now = millis();

    switch (_btn2State) {
        case BtnState::IDLE:
            if (pressed) {
                _btn2PressMs = now;
                _btn2State   = BtnState::PRESSED;
            }
            break;

        case BtnState::PRESSED:
            if (!pressed) {
                unsigned long held = now - _btn2PressMs;
                if (held < BTN_DEBOUNCE_MS) {
                    _btn2State = BtnState::IDLE;
                    break;
                }
                if (held >= BTN_LONG_PRESS_MS) {
                    lcdPrint(0, "Ertragstara?");
                    lcdPrint(1, "Nochmal drueck.");
                    _pendingYieldConfirm = true;
                    _confirmMs  = now;
                    _btn2State  = BtnState::CONFIRM_WAIT;
                } else {
                    hx711.startTareQuick(_selectedModule);
                    lcdPrint(1, "Schnellmessung..");
                    _btn2State = BtnState::IDLE;
                }
            }
            break;

        case BtnState::CONFIRM_WAIT:
            if (pressed && (now - _btn2PressMs > BTN_DEBOUNCE_MS)) {
                _btn2PressMs = now;
            }
            if (!pressed && _pendingYieldConfirm &&
                (now - _btn2PressMs > BTN_DEBOUNCE_MS) &&
                (now - _confirmMs   > BTN_DEBOUNCE_MS)) {
                _yieldConfirmed      = true;
                _pendingYieldConfirm = false;
                hx711.startTareYield(_selectedModule);
                lcdPrint(0, "Ertragstara...");
                lcdPrint(1, "");
                _btn2State = BtnState::IDLE;
                break;
            }
            if (now - _confirmMs > BTN_CONFIRM_MS) {
                _pendingYieldConfirm = false;
                lcdPrint(0, "Abgebrochen");
                lcdPrint(1, "");
                _btn2State = BtnState::IDLE;
            }
            break;

        default:
            _btn2State = BtnState::IDLE;
            break;
    }
}
