/**
 * LCD Display Module
 * 
 * Handles all display operations for the 16x2 I2C LCD
 * with HW-061 backpack (PCF8574-based)
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "../include/config.h"

// Initialize the LCD display (returns false if LCD not found)
bool initDisplay();

// Get LCD instance for direct access
LiquidCrystal_I2C& getLCD();

// Clear display and show two lines of text
void showStatus(const char* line1, const char* line2);

// Show progress with step counter (e.g., "SETUP [2/5]")
void showProgress(int current, int total, const char* title, const char* message);

// Show countdown timer (updates in place)
void showCountdown(const char* title, const char* prefix, int seconds);

// Show completion screen
void showComplete();

// Show error message (single line)
void showError(const char* message);

// Show error with code and detail (two lines)
void showError(const char* codeLine, const char* detailLine);

// Check if LCD is responding
bool isLCDConnected();

// Show safe mode (switch is OFF)
void showSafeMode();

// Show I2C scan mode message
void showScanMode();

// Flash the display (visual alert)
void flashDisplay(int times, int delayMs);

#endif // DISPLAY_H
