/**
 * Windows Auto-Wipe Device Configuration
 * Arduino Leonardo + QAPASS 16x2 LCD + HW-061 I2C Backpack
 * 
 * Target: Dell machines (F12 boot menu)
 */

#ifndef CONFIG_H
#define CONFIG_H

// ===========================================
// MODE SELECTION
// ===========================================
// Set to 1 to scan for I2C address, 0 for normal operation
#define I2C_SCAN_MODE       0

// DEMO MODE: Set to 1 to simulate without sending keystrokes
// Shows all actions on LCD/Serial but keyboard is disabled
#define DEMO_MODE           0

// ===========================================
// Hardware Pins
// ===========================================
#define ARM_BUTTON_PIN      7       // Arm button (INPUT_PULLUP, press to GND)
#define LED_PIN             13      // Status LED

// ===========================================
// Button Configuration
// ===========================================
#define ARM_HOLD_TIME       3000    // Hold button for 3 seconds to arm
#define BUTTON_DEBOUNCE     50      // Debounce delay in ms

// ===========================================
// I2C LCD Configuration (HW-061 Backpack)
// ===========================================
// Common addresses: 0x27 or 0x3F
// Use I2C_SCAN_MODE to find yours
#define LCD_ADDRESS         0x3F
#define LCD_COLS            16
#define LCD_ROWS            2

// ===========================================
// Dell BIOS Configuration
// ===========================================
#define BOOT_KEY            KEY_F12     // Dell boot menu key
#define BOOT_MENU_POSITION  2           // 3rd option (0-indexed: 2 = DOWN twice)

// ===========================================
// Timing Configuration (milliseconds)
// ===========================================
#define KEY_DELAY           100         // Delay between keystrokes
#define KEY_HOLD_DELAY      50          // How long to hold a key
#define SCREEN_DELAY        3000        // Wait between screens (3 seconds)
#define BOOT_SPAM_DURATION  10000       // Spam F12 for 10 seconds
#define BOOT_SPAM_INTERVAL  100         // F12 press interval during spam
#define BOOT_MENU_WAIT      3000        // Wait for boot menu to appear
#define WIN_SETUP_WAIT      45          // Seconds to wait for Windows Setup
#define PARTITION_DELAY     1500        // Delay after partition operations
#define DELETE_ATTEMPTS     10          // Max partition delete attempts

// ===========================================
// Serial Configuration
// ===========================================
#define SERIAL_BAUD_RATE    115200

// ===========================================
// Debug Macros
// ===========================================
#ifdef DEBUG
    #define DEBUG_PRINT(x)      Serial.print(x)
    #define DEBUG_PRINTLN(x)    Serial.println(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
#endif

// ===========================================
// ERROR CODES REFERENCE
// ===========================================
// E01: LCD not found - check I2C wiring (SDA->2, SCL->3, VCC->5V, GND)
// E02: LCD wrong address - found LCD but at different address, update config
// E03: I2C bus error - no devices found on I2C bus at all
// E04: USB/HID keyboard init failed
// E10: Button pin floating - check button wiring between GND and pin 7
// E11: No pullup on button pin
// E20: Boot menu timeout - BIOS didn't respond to F12
// E21: Windows Setup timeout - Setup screen didn't load
// E22: Partition wipe failed
// E23: Installation didn't start
// E99: Unknown error
//
// LED ERROR PATTERNS:
// - 1 long blink = E01 (LCD not connected)
// - 2 long blinks = E02 (wrong LCD address)
// - For E10+: blinks = tens digit (long), then ones digit (short)
// - Example: E22 = 2 long blinks, pause, 2 short blinks

#endif // CONFIG_H
