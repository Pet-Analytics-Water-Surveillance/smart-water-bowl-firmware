# Hardware Test Guide

## ✅ Configuration Updated

The firmware has been updated to use your new pin arrangement:

### Pin Configuration

| Component | ESP32 Pin | Notes |
|-----------|-----------|-------|
| **Grove AI Vision** | SDA: Pin 5<br>SCL: Pin 6 | I2C communication |
| **Status LED** | Pin 9 | Visual status indicator |
| **RD-03 Radar** | RX: Pin 7<br>TX: Pin 8 | **UART communication @ 115200 baud**<br>RX ← RD-03 TX<br>TX → RD-03 RX |
| **Ultrasonic Sensor** | TRIG: Pin 1<br>ECHO: Pin 2 | Water level measurement |
| **Pump Relay** | Pin 10 | Controls water pump |

## 🧪 Testing Before Main Firmware

### Step 1: Upload Test Firmware

1. Open `hardware_test.ino` in Arduino IDE
2. Select your ESP32 board and COM port
3. Upload the test firmware
4. Open Serial Monitor (115200 baud)

### Step 2: What the Test Does

The test firmware automatically checks all hardware components:

#### Test 1: I2C Communication
- Scans for I2C devices on pins 5 & 6
- Attempts to initialize Grove AI Vision
- **Expected**: Should find at least one I2C device

#### Test 2: Status LED
- Blinks LED 5 times on pin 9
- **Action**: Verify you see the LED blinking

#### Test 3: RD-03 Radar (UART)
- Initializes UART communication at 115200 baud
- Reads radar data frames for 15 seconds
- Displays received data and parses for motion detection
- LED turns on when target detected
- **Action**: Wave your hand in front of the sensor
- **Expected**: Should receive frames like "Range X" or "None"

#### Test 4: Ultrasonic Sensor
- Takes 10 distance readings
- Calculates average distance
- **Expected**: Should get valid readings (2-400 cm range)

#### Test 5: Pump Relay
- Activates relay for 2 seconds
- LED indicator turns on
- **Action**: Listen for relay click

### Step 3: Review Results

After all tests complete, you'll see a summary:

```
╔═══════════════════════════════════════════╗
║          TEST RESULTS SUMMARY            ║
╚═══════════════════════════════════════════╝

  I2C Communication:    ✓ PASS
  Status LED:           ✓ PASS
  RD-03 Radar:          ✓ PASS
  Ultrasonic Sensor:    ✓ PASS
  Pump Relay:           ✓ PASS

🎉 ALL TESTS PASSED!
   Your hardware is ready for main firmware.
```

### Step 4: Continuous Monitoring

After tests complete, the firmware enters continuous monitoring mode:
- Shows real-time radar status
- Displays water distance measurements
- LED blinks every 2 seconds

## 🚨 Troubleshooting

### I2C Not Detected
- Check Grove AI Vision connections (SDA→5, SCL→6)
- Verify power supply to Grove module
- Ensure proper I2C pull-up resistors

### RD-03 Radar Not Working
- **Critical**: RD-03 uses UART, not simple digital output!
- Verify connections:
  - RD-03 TX → ESP32 Pin 7 (RX)
  - RD-03 RX → ESP32 Pin 8 (TX)
  - RD-03 VCC → 5V
  - RD-03 GND → GND
- Check baud rate is set to 115200
- Ensure sensor is powered (5V)
- Verify RD-03 is in correct operating mode
- Look for UART data in Serial Monitor during test

### Ultrasonic Readings Invalid
- Check TRIG→Pin 1, ECHO→Pin 2
- Verify sensor has clear line of sight
- Maximum range is typically 4 meters

### Pump Relay Not Clicking
- Check relay connection to pin 10
- Verify relay power supply
- Ensure relay is compatible with ESP32 logic level (3.3V)

## ✨ Next Steps

Once all tests pass:

1. Upload the main firmware (`main.ino`)
2. The device will boot into BLE provisioning mode
3. Use mobile app to configure WiFi and Supabase credentials
4. Device will restart and enter normal operation

## 📝 Changes Made to Main Firmware

1. **config.h**: Updated all pin definitions
   - RD-03 now uses UART pins (RX=7, TX=8) instead of single digital pin
   - Added RD03_BAUD_RATE definition (115200)
   
2. **main.ino**: Removed buzzer references (not in new pin config)

3. **sensors.h**: Major update for RD-03 UART support
   - Replaced digitalRead with UART communication
   - Added HardwareSerial instance for radar
   - Parser for RD-03 frame data ("Range X" / "None")
   - Replaced buzzer alerts with LED flashing

All references to the old pin configuration have been updated throughout the firmware.

### Important: RD-03 is UART, Not Digital!

The RD-03 communicates via UART serial at 115200 baud. It sends text frames indicating presence and range. This is NOT a simple HIGH/LOW digital sensor.

## ⚠️ Important Notes

- **CRITICAL**: RD-03 uses UART communication (TX/RX pins), NOT a simple digital output!
- RD-03 communicates at 115200 baud and sends text frames
- Ultrasonic "TX" and "RX" refer to TRIG and ECHO pins (not UART)
- Always ensure pump relay is properly connected before testing
- Do NOT connect pump to water source during initial testing
- RD-03 requires 5V power supply (not 3.3V)

---

**Ready to test?** Upload `hardware_test.ino` and verify all components are working! ✅

