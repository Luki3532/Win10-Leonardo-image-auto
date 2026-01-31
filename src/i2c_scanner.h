/**
 * I2C Address Scanner
 * 
 * Scans the I2C bus to find connected devices.
 * Used to find the LCD address (usually 0x27 or 0x3F)
 */

#ifndef I2C_SCANNER_H
#define I2C_SCANNER_H

#include <Arduino.h>

/**
 * Scan I2C bus and print found addresses to Serial.
 * Also displays results on LCD if an address is found.
 * 
 * @return The first found I2C address, or 0 if none found
 */
uint8_t scanI2C();

#endif // I2C_SCANNER_H
