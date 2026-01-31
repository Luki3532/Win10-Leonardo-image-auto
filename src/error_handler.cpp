/**
 * Error Handling Module Implementation
 */

#include "error_handler.h"
#include "display.h"
#include <Wire.h>

// Error definitions table
ErrorInfo getErrorInfo(ErrorCode code) {
    ErrorInfo info;
    info.code = code;
    info.ledBlinks = (int)code;  // Default: blink count = error number
    
    switch (code) {
        case ERR_NONE:
            info.shortMsg = "NO ERROR";
            info.detailMsg = "All OK";
            info.ledBlinks = 0;
            break;
            
        // Hardware Errors
        case ERR_LCD_NOT_FOUND:
            info.shortMsg = "E01:LCD MISSING";
            info.detailMsg = "Check I2C wiring";
            info.ledBlinks = 1;
            break;
            
        case ERR_LCD_INIT_FAILED:
            info.shortMsg = "E02:LCD FAILED";
            info.detailMsg = "Wrong address?";
            info.ledBlinks = 2;
            break;
            
        case ERR_I2C_BUS_ERROR:
            info.shortMsg = "E03:I2C ERROR";
            info.detailMsg = "SDA/SCL wiring";
            info.ledBlinks = 3;
            break;
            
        case ERR_KEYBOARD_INIT:
            info.shortMsg = "E04:USB ERROR";
            info.detailMsg = "HID init failed";
            info.ledBlinks = 4;
            break;
            
        // Wiring Errors
        case ERR_SWITCH_FLOATING:
            info.shortMsg = "E10:BAD BUTTON";
            info.detailMsg = "Pin floating";
            info.ledBlinks = 10;
            break;
            
        case ERR_NO_PULLUP:
            info.shortMsg = "E11:NO PULLUP";
            info.detailMsg = "Check pin 7";
            info.ledBlinks = 11;
            break;
            
        // Runtime Errors
        case ERR_BOOT_TIMEOUT:
            info.shortMsg = "E20:BOOT FAIL";
            info.detailMsg = "No boot menu";
            info.ledBlinks = 20;
            break;
            
        case ERR_SETUP_TIMEOUT:
            info.shortMsg = "E21:SETUP FAIL";
            info.detailMsg = "Win not loaded";
            info.ledBlinks = 21;
            break;
            
        case ERR_PARTITION_FAILED:
            info.shortMsg = "E22:WIPE FAIL";
            info.detailMsg = "Partition error";
            info.ledBlinks = 22;
            break;
            
        case ERR_INSTALL_FAILED:
            info.shortMsg = "E23:INSTALL ERR";
            info.detailMsg = "Didn't start";
            info.ledBlinks = 23;
            break;
            
        default:
            info.shortMsg = "E99:UNKNOWN";
            info.detailMsg = "Unknown error";
            info.ledBlinks = 99;
            break;
    }
    
    return info;
}

bool checkI2CDevice(uint8_t address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    return (error == 0);
}

ErrorCode checkSwitchWiring() {
    // Read button multiple times to check for floating pin
    int highCount = 0;
    int lowCount = 0;
    
    for (int i = 0; i < 10; i++) {
        if (digitalRead(ARM_BUTTON_PIN) == HIGH) {
            highCount++;
        } else {
            lowCount++;
        }
        delay(5);
    }
    
    // If readings are inconsistent, pin might be floating
    if (highCount > 0 && lowCount > 0 && highCount < 8 && lowCount < 8) {
        return ERR_SWITCH_FLOATING;
    }
    
    return ERR_NONE;
}

ErrorCode checkHardware() {
    // Initialize I2C
    Wire.begin();
    
    // Check 1: Is anything on the I2C bus?
    bool foundAnyDevice = false;
    for (uint8_t addr = 1; addr < 127; addr++) {
        if (checkI2CDevice(addr)) {
            foundAnyDevice = true;
            break;
        }
    }
    
    if (!foundAnyDevice) {
        return ERR_I2C_BUS_ERROR;
    }
    
    // Check 2: Is LCD at expected address?
    if (!checkI2CDevice(LCD_ADDRESS)) {
        // Try alternate common address
        if (checkI2CDevice(0x3F)) {
            // Found at different address - still an error (wrong config)
            return ERR_LCD_INIT_FAILED;
        }
        return ERR_LCD_NOT_FOUND;
    }
    
    // Check 3: Switch wiring
    ErrorCode switchErr = checkSwitchWiring();
    if (switchErr != ERR_NONE) {
        return switchErr;
    }
    
    return ERR_NONE;
}

void blinkErrorPattern(int errorNum) {
    // Pattern: blink error number, pause, repeat
    // For large numbers, blink tens then ones with pause between
    
    int tens = errorNum / 10;
    int ones = errorNum % 10;
    
    while (true) {
        // Blink tens digit
        if (tens > 0) {
            for (int i = 0; i < tens; i++) {
                digitalWrite(LED_PIN, HIGH);
                delay(400);  // Long blink for tens
                digitalWrite(LED_PIN, LOW);
                delay(200);
            }
            delay(500);  // Pause between tens and ones
        }
        
        // Blink ones digit (or full number if < 10)
        int blinkCount = (tens > 0) ? ones : errorNum;
        if (blinkCount == 0) blinkCount = 10;  // 0 = 10 blinks
        
        for (int i = 0; i < blinkCount; i++) {
            digitalWrite(LED_PIN, HIGH);
            delay(150);  // Short blink for ones
            digitalWrite(LED_PIN, LOW);
            delay(150);
        }
        
        // Long pause before repeating
        delay(2000);
    }
}

void displayError(ErrorCode code) {
    ErrorInfo info = getErrorInfo(code);
    
    // Print to Serial
    Serial.println(F("\n!!! ERROR !!!"));
    Serial.print(F("Code: E"));
    if (code < 10) Serial.print("0");
    Serial.println((int)code);
    Serial.print(F("Message: "));
    Serial.println(info.shortMsg);
    Serial.print(F("Detail: "));
    Serial.println(info.detailMsg);
    Serial.println();
    
    // Try to display on LCD
    showError(info.shortMsg, info.detailMsg);
}

void haltWithError(ErrorCode code) {
    ErrorInfo info = getErrorInfo(code);
    
    // Try LCD first
    displayError(code);
    
    // Flash display if LCD is working
    flashDisplay(5, 200);
    
    // Then blink LED pattern forever
    blinkErrorPattern((int)code);
    
    // Never returns
}
