/**
 * Keyboard Utilities Implementation
 */

#include "keyboard_utils.h"

void initKeyboard() {
    #if DEMO_MODE
        Serial.println(F("[DEMO] Keyboard disabled - demo mode active"));
    #else
        Keyboard.begin();
        DEBUG_PRINTLN(F("Keyboard initialized"));
    #endif
}

void pressKey(uint8_t key) {
    #if DEMO_MODE
        Serial.print(F("[DEMO] Press key: 0x"));
        Serial.println(key, HEX);
    #else
        Keyboard.press(key);
        delay(KEY_HOLD_DELAY);
        Keyboard.release(key);
    #endif
    delay(KEY_DELAY);
}

void pressChar(char c) {
    #if DEMO_MODE
        Serial.print(F("[DEMO] Press char: "));
        Serial.println(c);
    #else
        Keyboard.write(c);
    #endif
    delay(KEY_DELAY);
}

void typeString(const char* str) {
    #if DEMO_MODE
        Serial.print(F("[DEMO] Type string: "));
        Serial.println(str);
    #else
        while (*str) {
            Keyboard.write(*str++);
            delay(KEY_DELAY / 2);
        }
    #endif
    delay(KEY_DELAY);
}

void pressCombo(uint8_t modifier, uint8_t key) {
    #if DEMO_MODE
        Serial.print(F("[DEMO] Combo: 0x"));
        Serial.print(modifier, HEX);
        Serial.print(F(" + 0x"));
        Serial.println(key, HEX);
    #else
        Keyboard.press(modifier);
        delay(KEY_HOLD_DELAY);
        Keyboard.press(key);
        delay(KEY_HOLD_DELAY);
        Keyboard.releaseAll();
    #endif
    delay(KEY_DELAY * 2);
}

void pressCombo3(uint8_t mod1, uint8_t mod2, uint8_t key) {
    #if DEMO_MODE
        Serial.print(F("[DEMO] Combo3: 0x"));
        Serial.print(mod1, HEX);
        Serial.print(F(" + 0x"));
        Serial.print(mod2, HEX);
        Serial.print(F(" + 0x"));
        Serial.println(key, HEX);
    #else
        Keyboard.press(mod1);
        delay(KEY_HOLD_DELAY);
        Keyboard.press(mod2);
        delay(KEY_HOLD_DELAY);
        Keyboard.press(key);
        delay(KEY_HOLD_DELAY);
        Keyboard.releaseAll();
    #endif
    delay(KEY_DELAY * 2);
}

void holdKey(uint8_t key, int durationMs) {
    #if DEMO_MODE
        Serial.print(F("[DEMO] Hold key 0x"));
        Serial.print(key, HEX);
        Serial.print(F(" for "));
        Serial.print(durationMs);
        Serial.println(F("ms"));
        delay(durationMs);
    #else
        Keyboard.press(key);
        delay(durationMs);
        Keyboard.release(key);
    #endif
    delay(KEY_DELAY);
}

int spamKey(uint8_t key, int durationMs, int intervalMs) {
    int count = 0;
    unsigned long startTime = millis();
    
    while (millis() - startTime < (unsigned long)durationMs) {
        #if DEMO_MODE
            // Just count, don't actually press
        #else
            Keyboard.press(key);
            delay(KEY_HOLD_DELAY);
            Keyboard.release(key);
        #endif
        delay(intervalMs - KEY_HOLD_DELAY);
        count++;
    }
    
    Serial.print(F("[DEMO] Spammed key 0x"));
    Serial.print(key, HEX);
    Serial.print(F(" "));
    Serial.print(count);
    Serial.println(F(" times"));
    
    return count;
}

void pressDownMultiple(int times) {
    Serial.print(F("[DEMO] DOWN x"));
    Serial.println(times);
    for (int i = 0; i < times; i++) {
        pressKey(KEY_DOWN_ARROW);
    }
}

void pressTabMultiple(int times) {
    Serial.print(F("[DEMO] TAB x"));
    Serial.println(times);
    for (int i = 0; i < times; i++) {
        pressKey(KEY_TAB);
    }
}

void waitThenPress(uint8_t key, int waitMs) {
    delay(waitMs);
    pressKey(key);
}

void releaseAllKeys() {
    #if !DEMO_MODE
        Keyboard.releaseAll();
    #endif
}
