/*
 * Sensor Management
 * Radar presence detection and ultrasonic water level
 */

 #ifndef SENSORS_H
 #define SENSORS_H
 
 #include "config.h"
 
 // Global sensor state
 bool motionDetected = false;
 unsigned long lastMotionTime = 0;
 float waterLevelReadings[WATER_READING_SAMPLES] = {0};
 int waterReadIndex = 0;
 
 void initializeSensors() {
     Serial.println("Initializing sensors...");
     
     pinMode(RD03_OUT, INPUT);
     pinMode(ULTRASONIC_TRIG, OUTPUT);
     pinMode(ULTRASONIC_ECHO, INPUT);
     digitalWrite(ULTRASONIC_TRIG, LOW);
     
     Serial.println("✓ Sensors initialized");
 }
 
 bool checkPresence() {
     bool currentMotion = digitalRead(RD03_OUT);
     
     if (currentMotion) {
         lastMotionTime = millis();
         motionDetected = true;
 #ifdef DEBUG_SENSORS
         if (!motionDetected) {
             Serial.println("[Radar] Motion detected");
         }
 #endif
     } else if (millis() - lastMotionTime > 2000) {
         if (motionDetected) {
 #ifdef DEBUG_SENSORS
             Serial.println("[Radar] Motion cleared");
 #endif
         }
         motionDetected = false;
     }
     
     return motionDetected;
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
     
     float radius = TANK_DIAMETER_CM / 2.0;
     float volumeCm3 = PI * radius * radius * levelCm;
     
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
         
         digitalWrite(BUZZER, HIGH);
         delay(100);
         digitalWrite(BUZZER, LOW);
     }
 }
 
 #endif // SENSORS_H