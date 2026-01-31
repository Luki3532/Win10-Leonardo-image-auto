# Auto Windows 10 Reinstall Device

An Arduino Leonardo-based HID keyboard automation device that automatically wipes and reinstalls Windows 10 on Dell machines.

## Features

- **Automated BIOS boot menu navigation** (F12 for Dell)
- **Windows Setup automation** - language, license, partition management
- **Complete disk wipe** - deletes all partitions automatically
- **LCD status display** - 16x2 I2C LCD shows progress
- **Safety arming** - 3-second button hold required to activate
- **Error codes** - LCD displays diagnostic codes if issues occur
- **Demo mode** - Test without sending keystrokes

## Hardware Requirements

| Component | Description |
|-----------|-------------|
| Arduino Leonardo | ATmega32u4 with native USB HID |
| 16x2 LCD | QAPASS or similar with I2C backpack |
| I2C Backpack | HW-061 or PCF8574-based (address 0x27 or 0x3F) |
| Momentary Button | For arming the device |
| Windows 10 USB | Bootable installation media |

## Wiring Schematic

```
                    ARDUINO LEONARDO
                   ┌────────────────┐
                   │            USB │ ──► To Target PC
                   │                │
    ┌──────────────┤ D2 (SDA)       │
    │   ┌──────────┤ D3 (SCL)       │
    │   │          │                │
    │   │   ┌──────┤ D7             │
    │   │   │      │                │
    │   │   │  ┌───┤ 5V             │
    │   │   │  │   │                │
    │   │   │  │ ┌─┤ GND            │
    │   │   │  │ │ │                │
    │   │   │  │ │ │ D13 (LED)      │──► Status LED (built-in)
    │   │   │  │ │ └────────────────┘
    │   │   │  │ │
    │   │   │  │ │      ARM BUTTON
    │   │   │  │ │     ┌─────────┐
    │   │   │  │ └─────┤ GND     │
    │   │   └──────────┤ Signal  │ (Momentary Push Button)
    │   │      │       └─────────┘
    │   │      │        (Uses internal pull-up)
    │   │      │
    │   │      │
    │   │   16x2 LCD WITH I2C BACKPACK (HW-061)
    │   │      │       ┌─────────────────────────────┐
    │   │      │       │  ┌─────────────────────┐    │
    │   │      │       │  │                     │    │
    │   │      │       │  │   QAPASS 1602A      │    │
    │   │      │       │  │                     │    │
    │   │      │       │  │  ████████████████   │    │
    │   │      │       │  │  ████████████████   │    │
    │   │      │       │  │                     │    │
    │   │      │       │  └─────────────────────┘    │
    │   │      │       │                             │
    │   │      │       │  [GND] [VCC] [SDA] [SCL]    │
    │   │      │       └────┬─────┬─────┬─────┬──────┘
    │   │      │            │     │     │     │
    │   │      └────────────│─────│─────│─────┘
    │   └───────────────────│─────│─────┘
    └───────────────────────│─────┘
                            │
                          ──┴── GND (common ground)


PINOUT SUMMARY:
═══════════════════════════════════════════════════════
 LEONARDO PIN  │  CONNECTS TO           │  FUNCTION
═══════════════════════════════════════════════════════
 D2 (SDA)      │  LCD SDA               │  I2C Data
 D3 (SCL)      │  LCD SCL               │  I2C Clock
 D7            │  Button (other to GND) │  Arm trigger
 5V            │  LCD VCC               │  Power
 GND           │  LCD GND + Button      │  Ground
 USB           │  Target PC             │  HID Keyboard
═══════════════════════════════════════════════════════

NOTE: Leonardo uses D2/D3 for I2C, NOT A4/A5 like Arduino Uno!
```

## I2C Address

The LCD I2C address varies by module:
- **0x27** - Most common (blue backpack)
- **0x3F** - HW-061 and some others

Use `I2C_SCAN_MODE 1` in config.h to find your address.

## Configuration

Edit `include/config.h` to customize:

```cpp
// LCD Settings
#define LCD_ADDRESS         0x3F    // Your I2C address
#define LCD_COLS            16
#define LCD_ROWS            2

// Boot Settings (Dell = F12)
#define BOOT_KEY            KEY_F12
#define BOOT_MENU_POSITION  2       // 0-indexed, 3rd option

// Safety
#define ARM_HOLD_TIME       3000    // 3 seconds to arm
#define DEMO_MODE           0       // 1 = no keystrokes sent
```

## Usage

1. **Prepare target PC** - Insert Windows 10 USB drive
2. **Connect Leonardo** - Plug into target PC USB port
3. **Power on target PC** - Device will wait for arm signal
4. **Arm the device** - Hold button for 3 seconds
5. **Watch the magic** - LCD shows progress through each phase

## Phases

| Phase | Description |
|-------|-------------|
| 1/5 | Boot Menu - Spam F12, select USB boot |
| 2/5 | Waiting - Windows Setup loading |
| 3/5 | Win Setup - Navigate installer options |
| 4/5 | Wipe Disk - Delete all partitions |
| 5/5 | Install - Start Windows installation |

## Error Codes

| Code | Description |
|------|-------------|
| E01 | LCD not detected |
| E02 | Wrong I2C address |
| E03 | I2C communication error |
| E10 | Button pin floating |
| E20+ | Runtime errors |

## Building

Requires [PlatformIO](https://platformio.org/):

```bash
# Build
pio run

# Upload
pio run --target upload

# Serial Monitor
pio device monitor --baud 9600
```

## Demo Mode

Set `DEMO_MODE 1` in config.h to test without sending keystrokes. All actions are logged to Serial and LCD but no keys are actually pressed.

## Project Structure

```
auto-reim/
├── src/
│   ├── main.cpp              # Main program & payload logic
│   ├── display.cpp/h         # LCD display functions
│   ├── keyboard_utils.cpp/h  # HID keyboard helpers
│   ├── error_handler.cpp/h   # Error codes & handling
│   └── i2c_scanner.cpp/h     # I2C address finder
├── include/
│   └── config.h              # All configuration settings
├── platformio.ini            # PlatformIO build config
└── README.md
```

## License

MIT License - Use at your own risk. This tool will **permanently erase all data** on target machines.

## ⚠️ Warning

This device is designed to **wipe hard drives**. Only use on machines you own and intend to reinstall. The author is not responsible for data loss.
