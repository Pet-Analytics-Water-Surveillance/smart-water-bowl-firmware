/*
 * Smart Pet Bowl Hardware Test (Updated for RD-03 UART)
 * Tests all hardware components before flashing main firmware
 * 
 * Test Sequence:
 * 1. I2C communication (Grove AI Vision)
 * 2. Status LED
 * 3. RD-03 Radar sensor (UART)
 * 4. Ultrasonic water level sensor
 * 5. Pump relay
 */

#include <Wire.h>
#include <Seeed_Arduino_GroveAI.h>

// ===== PIN DEFINITIONS (Updated - RD-03 is UART) =====
#define RD03_RX_PIN       7      // ESP32 RX ← RD-03 TX
#define RD03_TX_PIN       8      // ESP32 TX → RD-03 RX
#define RD03_BAUD_RATE    115200 // RD-03 default baud rate

#define ULTRASONIC_TRIG   1      // Ultrasonic trigger (TX)
#define ULTRASONIC_ECHO   2      // Ultrasonic echo (RX)
#define STATUS_LED        9      // Status LED
#define PUMP_RELAY        10     // Pump relay control
#define I2C_SDA           5      // I2C Data (Grove AI Vision)
#define I2C_SCL           6      // I2C Clock (Grove AI Vision)

// Hardware Serial for RD-03
HardwareSerial radarSerial(1);

// Grove AI Vision instance
GroveAI ai(Wire);

// Test results
bool i2cTest = false;
bool ledTest = false;
bool radarTest = false;
bool ultrasonicTest = false;
bool pumpTest = false;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    printHeader();
    
    // Initialize pins
    pinMode(STATUS_LED, OUTPUT);
    pinMode(PUMP_RELAY, OUTPUT);
    pinMode(ULTRASONIC_TRIG, OUTPUT);
    pinMode(ULTRASONIC_ECHO, INPUT);
    
    // Ensure safe starting state
    digitalWrite(STATUS_LED, LOW);
    digitalWrite(PUMP_RELAY, LOW);
    digitalWrite(ULTRASONIC_TRIG, LOW);
    
    Serial.println("\n🔧 Starting Hardware Tests...\n");
    delay(1000);
    
    // Run all tests
    testI2C();
    delay(1000);
    
    testLED();
    delay(1000);
    
    testRadarUART();
    delay(1000);
    
    testUltrasonic();
    delay(1000);
    
    testPumpRelay();
    delay(1000);
    
    // Print final results
    printResults();
}

void loop() {
    // Continuous monitoring mode
    Serial.println("\n========== CONTINUOUS MONITORING ==========");
    
    // Check radar via UART
    if (radarSerial.available()) {
        String radarData = radarSerial.readStringUntil('\n');
        radarData.trim();
        Serial.print("RD-03 Data: ");
        Serial.println(radarData);
        
        if (radarData.indexOf("Range") >= 0) {
            digitalWrite(STATUS_LED, HIGH);
            delay(100);
            digitalWrite(STATUS_LED, LOW);
        }
    }
    
    // Check ultrasonic
    float distance = measureDistance();
    Serial.print("Water Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    
    delay(2000);
}

void printHeader() {
    Serial.println("\n\n");
    Serial.println("╔═══════════════════════════════════════════╗");
    Serial.println("║   SMART PET BOWL - HARDWARE TEST         ║");
    Serial.println("║   Firmware v2.0 Pin Configuration        ║");
    Serial.println("║   RD-03 UART Mode                        ║");
    Serial.println("╚═══════════════════════════════════════════╝");
    Serial.println();
    Serial.println("Pin Configuration:");
    Serial.println("  - Grove AI Vision: SDA=5, SCL=6");
    Serial.println("  - Status LED: Pin 9");
    Serial.println("  - RD-03 Radar: RX=7, TX=8 (UART @ 115200)");
    Serial.println("  - Ultrasonic: TRIG=1, ECHO=2");
    Serial.println("  - Pump Relay: Pin 10");
    Serial.println();
}

void testI2C() {
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("TEST 1: I2C Communication (Grove AI Vision)");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    // Initialize I2C with custom pins
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(100);
    
    Serial.println("Scanning I2C bus...");
    byte error, address;
    int deviceCount = 0;
    
    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        
        if (error == 0) {
            Serial.print("  ✓ I2C device found at address 0x");
            if (address < 16) Serial.print("0");
            Serial.println(address, HEX);
            deviceCount++;
        }
    }
    
    if (deviceCount > 0) {
        Serial.print("\n✓ Found ");
        Serial.print(deviceCount);
        Serial.println(" I2C device(s)");
        i2cTest = true;
        
        // Try to initialize Grove AI Vision
        Serial.println("\nInitializing Grove AI Vision...");
        if (ai.begin(ALGO_OBJECT_DETECTION, MODEL_EXT_INDEX_1)) {
            Serial.println("✓ Grove AI Vision initialized successfully!");
        } else {
            Serial.println("⚠ Grove AI Vision initialization failed (may need model upload)");
        }
    } else {
        Serial.println("\n✗ No I2C devices found!");
        Serial.println("  Check SDA/SCL connections to pins 5 and 6");
        i2cTest = false;
    }
}

void testLED() {
    Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("TEST 2: Status LED (Pin 9)");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    Serial.println("Blinking LED 5 times...");
    
    for (int i = 0; i < 5; i++) {
        digitalWrite(STATUS_LED, HIGH);
        Serial.print("  ON  ");
        delay(300);
        digitalWrite(STATUS_LED, LOW);
        Serial.println("OFF");
        delay(300);
    }
    
    Serial.println("\n✓ LED test complete");
    Serial.println("  Did you see the LED blink 5 times? [Manual verification required]");
    ledTest = true;
}

void testRadarUART() {
    Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("TEST 3: RD-03 Radar Sensor (UART)");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    // Initialize UART for RD-03
    radarSerial.begin(RD03_BAUD_RATE, SERIAL_8N1, RD03_RX_PIN, RD03_TX_PIN);
    radarSerial.setRxBufferSize(1024);
    delay(200);
    
    Serial.println("RD-03 Configuration:");
    Serial.printf("  - Baud Rate: %d\n", RD03_BAUD_RATE);
    Serial.printf("  - RX Pin (ESP32): %d (← RD-03 TX)\n", RD03_RX_PIN);
    Serial.printf("  - TX Pin (ESP32): %d (→ RD-03 RX)\n", RD03_TX_PIN);
    Serial.println();
    
    Serial.println("Reading radar data for 15 seconds...");
    Serial.println("  Wave your hand in front of the sensor!");
    Serial.println();
    
    bool dataReceived = false;
    bool motionDetected = false;
    unsigned long startTime = millis();
    int frameCount = 0;
    
    while (millis() - startTime < 15000) {
        if (radarSerial.available()) {
            String radarData = radarSerial.readStringUntil('\n');
            radarData.trim();
            
            if (radarData.length() > 0) {
                dataReceived = true;
                frameCount++;
                
                Serial.print("  [");
                Serial.print(frameCount);
                Serial.print("] ");
                Serial.println(radarData);
                
                // Check for motion detection
                if (radarData.indexOf("Range") >= 0) {
                    motionDetected = true;
                    digitalWrite(STATUS_LED, HIGH);
                    Serial.println("    ✓ MOTION DETECTED!");
                } else if (radarData.indexOf("None") >= 0 || radarData.indexOf("none") >= 0) {
                    digitalWrite(STATUS_LED, LOW);
                }
            }
        }
        delay(50);
    }
    
    digitalWrite(STATUS_LED, LOW);
    Serial.println();
    
    if (dataReceived) {
        Serial.printf("✓ RD-03 UART communication working! (%d frames received)\n", frameCount);
        if (motionDetected) {
            Serial.println("✓ Motion detection confirmed!");
            radarTest = true;
        } else {
            Serial.println("⚠ No motion detected (try waving in front of sensor)");
            radarTest = true;  // Still pass if we got data
        }
    } else {
        Serial.println("✗ No data received from RD-03");
        Serial.println("  Check connections:");
        Serial.printf("    - RD-03 TX → ESP32 Pin %d (RX)\n", RD03_RX_PIN);
        Serial.printf("    - RD-03 RX → ESP32 Pin %d (TX)\n", RD03_TX_PIN);
        Serial.println("    - RD-03 VCC → 5V");
        Serial.println("    - RD-03 GND → GND");
        Serial.println("  Verify RD-03 is powered and baud rate is 115200");
        radarTest = false;
    }
}

void testUltrasonic() {
    Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("TEST 4: Ultrasonic Sensor (Pins 1 & 2)");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    Serial.println("Taking 10 distance readings...");
    Serial.println();
    
    int validReadings = 0;
    float totalDistance = 0;
    
    for (int i = 0; i < 10; i++) {
        float distance = measureDistance();
        
        Serial.print("  Reading ");
        Serial.print(i + 1);
        Serial.print(": ");
        
        if (distance > 0 && distance < 400) {
            Serial.print(distance);
            Serial.println(" cm ✓");
            validReadings++;
            totalDistance += distance;
        } else {
            Serial.println("Invalid ✗");
        }
        
        delay(200);
    }
    
    Serial.println();
    
    if (validReadings >= 7) {
        float avgDistance = totalDistance / validReadings;
        Serial.print("✓ Ultrasonic sensor working! Average: ");
        Serial.print(avgDistance);
        Serial.println(" cm");
        ultrasonicTest = true;
    } else {
        Serial.println("✗ Too many invalid readings");
        Serial.println("  Check ultrasonic connections to pins 1 (TRIG) and 2 (ECHO)");
        ultrasonicTest = false;
    }
}

void testPumpRelay() {
    Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("TEST 5: Pump Relay (Pin 10)");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    Serial.println("⚠ WARNING: Ensure pump is properly connected!");
    Serial.println("Testing relay activation...");
    Serial.println();
    
    // Test relay on/off
    Serial.println("  Relay ON (2 seconds)...");
    digitalWrite(PUMP_RELAY, HIGH);
    digitalWrite(STATUS_LED, HIGH);
    delay(2000);
    
    Serial.println("  Relay OFF");
    digitalWrite(PUMP_RELAY, LOW);
    digitalWrite(STATUS_LED, LOW);
    delay(1000);
    
    Serial.println("\n✓ Relay test complete");
    Serial.println("  Did you hear the relay click? [Manual verification required]");
    pumpTest = true;
}

float measureDistance() {
    digitalWrite(ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG, LOW);
    
    long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
    
    if (duration == 0) {
        return -1;
    }
    
    return duration * 0.034 / 2;
}

void printResults() {
    Serial.println("\n\n");
    Serial.println("╔═══════════════════════════════════════════╗");
    Serial.println("║          TEST RESULTS SUMMARY            ║");
    Serial.println("╚═══════════════════════════════════════════╝");
    Serial.println();
    
    Serial.print("  I2C Communication:    ");
    Serial.println(i2cTest ? "✓ PASS" : "✗ FAIL");
    
    Serial.print("  Status LED:           ");
    Serial.println(ledTest ? "✓ PASS" : "✗ FAIL");
    
    Serial.print("  RD-03 Radar (UART):   ");
    Serial.println(radarTest ? "✓ PASS" : "✗ FAIL");
    
    Serial.print("  Ultrasonic Sensor:    ");
    Serial.println(ultrasonicTest ? "✓ PASS" : "✗ FAIL");
    
    Serial.print("  Pump Relay:           ");
    Serial.println(pumpTest ? "✓ PASS" : "✗ FAIL");
    
    Serial.println();
    
    int passCount = i2cTest + ledTest + radarTest + ultrasonicTest + pumpTest;
    
    if (passCount == 5) {
        Serial.println("🎉 ALL TESTS PASSED!");
        Serial.println("   Your hardware is ready for main firmware.");
        
        // Victory blink
        for (int i = 0; i < 5; i++) {
            digitalWrite(STATUS_LED, HIGH);
            delay(100);
            digitalWrite(STATUS_LED, LOW);
            delay(100);
        }
    } else {
        Serial.println("⚠ SOME TESTS FAILED");
        Serial.print("   Passed: ");
        Serial.print(passCount);
        Serial.println("/5");
        Serial.println("   Review connections and try again.");
    }
    
    Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("Entering continuous monitoring mode...");
    Serial.println("Monitor RD-03 UART output and ultrasonic readings.");
    Serial.println("Press RESET to run tests again.");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}
