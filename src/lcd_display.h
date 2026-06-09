// Bienenwaage3 – I2C-LCD 16x2 (PCF8574, Adresse 0x27)
// Zentrale lcdPrint()-Funktion synchronisiert Hardware und Web-Buffer
// Autor: Johann Gerner

#pragma once
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"

class LcdDisplay {
public:
    void begin();
    void clear();
    void print(uint8_t row, const String& text);

    // Web-Buffer-Zugriff für /data JSON
    const String& getLine(uint8_t row) const { return _webBuffer[row < LCD_ROWS ? row : 0]; }

private:
    LiquidCrystal_I2C _lcd{LCD_I2C_ADDRESS, LCD_COLS, LCD_ROWS};
    String            _webBuffer[LCD_ROWS];

    String _pad(const String& text) const;
};

// Globale Hilfsfunktion – überall im Sketch statt lcd.print() verwenden
void lcdPrint(uint8_t row, const String& text);
