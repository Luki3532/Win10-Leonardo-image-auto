/**
 * Windows Auto-Wipe Device
 * Arduino Leonardo + 16x2 I2C LCD
 * 
 * Target: Dell machines (F12 boot menu, 3rd boot option)
 * 
 * SAFETY: Press and HOLD the button for 3 seconds to arm and execute.
 *         Release early to abort. LCD shows countdown while holding.
 * 
 * WIRING:
 *   - Arm Button: GND <-> Pin 7 (momentary push button)
 *   - LCD SDA: Pin 2
 *   - LCD SCL: Pin 3  
 *   - LCD VCC: 5V
 *   - LCD GND: GND
 *   - Status LED: Pin 13
 */

#include <Arduino.h>
#include <Wire.h>
#include "../include/config.h"
#include "display.h"
#include "keyboard_utils.h"
#include "i2c_scanner.h"
#include "error_handler.h"

// ============================================
// State tracking
// ============================================
bool payloadExecuted = false;
bool lcdAvailable = false;

// ============================================
// LED Status Functions
// ============================================
void ledOn() {
    digitalWrite(LED_PIN, HIGH);
}

void ledOff() {
    digitalWrite(LED_PIN, LOW);
}

void blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        ledOn();
        delay(delayMs);
        ledOff();
        delay(delayMs);
    }
}

void slowBlink() {
    // Continuous slow blink for safe mode
    while (true) {
        ledOn();
        delay(1000);
        ledOff();
        delay(1000);
    }
}

void rapidErrorBlink() {
    // Very fast blink indicates error (when no LCD)
    while (true) {
        ledOn();
        delay(50);
        ledOff();
        delay(50);
    }
}

// ============================================
// Button Functions
// ============================================
bool isButtonPressed() {
    // Button connected between GND and pin 7 with INPUT_PULLUP
    // Pressed = LOW, Released = HIGH
    return digitalRead(ARM_BUTTON_PIN) == LOW;
}

// Wait for button to be released (with debounce)
void waitForButtonRelease() {
    while (isButtonPressed()) {
        delay(10);
    }
    delay(BUTTON_DEBOUNCE);
}

// Check if button is held for the required time
// Returns true if held long enough, false if released early
bool waitForArmHold() {
    unsigned long startTime = millis();
    int lastSecond = -1;
    
    LiquidCrystal_I2C& lcd = getLCD();
    
    while (isButtonPressed()) {
        unsigned long elapsed = millis() - startTime;
        int remaining = (ARM_HOLD_TIME - elapsed) / 1000 + 1;
        
        // Update countdown display
        if (remaining != lastSecond) {
            lastSecond = remaining;
            lcd.setCursor(0, 0);
            lcd.print("HOLD TO ARM:  ");
            lcd.print(remaining);
            lcd.print("s");
            lcd.setCursor(0, 1);
            lcd.print("Release=Cancel  ");
            
            // Blink LED with countdown
            ledOn();
            delay(100);
            ledOff();
            
            DEBUG_PRINT(F("Arming in: "));
            DEBUG_PRINTLN(remaining);
        }
        
        // Check if held long enough
        if (elapsed >= ARM_HOLD_TIME) {
            return true;  // Armed!
        }
        
        delay(50);
    }
    
    // Button was released early
    return false;
}

// ============================================
// Phase 1: Boot Menu
// ============================================
void executeBootMenuPhase() {
    showStatus("OPENING BIOS", "Spamming F12...");
    
    // Spam F12 immediately and continuously for 10 seconds
    LiquidCrystal_I2C& lcd = getLCD();
    unsigned long startTime = millis();
    int keyCount = 0;
    
    while (millis() - startTime < BOOT_SPAM_DURATION) {
        pressKey(BOOT_KEY);
        keyCount++;
        
        // Update LCD with countdown
        int remaining = (BOOT_SPAM_DURATION - (millis() - startTime)) / 1000;
        lcd.setCursor(13, 1);
        if (remaining < 10) lcd.print(" ");
        lcd.print(remaining);
        lcd.print("s");
    }
    
    DEBUG_PRINT(F("Sent F12 "));
    DEBUG_PRINT(keyCount);
    DEBUG_PRINTLN(F(" times"));
    
    // Wait for boot menu to appear
    showCountdown("BOOT MENU", "Waiting...", 3);
    
    // Navigate to 3rd option (DOWN twice) and select
    showStatus("BOOT MENU", "Selecting USB..");
    pressDownMultiple(BOOT_MENU_POSITION);
    delay(500);
    pressKey(KEY_RETURN);
    
    DEBUG_PRINTLN(F("Boot menu: Selected USB drive"));
}

// ============================================
// Phase 2: Wait for Windows Setup
// ============================================
void waitForWindowsSetup() {
    showCountdown("WAITING", "Win Setup..", WIN_SETUP_WAIT);
    DEBUG_PRINTLN(F("Windows Setup should be loaded"));
}

// ============================================
// Phase 3: Windows Setup Navigation
// ============================================
void executeWindowsSetup() {
    const int TOTAL_STEPS = 5;
    
    // Step 1: Language Selection - Just press Enter (accept defaults)
    showProgress(1, TOTAL_STEPS, "SETUP", "Language");
    delay(SCREEN_DELAY);
    pressKey(KEY_RETURN);  // Click "Next"
    
    // Step 2: Install Now
    showProgress(2, TOTAL_STEPS, "SETUP", "Install Now");
    delay(SCREEN_DELAY);
    pressKey(KEY_RETURN);  // Click "Install now"
    
    // Step 3: Product Key - Skip it
    showProgress(3, TOTAL_STEPS, "SETUP", "Skip Key");
    delay(SCREEN_DELAY);
    // Tab to "I don't have a product key" and press Enter
    pressKey(KEY_TAB);
    delay(200);
    pressKey(KEY_RETURN);
    
    // Step 4: License Terms - Accept
    showProgress(4, TOTAL_STEPS, "SETUP", "License");
    delay(SCREEN_DELAY);
    pressKey(' ');         // Check "I accept" (spacebar)
    delay(200);
    pressKey(KEY_TAB);     // Tab to Next
    delay(200);
    pressKey(KEY_RETURN);  // Click Next
    
    // Step 5: Installation Type - Select Custom
    showProgress(5, TOTAL_STEPS, "SETUP", "Custom Install");
    delay(SCREEN_DELAY);
    // "Custom: Install Windows only (advanced)" is the second option
    pressKey(KEY_TAB);     // Tab to Custom option
    delay(200);
    pressKey(KEY_RETURN);  // Select it
    
    DEBUG_PRINTLN(F("Windows Setup navigation complete"));
}

// ============================================
// Phase 4: Partition Deletion
// ============================================
void executePartitionWipe() {
    showStatus("WIPING DISK", "Deleting...");
    LiquidCrystal_I2C& lcd = getLCD();
    
    delay(SCREEN_DELAY);  // Wait for partition screen to load
    
    // Delete partitions repeatedly
    // The loop will try to delete partitions until there are none left
    // Windows won't let you delete the last "unallocated space"
    
    for (int attempt = 1; attempt <= DELETE_ATTEMPTS; attempt++) {
        lcd.setCursor(0, 1);
        lcd.print("Deleting... #");
        lcd.print(attempt);
        lcd.print("   ");
        
        DEBUG_PRINT(F("Delete attempt "));
        DEBUG_PRINTLN(attempt);
        
        // Select first partition in list (might already be selected)
        pressKey(KEY_DOWN_ARROW);
        delay(300);
        
        // Method 1: Try ALT+D (Delete shortcut)
        pressCombo(KEY_LEFT_ALT, 'd');
        delay(500);
        
        // Confirm deletion (press Enter on confirmation dialog)
        pressKey(KEY_RETURN);
        delay(PARTITION_DELAY);
        
        // Press Enter again in case there's another confirmation
        pressKey(KEY_RETURN);
        delay(PARTITION_DELAY);
    }
    
    DEBUG_PRINTLN(F("Partition deletion loop complete"));
}

// ============================================
// Phase 5: Start Installation
// ============================================
void startInstallation() {
    showStatus("STARTING", "Installing...");
    
    delay(SCREEN_DELAY);
    
    // At this point, we should have unallocated space
    // Select it and click Next
    
    // First, make sure we're on the unallocated space
    // (should be the only thing left, or we select it)
    pressKey(KEY_DOWN_ARROW);
    delay(300);
    
    // Tab to "Next" button
    for (int i = 0; i < 5; i++) {
        pressKey(KEY_TAB);
        delay(100);
    }
    
    // Press Enter to start installation
    pressKey(KEY_RETURN);
    
    delay(2000);  // Wait a moment
    
    // Press Enter again in case there's a confirmation
    pressKey(KEY_RETURN);
    
    DEBUG_PRINTLN(F("Installation started!"));
}

// ============================================
// Main Payload Execution
// ============================================
void executePayload() {
    DEBUG_PRINTLN(F("\n========================================"));
    DEBUG_PRINTLN(F("  WINDOWS AUTO-WIPE PAYLOAD STARTING"));
    DEBUG_PRINTLN(F("========================================\n"));
    
    // Flash display to indicate starting
    if (lcdAvailable) flashDisplay(3, 200);
    
    showStatus("\x03 ARMED \x03", "Executing...");
    blinkLED(5, 100);  // Fast blink = running
    
    // Initialize keyboard HID
    initKeyboard();
    
    // Verify keyboard initialized (Leonardo should always work)
    // Note: There's no easy way to test this, but we log it
    DEBUG_PRINTLN(F("Keyboard HID initialized"));
    if (lcdAvailable) {
        showStatus("HID KEYBOARD", "Initialized OK");
        delay(500);
    }
    
    // PHASE 1: Boot Menu
    // NO DELAY - Start immediately to catch BIOS POST
    showStatus("PHASE 1/5", "Boot Menu...");
    delay(300);
    executeBootMenuPhase();
    
    // PHASE 2: Wait for Windows Setup to load
    showStatus("PHASE 2/5", "Waiting...");
    delay(300);
    waitForWindowsSetup();
    
    // PHASE 3: Navigate Windows Setup screens
    showStatus("PHASE 3/5", "Win Setup Nav");
    delay(300);
    executeWindowsSetup();
    
    // PHASE 4: Delete all partitions
    showStatus("PHASE 4/5", "Wiping Disk...");
    delay(300);
    executePartitionWipe();
    
    // PHASE 5: Start installation
    showStatus("PHASE 5/5", "Installing...");
    delay(300);
    startInstallation();
    
    // COMPLETE!
    showComplete();
    ledOn();  // Solid LED = complete
    if (lcdAvailable) flashDisplay(5, 300);
    
    DEBUG_PRINTLN(F("\n========================================"));
    DEBUG_PRINTLN(F("  PAYLOAD COMPLETE - WINDOWS INSTALLING"));
    DEBUG_PRINTLN(F("========================================\n"));
    
    payloadExecuted = true;
}

// ============================================
// I2C Scanner Mode
// ============================================
void runScannerMode() {
    // Initialize LED first for visual feedback
    pinMode(LED_PIN, OUTPUT);
    
    // Blink LED to show scanner mode is active
    for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
    
    Serial.begin(SERIAL_BAUD_RATE);
    
    // Wait for serial connection (Leonardo needs this)
    while (!Serial) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(100);
    }
    delay(500);  // Extra delay for terminal to stabilize
    
    Serial.println(F("\n\n"));
    Serial.println(F("================================"));
    Serial.println(F("   I2C SCANNER MODE ACTIVE"));
    Serial.println(F("================================"));
    Serial.println();
    
    // Initialize I2C
    Wire.begin();
    
    // Scan for devices
    Serial.println(F("Scanning I2C bus..."));
    Serial.println();
    
    int foundCount = 0;
    uint8_t foundAddr = 0;
    
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print(F("  >> FOUND device at 0x"));
            if (addr < 16) Serial.print("0");
            Serial.print(addr, HEX);
            
            if (addr >= 0x20 && addr <= 0x27) {
                Serial.print(F("  (PCF8574 - LCD)"));
            } else if (addr >= 0x38 && addr <= 0x3F) {
                Serial.print(F("  (PCF8574A - LCD)"));
            }
            Serial.println();
            
            if (foundAddr == 0) foundAddr = addr;
            foundCount++;
        }
    }
    
    Serial.println();
    Serial.println(F("================================"));
    Serial.print(F("Scan complete. Found "));
    Serial.print(foundCount);
    Serial.println(F(" device(s)."));
    
    if (foundAddr != 0) {
        Serial.println();
        Serial.println(F("*** UPDATE config.h with: ***"));
        Serial.print(F("#define LCD_ADDRESS  0x"));
        if (foundAddr < 16) Serial.print("0");
        Serial.println(foundAddr, HEX);
        
        // Try to display on LCD
        LiquidCrystal_I2C lcd(foundAddr, 16, 2);
        lcd.init();
        lcd.backlight();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Found: 0x");
        lcd.print(foundAddr, HEX);
        lcd.setCursor(0, 1);
        lcd.print("Adjust contrast!");
        
        Serial.println();
        Serial.println(F("LCD initialized. If blank, adjust contrast potentiometer!"));
    } else {
        Serial.println();
        Serial.println(F("!!! NO I2C DEVICES FOUND !!!"));
        Serial.println();
        Serial.println(F("Check wiring:"));
        Serial.println(F("  LCD SDA --> Arduino Pin 2"));
        Serial.println(F("  LCD SCL --> Arduino Pin 3"));
        Serial.println(F("  LCD VCC --> Arduino 5V"));
        Serial.println(F("  LCD GND --> Arduino GND"));
    }
    Serial.println(F("================================"));
    
    // Blink LED to show we're done
    while (true) {
        digitalWrite(LED_PIN, HIGH);
        delay(foundAddr ? 1000 : 200);  // Slow=found, Fast=not found
        digitalWrite(LED_PIN, LOW);
        delay(foundAddr ? 1000 : 200);
    }
}

// ============================================
// Setup
// ============================================
void setup() {
    // Initialize pins FIRST
    pinMode(ARM_BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    ledOff();
    
    // Initialize serial for debugging
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);  // Brief delay for serial
    
    Serial.println(F("\n===================================="));
    Serial.println(F(" WINDOWS AUTO-WIPE DEVICE"));
    #if DEMO_MODE
    Serial.println(F("    *** DEMO MODE ACTIVE ***"));
    Serial.println(F("  (No keystrokes will be sent)"));
    #endif
    Serial.println(F("====================================\n"));
    
    // Check for I2C scan mode
    #if I2C_SCAN_MODE
        runScannerMode();
        return;  // Never reaches here
    #endif
    
    // ==========================================
    // HARDWARE CHECKS
    // ==========================================
    Serial.println(F("Running hardware checks..."));
    
    // Check 1: Try to initialize display
    lcdAvailable = initDisplay();
    
    if (!lcdAvailable) {
        // LCD not found - check if it's wiring or wrong address
        Serial.println(F("LCD NOT FOUND!"));
        Serial.println(F("Checking I2C bus..."));
        
        Wire.begin();
        bool foundAny = false;
        uint8_t foundAddr = 0;
        
        for (uint8_t addr = 0x20; addr < 0x40; addr++) {
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                foundAny = true;
                foundAddr = addr;
                Serial.print(F("  Found device at 0x"));
                Serial.println(addr, HEX);
            }
        }
        
        if (!foundAny) {
            // No I2C devices at all - wiring issue
            Serial.println(F("\nERROR E01: LCD NOT CONNECTED"));
            Serial.println(F("Check wiring:"));
            Serial.println(F("  SDA -> Pin 2"));
            Serial.println(F("  SCL -> Pin 3"));
            Serial.println(F("  VCC -> 5V"));
            Serial.println(F("  GND -> GND"));
            Serial.println(F("\nLED will blink: 1 long flash"));
            blinkErrorPattern(1);  // Never returns
        } else {
            // Found device but wrong address
            Serial.println(F("\nERROR E02: WRONG LCD ADDRESS"));
            Serial.print(F("Found LCD at 0x"));
            Serial.print(foundAddr, HEX);
            Serial.print(F(" but config.h says 0x"));
            Serial.println(LCD_ADDRESS, HEX);
            Serial.println(F("\nUpdate LCD_ADDRESS in config.h!"));
            Serial.println(F("\nLED will blink: 2 long flashes"));
            blinkErrorPattern(2);  // Never returns
        }
    }
    
    Serial.println(F("  LCD: OK"));
    
    // Show startup message on LCD
    showStatus("AUTO-WIPE v1.0", "Checking HW...");
    delay(500);
    
    // Check 2: Button wiring
    ErrorCode buttonErr = checkSwitchWiring();
    if (buttonErr != ERR_NONE) {
        Serial.println(F("  Button: FLOATING!"));
        haltWithError(buttonErr);  // Never returns
    }
    Serial.println(F("  Button: OK"));
    
    // Update LCD with hardware check result
    #if DEMO_MODE
    showStatus("** DEMO MODE **", "No keys sent!");
    delay(1500);
    #endif
    showStatus("HW CHECK: OK", "Ready!");
    delay(500);
    
    Serial.println(F("Hardware checks passed!\n"));
    
    // ==========================================
    // READY - WAIT FOR BUTTON PRESS
    // ==========================================
    #if DEMO_MODE
    showStatus("DEMO - READY", "Press btn 3s...");
    #else
    showStatus("READY", "Press btn 3s...");
    #endif
    Serial.println(F("Waiting for button press..."));
    Serial.println(F("Hold button for 3 seconds to arm and execute."));
    
    // Main ready loop - wait for button press
    bool ledState = false;
    unsigned long lastBlink = 0;
    
    while (!payloadExecuted) {
        // Slow blink LED while waiting
        if (millis() - lastBlink > 500) {
            lastBlink = millis();
            ledState = !ledState;
            digitalWrite(LED_PIN, ledState);
        }
        
        // Check for button press
        if (isButtonPressed()) {
            delay(BUTTON_DEBOUNCE);  // Debounce
            
            if (isButtonPressed()) {  // Still pressed after debounce
                Serial.println(F("Button pressed - starting arm countdown..."));
                
                // Check if held long enough
                if (waitForArmHold()) {
                    // ARMED! Execute payload
                    Serial.println(F("\n*** ARMED! Executing payload... ***\n"));
                    showStatus("\x03 ARMED! \x03", "EXECUTING...");
                    blinkLED(5, 100);
                    
                    executePayload();
                } else {
                    // Released early - cancelled
                    Serial.println(F("Cancelled - button released early"));
                    showStatus("CANCELLED", "Press btn 3s...");
                    delay(1000);
                    showStatus("READY", "Press btn 3s...");
                }
            }
        }
        
        delay(10);
    }
}

// ============================================
// Loop
// ============================================
void loop() {
    // Payload runs in setup() after button arm
    // Just keep the LED on to show completion
    if (payloadExecuted) {
        ledOn();
        delay(1000);
    }
}
