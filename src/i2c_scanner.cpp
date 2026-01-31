/**
 * I2C Address Scanner Implementation
 */

#include "i2c_scanner.h"
#include <Wire.h>

uint8_t scanI2C() {
    Serial.println(F("\n================================"));
    Serial.println(F("I2C Scanner - Finding LCD Address"));
    Serial.println(F("================================\n"));
    
    Wire.begin();
    
    uint8_t foundAddress = 0;
    int deviceCount = 0;
    
    Serial.println(F("Scanning addresses 0x01 to 0x7F...\n"));
    
    for (uint8_t address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print(F("  >> FOUND device at address 0x"));
            if (address < 16) Serial.print("0");
            Serial.print(address, HEX);
            
            // Identify common devices
            if (address >= 0x20 && address <= 0x27) {
                Serial.print(F("  (PCF8574 - LCD backpack)"));
            } else if (address >= 0x38 && address <= 0x3F) {
                Serial.print(F("  (PCF8574A - LCD backpack)"));
            }
            Serial.println();
            
            if (foundAddress == 0) {
                foundAddress = address;
            }
            deviceCount++;
        }
    }
    
    Serial.println();
    Serial.println(F("================================"));
    Serial.print(F("Scan complete. Found "));
    Serial.print(deviceCount);
    Serial.println(F(" device(s)."));
    
    if (foundAddress != 0) {
        Serial.println();
        Serial.println(F("*** UPDATE config.h with: ***"));
        Serial.print(F("    #define LCD_ADDRESS  0x"));
        if (foundAddress < 16) Serial.print("0");
        Serial.println(foundAddress, HEX);
        Serial.println();
    } else {
        Serial.println(F("\nNo I2C devices found!"));
        Serial.println(F("Check wiring:"));
        Serial.println(F("  SDA -> Pin 2"));
        Serial.println(F("  SCL -> Pin 3"));
        Serial.println(F("  VCC -> 5V"));
        Serial.println(F("  GND -> GND"));
    }
    Serial.println(F("================================\n"));
    
    return foundAddress;
}
