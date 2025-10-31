# RD-03 Radar Sensor Integration Guide

## Overview

The Ai-Thinker RD-03 is a 24GHz FMCW radar sensor that communicates via UART at 115200 baud. It provides presence detection and distance estimation.

## Hardware Connection

### Wiring Diagram

```
RD-03 Sensor          ESP32-C3
━━━━━━━━━━━━         ━━━━━━━━━━
VCC (5V)    ─────────→ 5V
GND         ─────────→ GND
TXD         ─────────→ Pin 7 (RX)
RXD         ─────────→ Pin 8 (TX)
```

**Important Notes:**
- RD-03 requires **5V power** (not 3.3V)
- TX/RX pins are 3.3V logic compatible with ESP32
- Cross-connect: RD-03 TX → ESP32 RX, RD-03 RX → ESP32 TX

## Communication Protocol

### UART Settings
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Parity**: None
- **Stop Bits**: 1
- **Flow Control**: None

### Operating Modes

The RD-03 has three operating modes:

1. **OPERATING_MODE** (Default)
   - Simplified output: "Range X" or "None"
   - Best for presence detection
   - Low CPU overhead
   
2. **REPORTING_MODE**
   - Detailed energy readings per distance gate
   - Includes target distance and confidence
   - Useful for debugging
   
3. **DEBUGGING_MODE**
   - Raw Doppler data
   - 2D array of frequency vs distance
   - High data rate

## Current Implementation

### In `sensors.h`

The firmware uses **OPERATING_MODE** which outputs simple text frames:

```cpp
HardwareSerial radarSerial(1);  // UART1

void initializeSensors() {
    radarSerial.begin(115200, SERIAL_8N1, RD03_RX_PIN, RD03_TX_PIN);
    radarSerial.setRxBufferSize(1024);
}
```

### Frame Parsing

Expected frames from RD-03 in OPERATING_MODE:

- **Target detected**: `"Range X"` where X is distance gate (0-15)
- **No target**: `"None"` or `"none"`

Parser implementation:

```cpp
if (radarSerial.available()) {
    String radarData = radarSerial.readStringUntil('\n');
    radarData.trim();
    
    if (radarData.indexOf("Range") >= 0) {
        // Motion detected
        int rangeStart = radarData.indexOf("Range") + 5;
        detectedRange = radarData.substring(rangeStart).toInt();
        motionDetected = true;
    } else if (radarData.indexOf("None") >= 0) {
        // No motion
        motionDetected = false;
    }
}
```

## Advanced Configuration (Optional)

### Using the Ai-Thinker-RD-03 Library

For advanced features, you can use the full library:

```cpp
#include <Ai-Thinker-RD-03.h>

AiThinker_RD_03 radar;

void setup() {
    radar.begin(radarSerial, RD03_RX_PIN, RD03_TX_PIN);
    
    // Enter config mode
    radar.enableConfigMode();
    
    // Configure detection parameters
    radar.setMinDetectionDistance(1);  // gates (0-15)
    radar.setMaxDetectionDistance(10); // gates (0-15)
    radar.setMinFramesForDetection(3); // frames needed to confirm
    radar.setMinFramesForDisappear(3); // frames needed to clear
    radar.setTargetDisappearDelay(2);  // seconds
    
    // Set operating mode
    radar.setSystemMode(AiThinker_RD_03::OPERATING_MODE);
    
    // Exit config mode
    radar.disableConfigMode();
}
```

### Configurable Parameters

| Parameter | Range | Description |
|-----------|-------|-------------|
| MinDetectionDistance | 0-15 | Minimum gate to detect (each gate ≈ 0.75m) |
| MaxDetectionDistance | 0-15 | Maximum gate to detect |
| MinFramesForDetection | 1-15 | Frames before confirming presence |
| MinFramesForDisappear | 1-15 | Frames before clearing presence |
| TargetDisappearDelay | 0-15 | Seconds to wait before clearing |

## Distance Gates

The RD-03 divides space into 16 distance gates:

| Gate | Approximate Distance |
|------|---------------------|
| 0 | 0 - 0.75m |
| 1 | 0.75 - 1.5m |
| 2 | 1.5 - 2.25m |
| ... | ... |
| 15 | 11.25 - 12m |

Each gate is approximately **0.75 meters** wide.

## Troubleshooting

### No Data Received

1. **Check Wiring**
   - Verify RD-03 TX → ESP32 Pin 7 (RX)
   - Verify RD-03 RX → ESP32 Pin 8 (TX)
   
2. **Check Power**
   - RD-03 needs 5V (not 3.3V)
   - Current draw: ~100mA
   
3. **Check Baud Rate**
   - Default is 115200
   - If changed, may need to scan for correct rate

4. **Serial Monitor Test**
   ```cpp
   void loop() {
       if (radarSerial.available()) {
           Serial.write(radarSerial.read());
       }
   }
   ```
   You should see continuous data output.

### Inconsistent Detection

1. **Adjust Sensitivity**
   - Increase MinFramesForDetection to reduce false positives
   - Decrease to increase sensitivity

2. **Adjust Range**
   - Set appropriate min/max detection distances
   - For pet bowl, gates 1-5 (0.75m - 3.75m) may be optimal

3. **Environmental Factors**
   - Metal objects can cause reflections
   - Moving fans/curtains can trigger detection
   - Mounting angle affects detection zone

### Frame Format Issues

If you're getting garbled data:

1. Check for buffer overflow
   ```cpp
   radarSerial.setRxBufferSize(2048);  // Increase buffer
   ```

2. Add frame synchronization
   ```cpp
   while (radarSerial.available() && radarSerial.peek() != 'R') {
       radarSerial.read();  // Discard until 'R' (Range)
   }
   ```

## Integration with Pet Bowl Logic

### Current State Machine Flow

```
IDLE → [RD-03 detects presence] → PRESENCE_DETECTED
     → [Capture image with AI Vision]
     → [Identify pet]
     → DRINKING
     → [Monitor water level]
     → [RD-03 detects departure] → IDLE
```

### Timeout Handling

The firmware uses a 2-second timeout:

```cpp
if (millis() - lastMotionTime > 2000 && motionDetected) {
    motionDetected = false;  // Clear if no recent frames
}
```

This prevents false "stuck on" state if UART communication stops.

## Performance Considerations

### CPU Load
- Simple string parsing is lightweight
- No blocking delays in checkPresence()
- Suitable for FreeRTOS task

### Memory Usage
- 1KB UART buffer
- Minimal string allocations
- Cleared after each read

### Response Time
- Frame rate: ~10-20 Hz
- Detection latency: 50-200ms
- Adequate for pet detection

## Future Enhancements

1. **Add Library for Advanced Features**
   - Install via Arduino Library Manager
   - Enable configuration mode at startup
   - Tune detection parameters

2. **Implement REPORTING_MODE**
   - Get distance in meters (not just gate)
   - Monitor signal strength
   - Log confidence levels

3. **Add Calibration Routine**
   - Learn empty environment baseline
   - Adjust thresholds dynamically
   - Store settings in EEPROM

4. **Multi-target Tracking**
   - Distinguish multiple pets
   - Track approach direction
   - Estimate size based on signal strength

## References

- [RD-03 Datasheet](https://files.seeedstudio.com/products/114992867/RD-03%20Datasheet.pdf)
- [Ai-Thinker-RD-03 Arduino Library](https://github.com/CONTROLLINO-PLC/Ai-Thinker-RD-03)
- [ESP32 UART Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/uart.html)

## Sample Output

Expected Serial Monitor output during operation:

```
[RD-03] Raw: Range 3
[RD-03] Target detected at range 3
[RD-03] Raw: Range 3
[RD-03] Raw: Range 4
[RD-03] Raw: None
[RD-03] Target cleared
```

---

**Note**: The current implementation prioritizes simplicity and reliability. For most pet detection use cases, the basic OPERATING_MODE is sufficient. Advanced features can be added later if needed.

