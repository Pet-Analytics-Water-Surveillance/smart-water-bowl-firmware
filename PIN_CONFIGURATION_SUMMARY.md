# Pin Configuration Summary - Smart Pet Bowl v2.0

## ✅ Final Pin Assignment

| Component | Type | ESP32 Pins | Baud/Frequency | Notes |
|-----------|------|------------|----------------|-------|
| **Grove AI Vision V2** | I2C | SDA: 5<br>SCL: 6 | 400kHz (Fast Mode) | Seeed Grove sensor |
| **Status LED** | Digital Out | 9 | - | Visual indicator |
| **RD-03 Radar** | UART | RX: 7<br>TX: 8 | 115200 baud | **UART1** - Motion detection |
| **Ultrasonic Sensor** | Digital I/O | TRIG: 1<br>ECHO: 2 | - | Water level measurement |
| **Pump Relay** | Digital Out | 10 | - | Water circulation control |

## 🔧 Hardware Connections

### Grove AI Vision V2
```
Grove Module          ESP32-C3
━━━━━━━━━━━━         ━━━━━━━━━━
VCC (3.3V)  ─────────→ 3.3V
GND         ─────────→ GND
SDA         ─────────→ Pin 5
SCL         ─────────→ Pin 6
```

### RD-03 Radar Sensor (CRITICAL - UART)
```
RD-03 Sensor          ESP32-C3
━━━━━━━━━━━━         ━━━━━━━━━━
VCC (5V)    ─────────→ 5V
GND         ─────────→ GND
TXD         ─────────→ Pin 7 (RX) ← CROSS-CONNECT
RXD         ─────────→ Pin 8 (TX) ← CROSS-CONNECT
```
**⚠️ IMPORTANT**: RD-03 uses UART, NOT a simple digital pin!

### Ultrasonic Sensor
```
Ultrasonic            ESP32-C3
━━━━━━━━━━━━         ━━━━━━━━━━
VCC         ─────────→ 5V/3.3V
GND         ─────────→ GND
TRIG        ─────────→ Pin 1
ECHO        ─────────→ Pin 2
```

### Status LED
```
LED (+ Resistor)      ESP32-C3
━━━━━━━━━━━━━━       ━━━━━━━━━━
Anode (+)   ─────────→ Pin 9
Cathode (-) ─────────→ GND
```
Use 220Ω - 1kΩ resistor in series

### Pump Relay Module
```
Relay Module          ESP32-C3
━━━━━━━━━━━━         ━━━━━━━━━━
VCC         ─────────→ 5V/3.3V (check module)
GND         ─────────→ GND
IN          ─────────→ Pin 10
```

## 📋 Changes from Previous Configuration

### What Changed
| Component | Old Pin(s) | New Pin(s) | Change Type |
|-----------|-----------|-----------|-------------|
| RD-03 Radar | 3 (Digital) | 7 (RX) + 8 (TX) | **UART instead of digital** ⚠️ |
| Status LED | 8 | 9 | Pin reassignment |
| Buzzer | 9 | ~~Removed~~ | No longer in design |
| Ultrasonic | 1, 2 | 1, 2 | No change ✓ |
| Pump Relay | 10 | 10 | No change ✓ |
| Grove AI | 5, 6 | 5, 6 | No change ✓ |

### Key Insight: RD-03 is UART!

**Before (WRONG):**
```cpp
pinMode(RD03_OUT, INPUT);
bool motion = digitalRead(RD03_OUT);  // ❌ Wrong!
```

**After (CORRECT):**
```cpp
HardwareSerial radarSerial(1);
radarSerial.begin(115200, SERIAL_8N1, RD03_RX_PIN, RD03_TX_PIN);

// Read UART data
if (radarSerial.available()) {
    String data = radarSerial.readStringUntil('\n');
    if (data.indexOf("Range") >= 0) {
        // Motion detected! ✓
    }
}
```

## 📝 File Changes Summary

### 1. config.h
- Changed `RD03_OUT` (pin 3) → `RD03_RX_PIN` (pin 7) + `RD03_TX_PIN` (pin 8)
- Added `RD03_BAUD_RATE` definition (115200)
- Changed `STATUS_LED` from pin 8 → pin 9
- Removed `BUZZER` definition

### 2. sensors.h
- **Major rewrite** for RD-03 UART support
- Added `HardwareSerial radarSerial(1)` instance
- Replaced `digitalRead()` with UART communication
- Added frame parser for "Range X" and "None" formats
- Added `getDetectedRange()` function
- Removed buzzer code from `checkLowWater()`

### 3. main.ino
- Updated `initializePins()` to remove RD-03 pinMode (now UART)
- Removed buzzer pinMode and digitalWrite calls
- Added comment about RD-03 UART initialization

### 4. hardware_test.ino (New Test Suite)
- Complete rewrite with UART support for RD-03
- Tests UART communication (115200 baud)
- Displays real-time radar frames
- 15-second motion detection test
- Continuous monitoring mode after tests

## 🧪 Testing Checklist

Before flashing main firmware, verify:

- [ ] Grove AI Vision detected on I2C bus
- [ ] Status LED blinks correctly on pin 9
- [ ] RD-03 sends UART data (115200 baud on pins 7/8)
- [ ] RD-03 detects motion when you wave hand
- [ ] Ultrasonic sensor returns valid distances
- [ ] Pump relay clicks when activated

**Upload `hardware_test.ino` first to verify all connections!**

## 🔍 Debugging Tips

### RD-03 Not Working?

1. **Check Serial Monitor** - You should see frames like:
   ```
   [RD-03] Raw: Range 3
   [RD-03] Raw: None
   ```

2. **Verify Baud Rate** - Must be 115200

3. **Check Wiring** - TX/RX are crossed:
   - RD-03 TX → ESP32 RX (Pin 7)
   - RD-03 RX → ESP32 TX (Pin 8)

4. **Power** - RD-03 needs 5V, draws ~100mA

5. **Test with Simple Echo**:
   ```cpp
   void loop() {
       while (radarSerial.available()) {
           Serial.write(radarSerial.read());
       }
   }
   ```

### Other Components

- **I2C issues**: Try scanning with Wire.h scanner
- **Ultrasonic timeout**: Check TRIG/ECHO wiring
- **LED not lighting**: Check polarity and resistor
- **Relay not switching**: Verify module logic level (3.3V vs 5V)

## 📚 Additional Resources

- See `HARDWARE_TEST_GUIDE.md` for detailed testing procedures
- See `RD03_INTEGRATION_GUIDE.md` for RD-03 technical details
- See `README.md` for overall system architecture

## ⚡ Quick Reference Code

### Initialize All Hardware
```cpp
#include "config.h"

// LED & Relay
pinMode(STATUS_LED, OUTPUT);
pinMode(PUMP_RELAY, OUTPUT);

// I2C (Grove AI)
Wire.begin(I2C_SDA, I2C_SCL);

// RD-03 UART
HardwareSerial radarSerial(1);
radarSerial.begin(RD03_BAUD_RATE, SERIAL_8N1, RD03_RX_PIN, RD03_TX_PIN);

// Ultrasonic
pinMode(ULTRASONIC_TRIG, OUTPUT);
pinMode(ULTRASONIC_ECHO, INPUT);
```

### Check RD-03 Motion
```cpp
bool motionDetected = false;

if (radarSerial.available()) {
    String frame = radarSerial.readStringUntil('\n');
    if (frame.indexOf("Range") >= 0) {
        motionDetected = true;
        int gate = frame.substring(frame.indexOf("Range") + 5).toInt();
        Serial.printf("Motion at gate %d\n", gate);
    }
}
```

## ✅ Configuration Complete!

Your firmware is now configured for:
- ✅ Grove AI Vision on I2C (pins 5, 6)
- ✅ RD-03 Radar on UART (pins 7, 8) @ 115200 baud
- ✅ Status LED on pin 9
- ✅ Ultrasonic sensor on pins 1, 2
- ✅ Pump relay on pin 10

**Next Step**: Flash `hardware_test.ino` to verify all connections!

