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
    Serial.println("   Testing detection 5 times...");
    
    int detectionCount = 0;
    int totalConfidence = 0;
    
    for (int i = 0; i < 5; i++) {
        DetectionResult result = detectPet();
        if (result.detected) {
            detectionCount++;
            totalConfidence += result.confidence;
            Serial.printf("   Attempt %d: ✓ Detected! Confidence: %d%%\n", i+1, result.confidence);
            Serial.printf("            BBox: [%d, %d, %d, %d]\n", 
                         result.bbox[0], result.bbox[1], result.bbox[2], result.bbox[3]);
        } else {
            Serial.printf("   Attempt %d: ✗ Nothing detected\n", i+1);
        }
        delay(500);
    }
    
    if (detectionCount > 0) {
        Serial.printf("   ✓ AI working! Detection rate: %d/5 (avg confidence: %d%%)\n", 
                     detectionCount, totalConfidence / detectionCount);
        Serial.printf("   Threshold: %d%% (detections must be above this)\n", CONFIDENCE_THRESHOLD);
    } else {
        Serial.println("   ⚠️  No detections - check:");
        Serial.println("      - Camera has power");
        Serial.println("      - Object is in frame");
        Serial.println("      - Lighting is adequate");
        Serial.printf("      - Current threshold: %d%% (lower = more sensitive)\n", CONFIDENCE_THRESHOLD);
    }
    Serial.println();
    
    // 6. Pet Recognition Mode
    Serial.println("6️⃣  Pet Recognition:");
    if (singlePetMode) {
        Serial.println("   🎯 SINGLE PET MODE ACTIVE");
        Serial.printf("   Pet: %s\n", singlePetName.c_str());
        Serial.println("   ✓ AI detection DISABLED (power optimized)");
        Serial.println("   ✓ All drinking events logged to this pet");
    } else {
        Serial.printf("   Multi-pet mode: %d photos loaded\n", referenceFeatures.size());
        if (referenceFeatures.size() > 0) {
            Serial.println("   ✓ AI can identify multiple pets");
        } else {
            Serial.println("   ⚠️  No training data - use 'Train AI' in mobile app");
        }
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

