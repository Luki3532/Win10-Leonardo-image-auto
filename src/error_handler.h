/**
 * Error Handling Module
 * 
 * Provides error codes, detection, and display for hardware issues
 */

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <Arduino.h>
#include "../include/config.h"

// ============================================
// Error Codes
// ============================================
enum ErrorCode {
    ERR_NONE = 0,
    
    // Hardware Errors (E01-E09)
    ERR_LCD_NOT_FOUND       = 1,   // E01: LCD not detected on I2C bus
    ERR_LCD_INIT_FAILED     = 2,   // E02: LCD found but init failed
    ERR_I2C_BUS_ERROR       = 3,   // E03: I2C bus communication error
    ERR_KEYBOARD_INIT       = 4,   // E04: HID Keyboard init failed
    
    // Wiring Errors (E10-E19)
    ERR_SWITCH_FLOATING     = 10,  // E10: Switch pin reading unstable
    ERR_NO_PULLUP           = 11,  // E11: Internal pullup not working
    
    // Runtime Errors (E20-E29)
    ERR_BOOT_TIMEOUT        = 20,  // E20: Boot menu didn't appear
    ERR_SETUP_TIMEOUT       = 21,  // E21: Windows Setup didn't load
    ERR_PARTITION_FAILED    = 22,  // E22: Partition deletion issue
    ERR_INSTALL_FAILED      = 23,  // E23: Install didn't start
    
    // General Errors (E90-E99)
    ERR_UNKNOWN             = 99   // E99: Unknown error
};

// Error information structure
struct ErrorInfo {
    ErrorCode code;
    const char* shortMsg;    // 16 chars max for LCD line 1
    const char* detailMsg;   // 16 chars max for LCD line 2
    int ledBlinks;           // LED blink pattern (number of blinks)
};

// Get error info for a given code
ErrorInfo getErrorInfo(ErrorCode code);

// Display error on LCD (if available) and Serial
void displayError(ErrorCode code);

// Blink LED in error pattern (for when LCD isn't available)
void blinkErrorPattern(int errorNum);

// Halt with error - never returns
void haltWithError(ErrorCode code);

// Check hardware and return first error found (or ERR_NONE)
ErrorCode checkHardware();

// Check if I2C device exists at address
bool checkI2CDevice(uint8_t address);

// Check switch wiring
ErrorCode checkSwitchWiring();

#endif // ERROR_HANDLER_H
