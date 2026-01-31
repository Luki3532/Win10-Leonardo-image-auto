/**
 * Keyboard Utilities
 * 
 * Helper functions for HID keyboard operations
 * using the Arduino Leonardo's native USB
 */

#ifndef KEYBOARD_UTILS_H
#define KEYBOARD_UTILS_H

#include <Arduino.h>
#include <Keyboard.h>
#include "../include/config.h"

// Initialize the keyboard
void initKeyboard();

// Press and release a single key
void pressKey(uint8_t key);

// Press and release a regular character
void pressChar(char c);

// Type a string
void typeString(const char* str);

// Press a key combination (e.g., ALT+D)
void pressCombo(uint8_t modifier, uint8_t key);

// Press two modifiers + key (e.g., CTRL+ALT+DEL)
void pressCombo3(uint8_t mod1, uint8_t mod2, uint8_t key);

// Hold a key for a duration
void holdKey(uint8_t key, int durationMs);

// Spam a key repeatedly for a duration (returns actual key presses sent)
int spamKey(uint8_t key, int durationMs, int intervalMs);

// Press DOWN arrow multiple times
void pressDownMultiple(int times);

// Press TAB multiple times  
void pressTabMultiple(int times);

// Wait with countdown on LCD, then press a key
void waitThenPress(uint8_t key, int waitMs);

// Release all keys (safety)
void releaseAllKeys();

#endif // KEYBOARD_UTILS_H
