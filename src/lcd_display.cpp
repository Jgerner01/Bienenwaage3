// Bienenwaage3 – I2C-LCD 16x2
// Autor: Johann Gerner

#include "lcd_display.h"

// Globales LCD-Objekt (extern in main.cpp deklariert)
extern LcdDisplay lcd;

void LcdDisplay::begin() {
    _lcd.init();
    _lcd.backlight();
    _lcd.clear();
    _webBuffer[0] = "";
    _webBuffer[1] = "";
}

void LcdDisplay::clear() {
    _lcd.clear();
    _webBuffer[0] = "";
    _webBuffer[1] = "";
}

// Auf LCD schreiben und Web-Buffer synchron halten
void LcdDisplay::print(uint8_t row, const String& text) {
    if (row >= LCD_ROWS) return;
    String padded = _pad(text);
    _webBuffer[row] = padded;
    _lcd.setCursor(0, row);
    _lcd.print(padded);
}

// Text auf LCD_COLS Zeichen auffüllen/kürzen (verhindert LCD-Geisterschrift)
String LcdDisplay::_pad(const String& text) const {
    String result = text;
    result.reserve(LCD_COLS);
    while ((int)result.length() < LCD_COLS) result += ' ';
    if ((int)result.length() > LCD_COLS) result = result.substring(0, LCD_COLS);
    return result;
}

// ── Globale Hilfsfunktion ──────────────────────────────────────────────────────

void lcdPrint(uint8_t row, const String& text) {
    lcd.print(row, text);
}
