/*
 * Smart Pet Fountain - Main Entry Point (Updated for RD-03 UART)
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
    
    // Print initial memory state
    Serial.println("\n📊 Initial Memory State:");
    Serial.printf("  Total heap: %d bytes\n", ESP.getHeapSize());
    Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  Total PSRAM: %d bytes\n", ESP.getPsramSize());
    Serial.printf("  Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.println();
    
    // Initialize hardware pins FIRST
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
     
     // Initialize subsystems in correct order
     initializeStorage();
     
     // Initialize I2C and AI Vision (I2C init happens inside initializeAIVision)
     initializeAIVision();
     
     // Connect to WiFi
     connectToWiFi();
     setupNTP();
     
     // Initialize Supabase client
     initializeSupabase();
     
     // Sync reference images from cloud
     syncReferenceImages();
     
     // Initialize sensors (RD-03 UART and ultrasonic)
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
 
void initializePins() {
    Serial.println("[INIT] Initializing hardware pins...");
    
    // Initialize GPIO pins
    pinMode(STATUS_LED, OUTPUT);
    pinMode(PUMP_RELAY, OUTPUT);
    pinMode(ULTRASONIC_TRIG, OUTPUT);
    pinMode(ULTRASONIC_ECHO, INPUT);
    
    // Set safe initial states
    digitalWrite(STATUS_LED, HIGH);  // LED on during initialization
    digitalWrite(PUMP_RELAY, LOW);   // Pump off
    digitalWrite(ULTRASONIC_TRIG, LOW);
    
    Serial.println("  ✓ GPIO pins initialized");
    
    // Note: I2C initialized in initializeAIVision()
    // Note: RD-03 UART initialized in initializeSensors()
}
 
 void blinkSuccess() {
     for (int i = 0; i < 3; i++) {
         digitalWrite(STATUS_LED, HIGH);
         delay(100);
         digitalWrite(STATUS_LED, LOW);
         delay(100);
     }
 }