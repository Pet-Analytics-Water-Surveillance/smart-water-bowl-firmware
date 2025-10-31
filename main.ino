/*
 * Smart Pet Fountain - Main Entry Point
 * 
 * Cloud-First Architecture v2.0
 * - BLE provisioning on first boot only
 * - WiFi operation for normal use
 * - Real-time events pushed to Supabase
 */

 #include "config.h"
 #include "ble_provisioning.h"
 #include "wifi_manager.h"
 #include "ai_vision.h"
 #include "feature_matching.h"
 #include "sensors.h"
 #include "supabase_client.h"
 #include "state_machine.h"
 
 // Global state
 bool deviceProvisioned = false;
 
 void setup() {
     Serial.begin(115200);
     delay(2000);
     
     printWelcome();
     
     // Initialize hardware pins
     initializePins();
     
     // Check if device is provisioned
     deviceProvisioned = checkProvisioningStatus();
     
     if (!deviceProvisioned) {
         Serial.println("\n⚠️  DEVICE NOT PROVISIONED");
         Serial.println("Starting BLE provisioning mode...\n");
         
         // Start BLE provisioning (blocks until complete)
         startBLEProvisioning();
         
         // After provisioning, restart to enter normal operation
         Serial.println("\n✓ Provisioning complete!");
         Serial.println("Restarting in 3 seconds...");
         delay(3000);
         ESP.restart();
     }
     
     // Normal operation mode
     Serial.println("\n✓ Device provisioned - entering normal operation\n");
     
     // Initialize subsystems
     initializeStorage();
     initializeAIVision();
     
     // Connect to WiFi
     connectToWiFi();
     setupNTP();
     
     // Initialize Supabase client
     initializeSupabase();
     
     // Sync reference images from cloud
     syncReferenceImages();
     
     // Initialize sensors
     initializeSensors();
     
     // Create FreeRTOS tasks for concurrent operation
     createSystemTasks();
     
     digitalWrite(STATUS_LED, LOW);
     Serial.println("\n✓ System initialized and ready\n");
     blinkSuccess();
 }
 
 void loop() {
     // All work happens in FreeRTOS tasks
     vTaskDelay(portMAX_DELAY);
 }
 
 void printWelcome() {
     Serial.println("\n\n");
     Serial.println("╔═══════════════════════════════════════════╗");
     Serial.println("║   SMART PET FOUNTAIN v2.0                ║");
     Serial.println("║   Real-Time Cloud Architecture           ║");
     Serial.println("╚═══════════════════════════════════════════╝");
     Serial.println();
 }
 
void initializePins() {
    pinMode(STATUS_LED, OUTPUT);
    pinMode(PUMP_RELAY, OUTPUT);
    pinMode(ULTRASONIC_TRIG, OUTPUT);
    pinMode(ULTRASONIC_ECHO, INPUT);
    
    digitalWrite(STATUS_LED, HIGH);
    digitalWrite(PUMP_RELAY, LOW);
    
    // Note: RD-03 uses hardware serial (UART1), initialized in sensors.h
}
 
 void blinkSuccess() {
     for (int i = 0; i < 3; i++) {
         digitalWrite(STATUS_LED, HIGH);
         delay(100);
         digitalWrite(STATUS_LED, LOW);
         delay(100);
     }
 }