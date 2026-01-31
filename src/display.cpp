/**
 * LCD Display Module Implementation
 */

#include "display.h"
#include <Wire.h>

// LCD instance (address, columns, rows)
static LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);
static bool lcdInitialized = false;

// Custom characters
byte arrowRight[8] = {
    0b00000,
    0b00100,
    0b00110,
    0b11111,
    0b00110,
    0b00100,
    0b00000,
    0b00000
};

byte checkMark[8] = {
    0b00000,
    0b00001,
    0b00011,
    0b10110,
    0b11100,
    0b01000,
    0b00000,
    0b00000
};

byte warning[8] = {
    0b00100,
    0b00100,
    0b01110,
    0b01110,
    0b11111,
    0b11111,
    0b00100,
    0b00000
};

byte skull[8] = {
    0b01110,
    0b10101,
    0b11111,
    0b01110,
    0b01110,
    0b00100,
    0b01110,
    0b00000
};

bool initDisplay() {
    Wire.begin();
    
    // Check if LCD is present before initializing
    Wire.beginTransmission(LCD_ADDRESS);
    if (Wire.endTransmission() != 0) {
        DEBUG_PRINTLN(F("LCD not found at configured address!"));
        lcdInitialized = false;
        return false;
    }
    
    lcd.init();
    lcd.backlight();
    lcd.clear();
    
    // Create custom characters
    lcd.createChar(0, arrowRight);  // \x00
    lcd.createChar(1, checkMark);   // \x01
    lcd.createChar(2, warning);     // \x02
    lcd.createChar(3, skull);       // \x03
    
    lcdInitialized = true;
    DEBUG_PRINTLN(F("LCD initialized"));
    return true;
}

LiquidCrystal_I2C& getLCD() {
    return lcd;
}

void showStatus(const char* line1, const char* line2) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
    
    DEBUG_PRINT(F("LCD: "));
    DEBUG_PRINT(line1);
    DEBUG_PRINT(F(" | "));
    DEBUG_PRINTLN(line2);
}

void showProgress(int current, int total, const char* title, const char* message) {
    lcd.clear();
    
    // Line 1: Title with progress counter
    lcd.setCursor(0, 0);
    lcd.print(title);
    lcd.print(" [");
    lcd.print(current);
    lcd.print("/");
    lcd.print(total);
    lcd.print("]");
    
    // Line 2: Message
    lcd.setCursor(0, 1);
    lcd.write(0);  // Arrow character
    lcd.print(" ");
    lcd.print(message);
    
    DEBUG_PRINT(F("Progress: "));
    DEBUG_PRINT(current);
    DEBUG_PRINT(F("/"));
    DEBUG_PRINT(total);
    DEBUG_PRINT(F(" - "));
    DEBUG_PRINTLN(message);
}

void showCountdown(const char* title, const char* prefix, int seconds) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(title);
    
    for (int i = seconds; i > 0; i--) {
        lcd.setCursor(0, 1);
        lcd.print(prefix);
        lcd.print(" ");
        
        // Right-align the countdown
        if (i < 10) lcd.print(" ");
        lcd.print(i);
        lcd.print("s   ");  // Extra spaces to clear old digits
        
        delay(1000);
    }
}

void showComplete() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(1);  // Checkmark
    lcd.print(" COMPLETE ");
    lcd.write(1);  // Checkmark
    
    lcd.setCursor(0, 1);
    lcd.print("Installing Win!");
    
    DEBUG_PRINTLN(F("=== COMPLETE ==="));
}

void showError(const char* message) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(2);  // Warning
    lcd.print(" ERROR ");
    lcd.write(2);  // Warning
    
    lcd.setCursor(0, 1);
    lcd.print(message);
    
    DEBUG_PRINT(F("ERROR: "));
    DEBUG_PRINTLN(message);
}

void showError(const char* codeLine, const char* detailLine) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write(2);  // Warning
    lcd.print(codeLine);
    
    lcd.setCursor(0, 1);
    lcd.print(detailLine);
    
    DEBUG_PRINT(F("ERROR: "));
    DEBUG_PRINT(codeLine);
    DEBUG_PRINT(F(" - "));
    DEBUG_PRINTLN(detailLine);
}

bool isLCDConnected() {
    Wire.beginTransmission(LCD_ADDRESS);
    return (Wire.endTransmission() == 0);
}

void showSafeMode() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("  SAFE MODE");
    
    lcd.setCursor(0, 1);
    lcd.print("Switch is OFF");
    
    DEBUG_PRINTLN(F("Safe mode - switch is OFF"));
}

void showScanMode() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("I2C SCAN MODE");
    lcd.setCursor(0, 1);
    lcd.print("Check Serial...");
}

void flashDisplay(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        lcd.noBacklight();
        delay(delayMs);
        lcd.backlight();
        delay(delayMs);
    }
}
