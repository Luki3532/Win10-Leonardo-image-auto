/**
 * BIOS Password Removal / Windows 10 Install Device
 * Arduino Leonardo + 16x2 I2C LCD
 * 
 * DUAL MODE OPERATION:
 *   D7 only removed  -> BIOS Password Removal (types ls3gt1)
 *   D7 AND D10 removed -> Windows 10 Clean Install
 * 
 * SAFETY WIRES:
 *   - D7 to GND: Primary safety (must remove to do anything)
 *   - D10 to GND: Mode select (remove for Win10 install, keep for BIOS password)
 * 
 * WIRING:
 *   - Safety Wire 1: Pin 7 <-> GND (remove to arm)
 *   - Safety Wire 2: Pin 10 <-> GND (remove for Win10 mode)
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
// Safety Wire Pins
// ============================================
#define SAFETY_PIN_1        7       // Primary safety wire (D7)
#define SAFETY_PIN_2        10      // Secondary - mode select (D10)

// ============================================
// Safety Wire Check Functions
// ============================================
// Wire connected to GND = LOW = SAFE
// Wire removed = HIGH (pullup) = ARMED

bool isSafety1Off() {
    return digitalRead(SAFETY_PIN_1) == HIGH;  // D7 removed = armed
}

bool isSafety2Off() {
    return digitalRead(SAFETY_PIN_2) == HIGH;  // D10 removed = Win10 mode
}

// Check if device should execute at all (D7 must be removed)
bool isSafetyOff() {
    return isSafety1Off();
}

// Check if Win10 install mode (both D7 AND D10 removed)
bool isWin10Mode() {
    return isSafety1Off() && isSafety2Off();
}

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

/* ============================================
 * WINDOWS 10 INSTALLER PHASES - DISABLED
 * Uncomment these sections to enable Windows 10 installation functionality
 * ============================================

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
// Phase 6: OOBE (Out-of-Box Experience) Setup
// ============================================
void executeOOBESetup() {
    // Wait for Windows installation to complete and OOBE to start
    showCountdown("INSTALLING", "Wait for OOBE", OOBE_WAIT_TIME);
    
    const int TOTAL_STEPS = 8;
    
    // Step 1: Region selection - accept default, click Yes
    showProgress(1, TOTAL_STEPS, "OOBE", "Region");
    delay(OOBE_SCREEN_DELAY);
    pressKey(KEY_RETURN);  // Yes/Next
    
    // Step 2: Keyboard layout - accept default, click Yes
    showProgress(2, TOTAL_STEPS, "OOBE", "Keyboard");
    delay(OOBE_SCREEN_DELAY);
    pressKey(KEY_RETURN);  // Yes
    
    // Step 3: Second keyboard layout - Skip
    showProgress(3, TOTAL_STEPS, "OOBE", "Skip 2nd KB");
    delay(OOBE_SCREEN_DELAY);
    pressKey(KEY_TAB);     // Tab to Skip
    delay(200);
    pressKey(KEY_RETURN);  // Skip
    
    // Step 4: Network - Skip (for offline setup)
    showProgress(4, TOTAL_STEPS, "OOBE", "Skip Network");
    delay(OOBE_SCREEN_DELAY);
    // Press "I don't have internet" or skip
    pressKey(KEY_TAB);
    delay(200);
    pressKey(KEY_RETURN);
    delay(OOBE_SCREEN_DELAY);
    // "Continue with limited setup"
    pressKey(KEY_TAB);
    delay(200);
    pressKey(KEY_RETURN);
    
    // Step 5: Enter username (use "Admin" or similar)
    showProgress(5, TOTAL_STEPS, "OOBE", "Username");
    delay(OOBE_SCREEN_DELAY);
    typeString("Admin");
    delay(200);
    pressKey(KEY_RETURN);  // Next
    
    // Step 6: Password - leave blank (no password)
    showProgress(6, TOTAL_STEPS, "OOBE", "Skip Password");
    delay(OOBE_SCREEN_DELAY);
    // Don't type anything - just press Next for blank password
    pressKey(KEY_RETURN);  // Next (blank password)
    
    // No password = no confirm screen, no security questions
    // Windows skips those steps when password is blank
    
    // Privacy settings - just accept defaults and click Accept
    delay(OOBE_SCREEN_DELAY * 2);  // Extra wait for privacy screen
    showStatus("OOBE", "Privacy...");
    
    // Tab through privacy toggles and click Accept
    for (int i = 0; i < 6; i++) {
        pressKey(KEY_TAB);
        delay(100);
    }
    pressKey(KEY_RETURN);  // Accept
    
    DEBUG_PRINTLN(F("OOBE setup complete!"));
}

* ============================================ */

/* ============================================
 * WINDOWS 10 INSTALLER PAYLOAD - DISABLED
 * Uncomment this section to enable Windows 10 installation functionality
 * ============================================
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
    showStatus("PHASE 1/6", "Boot Menu...");
    delay(300);
    executeBootMenuPhase();
    
    // PHASE 2: Wait for Windows Setup to load
    showStatus("PHASE 2/6", "Waiting...");
    delay(300);
    waitForWindowsSetup();
    
    // PHASE 3: Navigate Windows Setup screens
    showStatus("PHASE 3/6", "Win Setup Nav");
    delay(300);
    executeWindowsSetup();
    
    // PHASE 4: Delete all partitions
    showStatus("PHASE 4/6", "Wiping Disk...");
    delay(300);
    executePartitionWipe();
    
    // PHASE 5: Start installation
    showStatus("PHASE 5/6", "Installing...");
    delay(300);
    startInstallation();
    
    // PHASE 6: OOBE Setup (after Windows installs)
    showStatus("PHASE 6/6", "OOBE Setup...");
    delay(300);
    executeOOBESetup();
    
    // COMPLETE!
    showComplete();
    ledOn();  // Solid LED = complete
    if (lcdAvailable) flashDisplay(5, 300);
    
    DEBUG_PRINTLN(F("\n========================================"));
    DEBUG_PRINTLN(F("  PAYLOAD COMPLETE - WINDOWS INSTALLING"));
    DEBUG_PRINTLN(F("========================================\n"));
    
    payloadExecuted = true;
}
* ============================================ */

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
// BIOS Admin Password Removal Payload
// Dell BIOS Navigation sequence (user-specified)
// ============================================

// Dynamic DOWN adjustment function
// Waits for initial window, then adds DOWNs when D7 is touched
// Each touch: press DOWN once and wait another 5 seconds
// If no touch for the wait period, proceed
// Returns total number of extra DOWNs pressed
int dynamicDownAdjustment(int initialWaitSec, int touchWaitSec, const char* title) {
    const unsigned long INITIAL_WAIT = initialWaitSec * 1000UL;  // Initial wait (10 sec)
    const unsigned long TOUCH_WAIT = touchWaitSec * 1000UL;      // Wait after each touch (5 sec)
    
    unsigned long windowStart = millis();
    unsigned long currentWait = INITIAL_WAIT;
    int extraDowns = 0;
    bool wasConnected = false;
    
    if (lcdAvailable) {
        LiquidCrystal_I2C& lcd = getLCD();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(title);
        lcd.setCursor(0, 1);
        lcd.print("Touch D7    ");
        lcd.print(initialWaitSec);
        lcd.print("s");
    }
    DEBUG_PRINT(F("Dynamic adjustment window: "));
    DEBUG_PRINTLN(title);
    
    while (true) {
        unsigned long elapsed = millis() - windowStart;
        int remaining = (currentWait - elapsed) / 1000;
        
        // Time's up - no touch detected in time
        if (elapsed >= currentWait) {
            DEBUG_PRINTLN(F("Adjustment window closed - proceeding"));
            break;
        }
        
        // Check if D7 is touched to GND (LOW = connected)
        bool isConnected = (digitalRead(SAFETY_PIN_1) == LOW);
        
        // Edge detection: register when wire connects to GND
        if (isConnected && !wasConnected) {
            // Touch detected! Press DOWN and reset timer
            extraDowns++;
            
            DEBUG_PRINT(F("Touch detected! Pressing DOWN #"));
            DEBUG_PRINTLN(extraDowns);
            
            // Visual feedback
            ledOn();
            
            // Press DOWN arrow
            pressKey(KEY_DOWN_ARROW);
            delay(200);
            
            ledOff();
            
            // Update LCD
            if (lcdAvailable) {
                LiquidCrystal_I2C& lcd = getLCD();
                lcd.setCursor(0, 1);
                lcd.print("+");
                lcd.print(extraDowns);
                lcd.print(" DOWN   ");
                lcd.print(touchWaitSec);
                lcd.print("s");
            }
            
            // Reset timer for another wait period
            windowStart = millis();
            currentWait = TOUCH_WAIT;
        }
        wasConnected = isConnected;
        
        // Update countdown on LCD
        if (lcdAvailable) {
            LiquidCrystal_I2C& lcd = getLCD();
            lcd.setCursor(12, 1);
            if (remaining < 10) lcd.print(" ");
            lcd.print(remaining);
            lcd.print("s");
        }
        
        delay(50);  // Poll every 50ms
    }
    
    // Window complete - show result briefly
    if (lcdAvailable) {
        LiquidCrystal_I2C& lcd = getLCD();
        lcd.setCursor(0, 1);
        lcd.print("Done: +");
        lcd.print(extraDowns);
        lcd.print(" DOWNs  ");
    }
    DEBUG_PRINT(F("Dynamic adjustment done. Extra DOWNs: "));
    DEBUG_PRINTLN(extraDowns);
    delay(500);
    
    return extraDowns;
}

void executeBIOSPasswordRemoval() {
    DEBUG_PRINTLN(F("\n========================================"));
    DEBUG_PRINTLN(F("  DELL BIOS PASSWORD REMOVAL STARTING"));
    DEBUG_PRINTLN(F("========================================\n"));
    
    // Initialize keyboard HID immediately
    initKeyboard();
    
    // ==========================================
    // PHASE 1: Spam F2 to enter BIOS Setup
    // ==========================================
    if (lcdAvailable) {
        showStatus("ENTERING BIOS", "Spamming F2...");
    }
    DEBUG_PRINTLN(F("Spamming F2 to enter BIOS Setup..."));
    
    unsigned long startTime = millis();
    int keyCount = 0;
    
    // Spam F2 for 10 seconds to catch BIOS POST
    while (millis() - startTime < BOOT_SPAM_DURATION) {
        pressKey(KEY_F2);  // F2 for Dell BIOS Setup
        keyCount++;
        
        // Update LCD with countdown if available
        if (lcdAvailable) {
            LiquidCrystal_I2C& lcd = getLCD();
            int remaining = (BOOT_SPAM_DURATION - (millis() - startTime)) / 1000;
            lcd.setCursor(13, 1);
            if (remaining < 10) lcd.print(" ");
            lcd.print(remaining);
            lcd.print("s");
        }
    }
    
    DEBUG_PRINT(F("Sent F2 "));
    DEBUG_PRINT(keyCount);
    DEBUG_PRINTLN(F(" times"));
    
    // ==========================================
    // PHASE 2: Wait for BIOS to fully load
    // ==========================================
    if (lcdAvailable) {
        showStatus("BIOS LOADING", "Waiting...");
    }
    DEBUG_PRINTLN(F("Waiting for BIOS to load..."));
    
    // Countdown for 5 seconds
    for (int i = 5; i > 0; i--) {
        if (lcdAvailable) {
            LiquidCrystal_I2C& lcd = getLCD();
            lcd.setCursor(14, 1);
            lcd.print(i);
            lcd.print("s");
        }
        delay(1000);
    }
    
    // ==========================================
    // PHASE 3: Initial navigation - Down 5 times
    // ==========================================
    if (lcdAvailable) {
        showStatus("NAVIGATING", "Down 5...");
    }
    DEBUG_PRINTLN(F("Navigating BIOS - Down 5 times"));
    
    for (int i = 0; i < 5; i++) {
        pressKey(KEY_DOWN_ARROW);
        delay(300);
        
        if (lcdAvailable) {
            LiquidCrystal_I2C& lcd = getLCD();
            lcd.setCursor(11, 1);
            lcd.print(i + 1);
            lcd.print("/5");
        }
    }
    delay(300);
    
    // ==========================================
    // PHASE 4: Dynamic adjustment window
    // Wait 10s, touch D7 to add DOWN + 5s more
    // ==========================================
    int extraDowns = dynamicDownAdjustment(10, 5, "BIOS ADJUST");
    DEBUG_PRINT(F("Total extra DOWNs from adjustment: "));
    DEBUG_PRINTLN(extraDowns);
    
    // ==========================================
    // PHASE 5: Continue BIOS navigation
    // Enter, Down 1, Tab, Enter
    // ==========================================
    if (lcdAvailable) {
        showStatus("BIOS NAV", "Selecting...");
    }
    
    // Enter
    pressKey(KEY_RETURN);
    delay(500);
    
    // Down once
    pressKey(KEY_DOWN_ARROW);
    delay(300);
    
    // Tab
    pressKey(KEY_TAB);
    delay(300);
    
    // Enter
    pressKey(KEY_RETURN);
    delay(500);
    
    // ==========================================
    // PHASE 6: Enter OLD password
    // Type ls3gt1, Tab, Enter
    // ==========================================
    if (lcdAvailable) {
        showStatus("OLD PASSWORD", "Typing...");
    }
    DEBUG_PRINTLN(F("Entering old password: ls3gt1"));
    
    typeString("ls3gt1");
    delay(200);
    
    // Tab
    pressKey(KEY_TAB);
    delay(300);
    
    // Enter
    pressKey(KEY_RETURN);
    delay(500);
    
    // ==========================================
    // PHASE 5: Confirm/Clear password
    // Tab, ls3gt1, Tab 3 times, Enter
    // ==========================================
    if (lcdAvailable) {
        showStatus("CONFIRMING", "Password...");
    }
    DEBUG_PRINTLN(F("Confirming password change..."));
    
    // Tab
    pressKey(KEY_TAB);
    delay(300);
    
    // Type ls3gt1 again
    typeString("ls3gt1");
    delay(200);
    
    // Tab 3 times
    for (int i = 0; i < 3; i++) {
        pressKey(KEY_TAB);
        delay(300);
    }
    
    // Enter
    pressKey(KEY_RETURN);
    delay(500);
    
    // ==========================================
    // PHASE 6: Final confirmation
    // Tab 2 times, Enter
    // ==========================================
    if (lcdAvailable) {
        showStatus("SAVING", "Confirming...");
    }
    DEBUG_PRINTLN(F("Final confirmation..."));
    
    // Tab 2 times
    for (int i = 0; i < 2; i++) {
        pressKey(KEY_TAB);
        delay(300);
    }
    
    // Enter
    pressKey(KEY_RETURN);
    delay(500);
    
    // ==========================================
    // COMPLETE
    // ==========================================
    if (lcdAvailable) {
        showStatus("PASS REMOVED!", "Rebooting...");
    }
    
    DEBUG_PRINTLN(F("\n========================================"));
    DEBUG_PRINTLN(F("  BIOS PASSWORD REMOVAL COMPLETE"));
    DEBUG_PRINTLN(F("  System should reboot with no password"));
    DEBUG_PRINTLN(F("========================================\n"));
    
    payloadExecuted = true;
}

// ============================================
// Windows 10 Clean Install Payload
// Sequence: F12(10s) -> Down5 -> Enter -> Wait10s -> Tab3 -> Enter2 -> Wait15s -> Space -> Enter -> Down -> Enter -> Delete partitions
// ============================================
void executeWindows10Install() {
    DEBUG_PRINTLN(F("\n========================================"));
    DEBUG_PRINTLN(F("  WINDOWS 10 CLEAN INSTALL STARTING"));
    DEBUG_PRINTLN(F("========================================\n"));
    
    // Initialize keyboard HID immediately
    initKeyboard();
    
    // ==========================================
    // STEP 1: Spam F12 for 10 seconds
    // ==========================================
    if (lcdAvailable) {
        showStatus("BOOT MENU", "Spamming F12...");
    }
    DEBUG_PRINTLN(F("Spamming F12 for 10 seconds..."));
    
    unsigned long startTime = millis();
    int keyCount = 0;
    
    while (millis() - startTime < BOOT_SPAM_DURATION) {
        pressKey(KEY_F12);
        keyCount++;
        
        if (lcdAvailable) {
            LiquidCrystal_I2C& lcd = getLCD();
            int remaining = (BOOT_SPAM_DURATION - (millis() - startTime)) / 1000;
            lcd.setCursor(13, 1);
            if (remaining < 10) lcd.print(" ");
            lcd.print(remaining);
            lcd.print("s");
        }
    }
    
    DEBUG_PRINT(F("Sent F12 "));
    DEBUG_PRINT(keyCount);
    DEBUG_PRINTLN(F(" times"));
    
    // ==========================================
    // STEP 2: Down 1 time (initial position)
    // ==========================================
    if (lcdAvailable) {
        showStatus("BOOT MENU", "Down 1...");
    }
    DEBUG_PRINTLN(F("Down 1 time..."));
    
    pressKey(KEY_DOWN_ARROW);
    delay(300);
    
    // ==========================================
    // STEP 3: Dynamic adjustment window for USB
    // Wait 10s, touch D7 to add DOWN + 5s more
    // USB position varies so this allows dynamic selection
    // ==========================================
    int extraDowns = dynamicDownAdjustment(10, 5, "USB ADJUST");
    DEBUG_PRINT(F("Total extra DOWNs from adjustment: "));
    DEBUG_PRINTLN(extraDowns);
    
    // ==========================================
    // STEP 4: Enter to select boot device
    // ==========================================
    if (lcdAvailable) {
        showStatus("BOOT MENU", "Selecting...");
    }
    DEBUG_PRINTLN(F("Enter to select..."));
    pressKey(KEY_RETURN);
    
    // ==========================================
    // STEP 4: Wait 30 seconds
    // ==========================================
    if (lcdAvailable) {
        showStatus("LOADING", "Win Setup...");
    }
    DEBUG_PRINTLN(F("Waiting 30 seconds..."));
    
    for (int i = 30; i > 0; i--) {
        if (lcdAvailable) {
            LiquidCrystal_I2C& lcd = getLCD();
            lcd.setCursor(13, 1);
            if (i < 10) lcd.print(" ");
            lcd.print(i);
            lcd.print("s");
        }
        delay(1000);
    }
    
    // ==========================================
    // STEP 5: Tab 3 times
    // ==========================================
    if (lcdAvailable) {
        showStatus("SETUP", "Tab 3...");
    }
    DEBUG_PRINTLN(F("Tab 3 times..."));
    
    for (int i = 0; i < 3; i++) {
        pressKey(KEY_TAB);
        delay(200);
    }
    
    // ==========================================
    // STEP 6: Enter 2 times
    // ==========================================
    if (lcdAvailable) {
        showStatus("SETUP", "Enter 2...");
    }
    DEBUG_PRINTLN(F("Enter 2 times..."));
    
    pressKey(KEY_RETURN);
    delay(300);
    pressKey(KEY_RETURN);
    
    // ==========================================
    // STEP 7: Wait 30 seconds
    // ==========================================
    if (lcdAvailable) {
        showStatus("SETUP", "Waiting...");
    }
    DEBUG_PRINTLN(F("Waiting 30 seconds..."));
    
    for (int i = 30; i > 0; i--) {
        if (lcdAvailable) {
            LiquidCrystal_I2C& lcd = getLCD();
            lcd.setCursor(13, 1);
            if (i < 10) lcd.print(" ");
            lcd.print(i);
            lcd.print("s");
        }
        delay(1000);
    }
    
    // ==========================================
    // STEP 8: Space, Enter, Down, Enter
    // ==========================================
    if (lcdAvailable) {
        showStatus("SETUP", "License...");
    }
    DEBUG_PRINTLN(F("Space, Enter, Down, Enter..."));
    
    // Space
    pressKey(' ');
    delay(300);
    
    // Enter
    pressKey(KEY_RETURN);
    delay(300);
    
    // Down
    pressKey(KEY_DOWN_ARROW);
    delay(300);
    
    // Enter
    pressKey(KEY_RETURN);
    delay(2000);  // Wait for partition screen to load
    
    // ==========================================
    // STEP 9: Delete ALL Partitions - SMART ALGORITHM
    // Windows Setup: Select partition, RIGHT to Delete link, Enter, confirm
    // Strategy: Sweep up and down through the list, trying to delete at each position
    // ==========================================
    if (lcdAvailable) {
        showStatus("WIPING DISK", "Smart delete...");
    }
    DEBUG_PRINTLN(F("Starting smart partition deletion..."));
    
    delay(2000);  // Wait for partition list to fully populate
    
    const int MAX_SWEEPS = 4;         // Number of up/down sweeps
    int totalAttempts = 0;
    
    // First, go to top of list
    for (int i = 0; i < 10; i++) {
        pressKey(KEY_UP_ARROW);
        delay(80);
    }
    delay(200);
    
    // Skip the drive header - move down once
    pressKey(KEY_DOWN_ARROW);
    delay(200);
    
    // Perform sweeps: down then up, repeat
    for (int sweep = 0; sweep < MAX_SWEEPS; sweep++) {
        bool goingDown = (sweep % 2 == 0);  // Alternate direction each sweep
        
        if (lcdAvailable) {
            LiquidCrystal_I2C& lcd = getLCD();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("SWEEP ");
            lcd.print(sweep + 1);
            lcd.print("/");
            lcd.print(MAX_SWEEPS);
            lcd.print(goingDown ? " DN" : " UP");
            lcd.setCursor(0, 1);
            lcd.print("Deleting...");
        }
        
        DEBUG_PRINT(F("Sweep "));
        DEBUG_PRINT(sweep + 1);
        DEBUG_PRINTLN(goingDown ? F(" going DOWN") : F(" going UP"));
        
        // Try to delete at each position in this sweep
        for (int pos = 0; pos < 8; pos++) {
            totalAttempts++;
            
            // Update LCD with position
            if (lcdAvailable) {
                LiquidCrystal_I2C& lcd = getLCD();
                lcd.setCursor(11, 1);
                lcd.print("P");
                lcd.print(pos + 1);
                lcd.print(" ");
            }
            
            // DELETE SEQUENCE:
            // 1. TAB to change to delete panel
            // 2. RIGHT to delete button
            // 3. ENTER to click delete
            // 4. TAB to OK button in confirm dialog
            // 5. ENTER to confirm
            
            // TAB to change to delete panel
            pressKey(KEY_TAB);
            delay(400);
            
            // RIGHT to delete button
            pressKey(KEY_RIGHT_ARROW);
            delay(400);
            
            // ENTER to click delete
            pressKey(KEY_RETURN);
            delay(500);
            
            // TAB to OK button
            pressKey(KEY_TAB);
            delay(300);
            
            // ENTER to confirm
            pressKey(KEY_RETURN);
            delay(600);
            
            // Move to next partition row (UP or DOWN)
            if (goingDown) {
                pressKey(KEY_DOWN_ARROW);
            } else {
                pressKey(KEY_UP_ARROW);
            }
            delay(300);
        }
        
        // After each sweep, go to opposite end to start next sweep
        if (goingDown) {
            // We were going down, now go to top for up sweep
            for (int i = 0; i < 10; i++) {
                pressKey(KEY_UP_ARROW);
                delay(60);
            }
            pressKey(KEY_DOWN_ARROW);  // Skip header
            delay(100);
        } else {
            // We were going up, now go to bottom for down sweep
            for (int i = 0; i < 10; i++) {
                pressKey(KEY_DOWN_ARROW);
                delay(60);
            }
        }
        delay(200);
    }
    
    DEBUG_PRINT(F("Smart deletion complete. Total attempts: "));
    DEBUG_PRINTLN(totalAttempts);
    
    // Final cleanup - select unallocated space and start install
    if (lcdAvailable) {
        showStatus("FINALIZING", "Starting...");
    }
    DEBUG_PRINTLN(F("Selecting unallocated space and starting install..."));
    
    // Go to top
    for (int i = 0; i < 10; i++) {
        pressKey(KEY_UP_ARROW);
        delay(80);
    }
    
    // Select first item (should be unallocated space)
    pressKey(KEY_DOWN_ARROW);
    delay(300);
    
    // Tab to Next button and press Enter
    for (int i = 0; i < 6; i++) {
        pressKey(KEY_TAB);
        delay(120);
    }
    pressKey(KEY_RETURN);
    delay(800);
    
    // Press Enter again in case of any confirmation dialog
    pressKey(KEY_RETURN);
    delay(500);
    
    // ==========================================
    // COMPLETE
    // ==========================================
    if (lcdAvailable) {
        showStatus("DONE!", "Install started");
    }
    
    DEBUG_PRINTLN(F("\n========================================"));
    DEBUG_PRINTLN(F("  WINDOWS 10 PARTITION WIPE COMPLETE"));
    DEBUG_PRINTLN(F("  Installation should be starting..."));
    DEBUG_PRINTLN(F("========================================\n"));
    
    payloadExecuted = true;
}

// ============================================
// Setup
// ============================================
void setup() {
    // Initialize pins FIRST - including both safety wire pins
    pinMode(SAFETY_PIN_1, INPUT_PULLUP);  // D7 - primary safety
    pinMode(SAFETY_PIN_2, INPUT_PULLUP);  // D10 - mode select
    pinMode(LED_PIN, OUTPUT);
    ledOff();
    
    // Initialize serial for debugging
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);  // Brief delay for serial
    
    Serial.println(F("\n===================================="));
    Serial.println(F(" BIOS/WIN10 MULTI-TOOL DEVICE"));
    Serial.println(F(" D7 removed = BIOS password"));
    Serial.println(F(" D7+D10 removed = Win10 install"));
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
    showStatus("MULTI-TOOL", "Checking...");
    delay(300);
    
    // ==========================================
    // SAFETY WIRE CHECK
    // ==========================================
    // D7 to GND: Primary safety (must remove to execute anything)
    // D10 to GND: Mode select (remove for Win10, keep for BIOS password)
    
    Serial.println(F("Checking safety wires..."));
    Serial.print(F("  D7 (primary): "));
    Serial.println(isSafety1Off() ? F("REMOVED (armed)") : F("connected (safe)"));
    Serial.print(F("  D10 (mode): "));
    Serial.println(isSafety2Off() ? F("REMOVED (Win10)") : F("connected (BIOS)"));
    
    if (!isSafetyOff()) {
        // Primary safety wire is connected - DO NOT EXECUTE
        Serial.println(F("\n  PRIMARY SAFETY ON - waiting..."));
        Serial.println(F("  Remove D7 wire to arm device."));
        Serial.println(F("  Also remove D10 for Win10 install mode."));
        
        if (lcdAvailable) {
            showStatus("SAFETY ON", "Remove D7 wire");
        }
        
        // Slow blink to indicate safe mode - wait until D7 removed
        while (true) {
            ledOn();
            delay(1000);
            ledOff();
            delay(1000);
            
            // Check if primary wire was removed
            if (isSafetyOff()) {
                Serial.println(F("  D7 removed - ARMING!"));
                break;
            }
        }
    }
    
    // Primary safety is OFF - check mode and proceed
    bool win10Mode = isWin10Mode();
    
    Serial.println(F("\n  PRIMARY SAFETY OFF - Device armed!"));
    Serial.print(F("  Mode: "));
    Serial.println(win10Mode ? F("WINDOWS 10 INSTALL") : F("BIOS PASSWORD REMOVAL"));
    
    // Update LCD with hardware check result
    #if DEMO_MODE
    showStatus("** DEMO MODE **", "No keys sent!");
    delay(1500);
    #endif
    
    if (win10Mode) {
        showStatus("MODE: WIN10", "Install ready");
    } else {
        showStatus("MODE: BIOS", "Password ready");
    }
    delay(500);
    
    Serial.println(F("Hardware checks passed!\n"));
    
    // ==========================================
    // EXECUTE BASED ON MODE
    // ==========================================
    if (lcdAvailable) {
        showStatus("!! ARMED !!", "Executing...");
    }
    blinkLED(3, 100);  // Quick blink to indicate starting
    
    if (win10Mode) {
        // D7 AND D10 removed - Windows 10 Install mode
        Serial.println(F("Executing Windows 10 clean install..."));
        executeWindows10Install();
        
        if (lcdAvailable) {
            showStatus("DONE!", "Win10 wipe done");
        }
    } else {
        // Only D7 removed - BIOS Password Removal mode
        Serial.println(F("Executing BIOS password removal..."));
        executeBIOSPasswordRemoval();
        
        if (lcdAvailable) {
            showStatus("COMPLETE!", "Password removed");
        }
    }
    
    ledOn();  // Solid LED = complete
    
    /* ==========================================
     * WINDOWS 10 INSTALLER CODE - DISABLED
     * Uncomment below and comment out executeBIOSPasswordRemoval()
     * if you want to use the Windows installer functionality
     * ==========================================
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
    * ========================================== */
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
