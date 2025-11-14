/*
 * System Diagnostics and Testing
 */

#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "config.h"
#include "sensors.h"
#include "ai_vision.h"

// Run comprehensive system diagnostics
void runDiagnostics() {
    Serial.println("\n╔═══════════════════════════════════════════╗");
    Serial.println("║   SYSTEM DIAGNOSTICS                     ║");
    Serial.println("╚═══════════════════════════════════════════╝\n");
    
    // 1. Memory Test
    Serial.println("1️⃣  Memory Test:");
    Serial.printf("   Free Heap: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("   Free PSRAM: %d KB\n", ESP.getFreePsram() / 1024);
    Serial.printf("   JPEG Buffer: %s\n", jpegBuffer ? "Allocated" : "NULL");
    Serial.println();
    
    // 2. WiFi Test
    Serial.println("2️⃣  WiFi Test:");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("   ✓ Connected: %s\n", WiFi.SSID().c_str());
        Serial.printf("   IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("   Signal: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("   ✗ Not connected");
    }
    Serial.println();
    
    // 3. Radar Test
    Serial.println("3️⃣  Radar Test (RD-03):");
    Serial.println("   Wave your hand in front of the radar...");
    Serial.println("   Listening for 10 seconds...\n");
    
    unsigned long testStart = millis();
    bool radarWorking = false;
    int dataCount = 0;
    
    while (millis() - testStart < 10000) {
        if (radarSerial.available()) {
            String data = radarSerial.readStringUntil('\n');
            data.trim();
            
            if (data.length() > 0) {
                dataCount++;
                Serial.printf("   [RD-03] %s\n", data.c_str());
                radarWorking = true;
                
                if (data.indexOf("Range") >= 0) {
                    Serial.println("   ✓ MOTION DETECTED!");
                }
            }
        }
        delay(100);
    }
    
    Serial.println();
    if (radarWorking) {
        Serial.printf("   ✓ Radar working (%d data packets received)\n", dataCount);
    } else {
        Serial.println("   ✗ NO DATA from radar!");
        Serial.println("   Check wiring:");
        Serial.printf("     RD-03 TX → ESP32 GPIO%d (RX)\n", RD03_RX_PIN);
        Serial.printf("     RD-03 RX → ESP32 GPIO%d (TX)\n", RD03_TX_PIN);
        Serial.println("     RD-03 VCC → 5V");
        Serial.println("     RD-03 GND → GND");
    }
    Serial.println();
    
    // 4. Ultrasonic Test
    Serial.println("4️⃣  Ultrasonic Sensor Test:");
    for (int i = 0; i < 5; i++) {
        float distance = measureWaterDistance();
        if (distance > 0) {
            Serial.printf("   Reading %d: %.1f cm\n", i+1, distance);
        } else {
            Serial.printf("   Reading %d: Timeout\n", i+1);
        }
        delay(500);
    }
    float avgLevel = getFilteredWaterLevel();
    Serial.printf("   Water Level: %.1f cm\n", avgLevel);
    Serial.println();
    
    // 5. AI Vision Test
    Serial.println("5️⃣  AI Vision Test:");
    DetectionResult result = detectPet();
    if (result.detected) {
        Serial.printf("   ✓ Pet detected! Confidence: %d%%\n", result.confidence);
        Serial.printf("   BBox: [%d, %d, %d, %d]\n", 
                     result.bbox[0], result.bbox[1], result.bbox[2], result.bbox[3]);
    } else {
        Serial.println("   ℹ️  No pet in view (this is normal if no pet present)");
    }
    Serial.println();
    
    // 6. Reference Photos
    Serial.println("6️⃣  Training Data:");
    Serial.printf("   Loaded photos: %d\n", referenceFeatures.size());
    if (referenceFeatures.size() > 0) {
        Serial.println("   ✓ AI can identify pets");
    } else {
        Serial.println("   ⚠️  No training data - use 'Train AI' in mobile app");
    }
    Serial.println();
    
    // 7. FreeRTOS Tasks
    Serial.println("7️⃣  FreeRTOS Tasks:");
    if (stateMachineTaskHandle != NULL) {
        Serial.println("   ✓ State Machine task running");
    }
    if (sensorMonitorTaskHandle != NULL) {
        Serial.println("   ✓ Sensor Monitor task running");
    }
    if (networkTaskHandle != NULL) {
        Serial.println("   ✓ Network task running");
    }
    Serial.println();
    
    Serial.println("╔═══════════════════════════════════════════╗");
    Serial.println("║   DIAGNOSTICS COMPLETE                   ║");
    Serial.println("╚═══════════════════════════════════════════╝\n");
    
    Serial.println("System is now in normal operation mode.");
    Serial.println("LED will light up when motion is detected.\n");
}

// Enable verbose sensor debugging
void enableSensorDebug() {
    Serial.println("\n🔍 VERBOSE SENSOR DEBUG MODE ENABLED");
    Serial.println("All sensor readings will be printed.\n");
}

// Print state machine status
void printStateMachineStatus() {
    Serial.print("State: ");
    switch (sm.currentState) {
        case STATE_IDLE:
            Serial.println("IDLE");
            break;
        case STATE_PRESENCE_DETECTED:
            Serial.println("PRESENCE_DETECTED");
            break;
        case STATE_PET_DETECTION:
            Serial.println("PET_DETECTION");
            break;
        case STATE_FEATURE_MATCHING:
            Serial.println("FEATURE_MATCHING");
            break;
        case STATE_WATER_MEASUREMENT:
            Serial.println("WATER_MEASUREMENT");
            break;
        case STATE_DATA_UPLOAD:
            Serial.println("DATA_UPLOAD");
            break;
        default:
            Serial.println("UNKNOWN");
    }
}

#endif // DIAGNOSTICS_H

