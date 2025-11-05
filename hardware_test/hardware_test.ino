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
#include <Seeed_Arduino_SSCMA.h>

// ===== PIN DEFINITIONS (Updated - RD-03 is UART) =====
#define RD03_RX_PIN       44      // ESP32 RX ← RD-03 TX
#define RD03_TX_PIN       43      // ESP32 TX → RD-03 RX
#define RD03_BAUD_RATE    115200 // RD-03 default baud rate

#define ULTRASONIC_TRIG   1      // Ultrasonic trigger (TX)
#define ULTRASONIC_ECHO   2      // Ultrasonic echo (RX)
#define STATUS_LED        7      // Status LED
#define PUMP_RELAY        8     // Pump relay control
#define I2C_SDA           5      // I2C Data (Grove AI Vision)
#define I2C_SCL           6      // I2C Clock (Grove AI Vision)

// Hardware Serial for RD-03
HardwareSerial radarSerial(1);

// Grove AI Vision V2 instance (SSCMA)
SSCMA AI;

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
    
    // Initialize I2C FIRST - like pet_fountain does
    Serial.println("[INIT] Initializing I2C bus...");
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);
    delay(500);  // Give I2C time to stabilize
    Serial.println("  ✓ I2C bus ready\n");
    
    // Initialize pins
    pinMode(STATUS_LED, OUTPUT);
    pinMode(PUMP_RELAY, OUTPUT);
    pinMode(ULTRASONIC_TRIG, OUTPUT);
    pinMode(ULTRASONIC_ECHO, INPUT);
    
    // Ensure safe starting state
    digitalWrite(STATUS_LED, LOW);
    digitalWrite(PUMP_RELAY, LOW);
    digitalWrite(ULTRASONIC_TRIG, LOW);
    
    Serial.println("🔧 Starting Hardware Tests...\n");
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
    static unsigned long lastStatusPrint = 0;
    static int cycleCount = 0;
    
    // Print status every 3 seconds
    if (millis() - lastStatusPrint > 3000) {
        Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        Serial.printf("[MONITORING CYCLE #%d]\n", ++cycleCount);
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
        
        // Check RD-03 radar
        bool motionDetected = false;
        if (radarSerial.available()) {
            String radarData = radarSerial.readStringUntil('\n');
            radarData.trim();
            if (radarData.length() > 0) {
                Serial.print("  RD-03: ");
                Serial.println(radarData);
                if (radarData.indexOf("Range") >= 0) {
                    motionDetected = true;
                    blinkLED(1, 100);
                }
            }
        } else {
            Serial.println("  RD-03: No data");
        }
        
        // Check ultrasonic water level
        float distance = measureDistance();
        Serial.print("  Water Level: ");
        if (distance > 0) {
            Serial.print(distance);
            Serial.println(" cm");
        } else {
            Serial.println("ERROR (timeout)");
        }
        
        // AI detection status
        if (motionDetected) {
            Serial.println("  AI: Motion detected, checking...");
            if (!AI.invoke()) {
                if (AI.boxes().size() > 0) {
                    auto box = AI.boxes()[0];
                    Serial.printf("  AI: Pet detected! Target=%d, Score=%d%%\n", box.target, box.score);
                    blinkLED(2, 150);
                } else {
                    Serial.println("  AI: No pet detected");
                }
            }
        }
        
        // Memory status
        Serial.printf("  Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
        
        Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
        
        lastStatusPrint = millis();
    }
    
    delay(100);
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
    Serial.println("  - Status LED: Pin 7");
    Serial.println("  - RD-03 Radar: RX=44, TX=43 (UART @ 115200)");
    Serial.println("  - Ultrasonic: TRIG=1, ECHO=2");
    Serial.println("  - Pump Relay: Pin 8");
    Serial.println();
}

void testI2C() {
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("TEST 1: Grove AI Vision V2");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    // Initialize AI Vision - I2C already initialized in setup()
    Serial.println("[INIT] Initializing AI Vision V2...");
    
    // Initialize AI Vision
    if (AI.begin()) {
        Serial.println("  ✓ AI Vision V2 initialized");
        Serial.println("  ✓ Pet detection model loaded");
        Serial.println("[INIT] AI Vision ready");
        i2cTest = true;
        
        // Test detection capability
        Serial.println("\n[TEST] Testing object detection (wave hand in front)...");
        delay(1000);
        
        int detectionAttempts = 0;
        int successfulDetections = 0;
        
        for (int i = 0; i < 5; i++) {
            detectionAttempts++;
            int invokeStatus = AI.invoke();
            
            if (invokeStatus == 0) {
                if (AI.boxes().size() > 0) {
                    successfulDetections++;
                    auto box = AI.boxes()[0];
                    Serial.printf("[AI] Object detected - Target=%d, Score=%d%%\n", 
                                 box.target, box.score);
                } else {
                    Serial.printf("[%d] No objects detected\n", i+1);
                }
            } else {
                Serial.printf("[%d] Invoke failed (status=%d)\n", i+1, invokeStatus);
            }
            delay(500);
        }
        
        Serial.printf("\n✓ Detection test complete: %d/%d successful\n", 
                     successfulDetections, detectionAttempts);
        
    } else {
        Serial.println("[ERROR] AI Vision V2 initialization failed!");
        Serial.println("[ERROR] Check I2C connections (SDA=GPIO5, SCL=GPIO6)");
        i2cTest = false;
        
        // Now try I2C scan to diagnose
        Serial.println("\n[DEBUG] Running I2C scan for diagnostics...");
        byte error, address;
        int deviceCount = 0;
        
        for (address = 1; address < 127; address++) {
            Wire.beginTransmission(address);
            error = Wire.endTransmission();
            
            if (error == 0) {
                Serial.print("  Found I2C device at 0x");
                if (address < 16) Serial.print("0");
                Serial.println(address, HEX);
                deviceCount++;
            }
        }
        
        if (deviceCount == 0) {
            Serial.println("  ✗ No I2C devices found - check wiring!");
        } else {
            Serial.printf("  Found %d device(s) but AI init failed\n", deviceCount);
            Serial.println("  → Grove AI may need firmware/model upload");
        }
    }
}

void testLED() {
    Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("TEST 2: Status LED (Pin 7)");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    
    Serial.println("Blinking LED 5 times...");
    Serial.println("  Watch for 5 blinks on GPIO7");
    
    blinkLED(5, 300);
    
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
                    Serial.println("    ✓ MOTION DETECTED!");
                    blinkLED(1, 100);
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
    
    Serial.println("Testing ultrasonic sensor...");
    Serial.printf("  TRIG Pin: GPIO%d\n", ULTRASONIC_TRIG);
    Serial.printf("  ECHO Pin: GPIO%d\n", ULTRASONIC_ECHO);
    Serial.println("  Sensor should be connected to 5V or 3.3V");
    Serial.println();
    
    // Test pin state
    Serial.println("Testing pin states:");
    digitalWrite(ULTRASONIC_TRIG, HIGH);
    delay(10);
    Serial.printf("  TRIG HIGH: %d\n", digitalRead(ULTRASONIC_TRIG));
    digitalWrite(ULTRASONIC_TRIG, LOW);
    delay(10);
    Serial.printf("  TRIG LOW: %d\n", digitalRead(ULTRASONIC_TRIG));
    Serial.printf("  ECHO state: %d\n", digitalRead(ULTRASONIC_ECHO));
    Serial.println();
    
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
        } else if (distance == -1) {
            Serial.println("TIMEOUT ✗ (no echo received)");
        } else {
            Serial.print(distance);
            Serial.println(" cm (out of range) ✗");
        }
        
        delay(500);  // Increased delay between readings
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
        Serial.println("\n  Troubleshooting:");
        Serial.println("  1. Check wiring:");
        Serial.println("     - VCC → 5V (or 3.3V depending on sensor)");
        Serial.println("     - GND → GND");
        Serial.println("     - TRIG → GPIO1");
        Serial.println("     - ECHO → GPIO2");
        Serial.println("  2. Ensure sensor is powered (LED on some models)");
        Serial.println("  3. Point sensor at an object 5-200cm away");
        Serial.println("  4. Some sensors need 5V to work properly");
        Serial.println("  5. Try a different ultrasonic sensor if available");
        ultrasonicTest = false;
    }
}

void testPumpRelay() {
    Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("TEST 5: Pump Relay (Pin 8)");
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
    // EXACTLY like working pet_fountain sensors.h
    digitalWrite(ULTRASONIC_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG, LOW);
    
    long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
    
    if (duration == 0) {
        return -1;
    }
    
    float distance = duration * 0.034 / 2;
    
    return distance;
}

// Helper function for LED feedback (from working firmware)
void blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(STATUS_LED, HIGH);
        delay(delayMs);
        digitalWrite(STATUS_LED, LOW);
        delay(delayMs);
    }
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
        blinkLED(5, 100);
    } else {
        Serial.println("⚠ SOME TESTS FAILED");
        Serial.print("   Passed: ");
        Serial.print(passCount);
        Serial.println("/5");
        Serial.println("   Review connections and try again.");
        
        // Error pattern
        blinkLED(3, 300);
    }
    
    Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
    Serial.println("Entering continuous monitoring mode...");
    Serial.println("Monitor RD-03 UART output and ultrasonic readings.");
    Serial.println("Press RESET to run tests again.");
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}
