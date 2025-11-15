/*
 * State Machine and FreeRTOS Tasks
 */

 #ifndef STATE_MACHINE_H
 #define STATE_MACHINE_H
 
 #include "config.h"
 #include "sensors.h"
 #include "ai_vision.h"
 #include "feature_matching.h"
 #include "supabase_client.h"
 #include "wifi_manager.h"
 
 // State definitions
 typedef enum {
     STATE_IDLE,
     STATE_PRESENCE_DETECTED,
     STATE_PET_DETECTION,
     STATE_FEATURE_MATCHING,
     STATE_WATER_MEASUREMENT,
     STATE_DATA_UPLOAD
 } SystemState;
 
// External variables for single pet mode
extern bool singlePetMode;
extern String singlePetId;
extern String singlePetName;

// State machine context
struct StateMachineContext {
    SystemState currentState;
    unsigned long stateEntryTime;
    String identifiedPet;
    float initialWaterLevel;
    float finalWaterLevel;
    DetectionResult lastDetection;
};

StateMachineContext sm = {STATE_IDLE, 0, "", 0, 0, {false, 0, {0,0,0,0}, 0}};
 
// Task handles
TaskHandle_t stateMachineTaskHandle;
TaskHandle_t sensorMonitorTaskHandle;
TaskHandle_t networkTaskHandle;

// Status printing
unsigned long lastStatusPrint = 0;
unsigned long lastDetectionAttempt = 0;
 
 // ===== STATE HANDLERS =====
 
void handleStateIdle() {
    // Print status every 10 seconds while idle
    if (millis() - lastStatusPrint > 10000) {
        Serial.println("💤 Idle - Waiting for motion...");
        lastStatusPrint = millis();
    }
    
    if (checkPresence()) {
        Serial.println("\n🚨 MOTION DETECTED!");
        
        // Check if we're in single pet mode
        if (isSinglePetMode()) {
            Serial.println("→ STATE: WATER_MEASUREMENT (Single Pet Mode)");
            Serial.printf("  Pet: %s\n", singlePetName.c_str());
            
            // Skip AI detection and go straight to water measurement
            sm.identifiedPet = getSinglePetId();
            
            // Measure initial water level
            Serial.println("  📏 Measuring initial water level...");
            float readings[5];
            for (int i = 0; i < 5; i++) {
                readings[i] = getFilteredWaterLevel();
                delay(200);
            }
            float sum = 0;
            for (int i = 0; i < 5; i++) {
                sum += readings[i];
            }
            sm.initialWaterLevel = sum / 5.0;
            Serial.printf("  Initial water: %.1f cm\n", sm.initialWaterLevel);
            
            // Turn ON pump for pet to drink
            digitalWrite(PUMP_RELAY, HIGH);
            Serial.println("  💧 Pump ON - Water flowing!");
            
            sm.currentState = STATE_WATER_MEASUREMENT;
            sm.stateEntryTime = millis();
        } else {
            // Multi-pet mode - need AI detection
            Serial.println("→ STATE: PRESENCE_DETECTED");
            sm.currentState = STATE_PRESENCE_DETECTED;
            sm.stateEntryTime = millis();
        }
        
        digitalWrite(STATUS_LED, HIGH);
    }
}
 
void handleStatePresenceDetected() {
    DetectionResult result = detectPet();
    
    if (result.detected) {
        Serial.println("→ STATE: PET_DETECTION");
        Serial.printf("  ✓ AI detected object! Confidence: %d%%\n", result.confidence);
        
        sm.currentState = STATE_PET_DETECTION;
        sm.stateEntryTime = millis();
    } else {
        // Print status every second while waiting
        if (millis() - lastDetectionAttempt > 1000) {
            unsigned long elapsed = millis() - sm.stateEntryTime;
            Serial.printf("  🔍 AI checking... (%d/%d sec) - No object detected yet\n", 
                         elapsed/1000, PRESENCE_TIMEOUT_MS/1000);
            lastDetectionAttempt = millis();
        }
    }
    
    if (millis() - sm.stateEntryTime > PRESENCE_TIMEOUT_MS) {
        Serial.println("→ STATE: IDLE (timeout - AI didn't detect anything)");
        Serial.println("  💡 TIP: Make sure camera has clear view and object is in frame");
        Serial.printf("  Current confidence threshold: %d%%\n", CONFIDENCE_THRESHOLD);
        sm.currentState = STATE_IDLE;
        digitalWrite(STATUS_LED, LOW);
    }
}
 
 void handleStatePetDetection() {
     DetectionResult result = detectPetWithImage();
     
     if (result.detected && result.imageSize > 0) {
         Serial.println("→ STATE: FEATURE_MATCHING");
         Serial.printf("  Image captured: %d bytes\n", result.imageSize);
         
         sm.lastDetection = result;
         sm.currentState = STATE_FEATURE_MATCHING;
     } else {
         Serial.println("→ STATE: IDLE (capture failed)");
         sm.currentState = STATE_IDLE;
         digitalWrite(STATUS_LED, LOW);
     }
 }
 
void handleStateFeatureMatching() {
    extern uint8_t* jpegBuffer;
    
    sm.identifiedPet = identifyPet(jpegBuffer, sm.lastDetection.imageSize);
    
    // Take multiple stable measurements for initial level
    Serial.println("  📏 Measuring initial water level...");
    float readings[5];
    for (int i = 0; i < 5; i++) {
        readings[i] = getFilteredWaterLevel();
        delay(200);  // Wait between readings
    }
    
    // Use median of 5 readings for stability
    float sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += readings[i];
    }
    sm.initialWaterLevel = sum / 5.0;
    
    Serial.printf("  Initial water: %.1f cm\n", sm.initialWaterLevel);
    
    Serial.println("→ STATE: WATER_MEASUREMENT");
    
    // Turn ON pump for pet to drink
    digitalWrite(PUMP_RELAY, HIGH);
    Serial.println("  💧 Pump ON - Water flowing for pet to drink!");
    
    sm.currentState = STATE_WATER_MEASUREMENT;
    sm.stateEntryTime = millis();
}
 
void handleStateWaterMeasurement() {
    // Check if pet left or timeout reached
    if (!checkPresence() || millis() - sm.stateEntryTime > DRINKING_TIMEOUT_MS) {
        // Turn OFF pump when pet leaves
        digitalWrite(PUMP_RELAY, LOW);
        Serial.println("  💧 Pump OFF - Pet left");
        
        // Wait a moment for water to settle before measuring
        delay(1000);
        
        // Take multiple stable measurements for final level
        Serial.println("  📏 Measuring final water level...");
        float readings[5];
        for (int i = 0; i < 5; i++) {
            readings[i] = getFilteredWaterLevel();
            delay(200);  // Wait between readings
        }
        
        // Use median of 5 readings for stability
        float sum = 0;
        for (int i = 0; i < 5; i++) {
            sum += readings[i];
        }
        sm.finalWaterLevel = sum / 5.0;
        
        int consumed = calculateConsumption(sm.initialWaterLevel, sm.finalWaterLevel);
        
        Serial.printf("  Final water: %.1f cm\n", sm.finalWaterLevel);
        Serial.printf("  Water consumed: %d ml\n", consumed);
        
        // More lenient consumption check - accept even small amounts
        if (consumed > 0 && consumed < 500) {
            Serial.println("→ STATE: DATA_UPLOAD");
            sm.currentState = STATE_DATA_UPLOAD;
        } else if (consumed == 0) {
            // If 0ml but water level dropped (within noise), still proceed
            float levelDrop = sm.initialWaterLevel - sm.finalWaterLevel;
            if (levelDrop > 0.1) {  // At least 0.1cm drop
                Serial.printf("  ℹ️  Small level drop detected (%.2f cm), proceeding...\n", levelDrop);
                Serial.println("→ STATE: DATA_UPLOAD");
                sm.currentState = STATE_DATA_UPLOAD;
            } else {
                Serial.println("→ STATE: IDLE (no consumption detected)");
                Serial.printf("  ⚠️  Level change: %.2f cm (initial) -> %.2f cm (final)\n", 
                             sm.initialWaterLevel, sm.finalWaterLevel);
                sm.currentState = STATE_IDLE;
                digitalWrite(STATUS_LED, LOW);
            }
        } else {
            Serial.println("→ STATE: IDLE (invalid consumption)");
            Serial.printf("  ⚠️  Consumed: %d ml (out of valid range)\n", consumed);
            sm.currentState = STATE_IDLE;
            digitalWrite(STATUS_LED, LOW);
        }
    } else {
        // While pet is still present, show status every 2 seconds
        static unsigned long lastPrintTime = 0;
        if (millis() - lastPrintTime > 2000) {
            unsigned long elapsed = (millis() - sm.stateEntryTime) / 1000;
            Serial.printf("  💧 Pet drinking... (%lu sec, pump running)\n", elapsed);
            lastPrintTime = millis();
        }
    }
}
 
void handleStateDataUpload() {
    if (WiFi.status() == WL_CONNECTED) {
        int waterConsumed = calculateConsumption(sm.initialWaterLevel, sm.finalWaterLevel);
        String timestamp = getISOTimestamp();
        
        bool success = insertDrinkingEvent(sm.identifiedPet, waterConsumed, timestamp);
        
        if (success) {
            Serial.println("  ✓ Data uploaded successfully");
            for (int i = 0; i < 3; i++) {
                digitalWrite(STATUS_LED, HIGH);
                delay(100);
                digitalWrite(STATUS_LED, LOW);
                delay(100);
            }
        }
    } else {
        Serial.println("[Upload] ✗ WiFi not connected");
    }
    
    Serial.println("→ STATE: IDLE\n");
    sm.currentState = STATE_IDLE;
    sm.identifiedPet = "";
    digitalWrite(STATUS_LED, LOW);
}
 
 // ===== STATE MACHINE PROCESSOR =====
 
 void processStateMachine() {
     switch (sm.currentState) {
         case STATE_IDLE:
             handleStateIdle();
             break;
             
         case STATE_PRESENCE_DETECTED:
             handleStatePresenceDetected();
             break;
             
         case STATE_PET_DETECTION:
             handleStatePetDetection();
             break;
             
         case STATE_FEATURE_MATCHING:
             handleStateFeatureMatching();
             break;
             
         case STATE_WATER_MEASUREMENT:
             handleStateWaterMeasurement();
             break;
             
         case STATE_DATA_UPLOAD:
             handleStateDataUpload();
             break;
     }
 }
 
 // ===== FREERTOS TASKS =====
 
 void stateMachineTask(void* parameter) {
     for (;;) {
         processStateMachine();
         vTaskDelay(pdMS_TO_TICKS(100));
     }
 }
 
 void sensorMonitorTask(void* parameter) {
     for (;;) {
         checkPresence();
         checkLowWater();
         vTaskDelay(pdMS_TO_TICKS(200));
     }
 }
 
 void networkTask(void* parameter) {
     unsigned long lastSyncTime = 0;
     
     for (;;) {
         maintainWiFiConnection();
         
         if (millis() - lastSyncTime > SYNC_INTERVAL_MS) {
             syncReferenceImages();
             lastSyncTime = millis();
         }
         
         vTaskDelay(pdMS_TO_TICKS(5000));
     }
 }
 
 // ===== TASK CREATION =====
 
void createSystemTasks() {
    Serial.println("Creating FreeRTOS tasks...");
    
    // Print available memory before task creation
    Serial.printf("Memory available: %d KB heap, %d KB PSRAM\n", 
                  ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);
    
    // Increased stack sizes for stability
    xTaskCreatePinnedToCore(
        stateMachineTask,
        "StateMachine",
        10240,  // Increased from 8192 to 10KB
        NULL,
        2,
        &stateMachineTaskHandle,
        1
    );
    
    xTaskCreatePinnedToCore(
        sensorMonitorTask,
        "Sensors",
        4096,
        NULL,
        1,
        &sensorMonitorTaskHandle,
        1
    );
    
    xTaskCreatePinnedToCore(
        networkTask,
        "Network",
        12288,  // Increased from 8192 to 12KB (needs more for HTTP/JSON)
        NULL,
        1,
        &networkTaskHandle,
        0
    );
    
    Serial.println("✓ FreeRTOS tasks created");
    Serial.printf("Memory after tasks: %d KB heap\n", ESP.getFreeHeap() / 1024);
}
 
 #endif // STATE_MACHINE_H