/*
 * Sensor Management
 * RD-03 radar presence detection (UART) and ultrasonic water level
 */

#ifndef SENSORS_H
#define SENSORS_H

#include "config.h"

// RD-03 Radar Serial
HardwareSerial radarSerial(1);  // Use UART1 for RD-03

// Global sensor state
bool motionDetected = false;
unsigned long lastMotionTime = 0;
int detectedRange = -1;
float waterLevelReadings[WATER_READING_SAMPLES] = {0};
int waterReadIndex = 0;
bool verboseSensorMode = false;  // Set to true for troubleshooting
unsigned long lastRadarPrint = 0;

void initializeSensors() {
    Serial.println("Initializing sensors...");
    
    // Initialize RD-03 Radar UART
    radarSerial.begin(RD03_BAUD_RATE, SERIAL_8N1, RD03_RX_PIN, RD03_TX_PIN);
    radarSerial.setRxBufferSize(1024);
    delay(100);
    
    // Initialize Ultrasonic sensor
    pinMode(ULTRASONIC_TRIG, OUTPUT);
    pinMode(ULTRASONIC_ECHO, INPUT);
    digitalWrite(ULTRASONIC_TRIG, LOW);
    
    Serial.println("✓ Sensors initialized");
    Serial.println("  - RD-03 Radar on UART1 (115200 baud)");
    Serial.println("  - Ultrasonic sensor ready");
}

bool checkPresence() {
    // Print "no data" message every 30 seconds if radar is silent
    if (millis() - lastRadarPrint > 30000 && radarSerial.available() == 0) {
        Serial.println("⚠️  No data from RD-03 radar in 30 seconds");
        Serial.println("   Check wiring and power to radar sensor");
        lastRadarPrint = millis();
    }
    
    // Read data from RD-03 radar
    if (radarSerial.available()) {
        String radarData = radarSerial.readStringUntil('\n');
        radarData.trim();
        
        lastRadarPrint = millis();  // Reset silence timer
        
        // Always print raw data in verbose mode
        if (verboseSensorMode && radarData.length() > 0) {
            Serial.printf("[RD-03 RAW] %s\n", radarData.c_str());
        }
        
        // Parse RD-03 output (typical formats: "Range X" or "None")
        if (radarData.indexOf("Range") >= 0) {
            // Target detected
            int rangeStart = radarData.indexOf("Range") + 5;
            String rangeStr = radarData.substring(rangeStart);
            detectedRange = rangeStr.toInt();
            
            lastMotionTime = millis();
            if (!motionDetected) {
                Serial.printf("📡 [RD-03] Target detected at range %d\n", detectedRange);
            }
            motionDetected = true;
        } else if (radarData.indexOf("None") >= 0 || radarData.indexOf("none") >= 0) {
            // No target
            if (motionDetected) {
                Serial.println("📡 [RD-03] Target cleared");
            }
            motionDetected = false;
            detectedRange = -1;
        }
    }
    
    // Timeout check - if no data for 2 seconds, consider no motion
    if (millis() - lastMotionTime > 2000 && motionDetected) {
        motionDetected = false;
        detectedRange = -1;
        if (verboseSensorMode) {
            Serial.println("[RD-03] Timeout - no motion");
        }
    }
    
    return motionDetected;
}

int getDetectedRange() {
    return detectedRange;
}
 
 float measureWaterDistance() {
     digitalWrite(ULTRASONIC_TRIG, LOW);
     delayMicroseconds(2);
     digitalWrite(ULTRASONIC_TRIG, HIGH);
     delayMicroseconds(10);
     digitalWrite(ULTRASONIC_TRIG, LOW);
     
     long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
     
     if (duration == 0) {
 #ifdef DEBUG_SENSORS
         Serial.println("[Ultrasonic] Timeout");
 #endif
         return -1;
     }
     
     float distance = duration * 0.034 / 2;
     
 #ifdef DEBUG_SENSORS
     Serial.printf("[Ultrasonic] Distance: %.1f cm\n", distance);
 #endif
     
     return distance;
 }
 
 float getFilteredWaterLevel() {
     waterLevelReadings[waterReadIndex] = measureWaterDistance();
     waterReadIndex = (waterReadIndex + 1) % WATER_READING_SAMPLES;
     
     float sum = 0;
     int validCount = 0;
     
     for (int i = 0; i < WATER_READING_SAMPLES; i++) {
         if (waterLevelReadings[i] > 0) {
             sum += waterLevelReadings[i];
             validCount++;
         }
     }
     
     if (validCount == 0) return -1;
     
     float avgDistance = sum / validCount;
     float waterLevel = TANK_HEIGHT_CM - avgDistance;
     
     if (waterLevel < 0) waterLevel = 0;
     if (waterLevel > TANK_HEIGHT_CM) waterLevel = TANK_HEIGHT_CM;
     
     return waterLevel;
 }
 
 int calculateWaterVolume(float levelCm) {
     if (levelCm < 0) return 0;
     
     // Volume = Area × Height
     float volumeCm3 = BOWL_AREA_CM2 * levelCm;
     
     return (int)volumeCm3;
 }
 
 int calculateConsumption(float initialLevel, float finalLevel) {
     int initialVol = calculateWaterVolume(initialLevel);
     int finalVol = calculateWaterVolume(finalLevel);
     
     int consumed = initialVol - finalVol;
     
     if (consumed < 0) consumed = 0;
     if (consumed > 500) consumed = 500;
     
     return consumed;
 }
 
void checkLowWater() {
    float waterLevel = getFilteredWaterLevel();
    
    if (waterLevel > 0 && waterLevel < LOW_WATER_THRESHOLD_CM) {
        Serial.println("⚠️  Low water level!");
        
        // Flash LED to indicate low water
        for (int i = 0; i < 3; i++) {
            digitalWrite(STATUS_LED, HIGH);
            delay(100);
            digitalWrite(STATUS_LED, LOW);
            delay(100);
        }
    }
}
 
 #endif // SENSORS_H