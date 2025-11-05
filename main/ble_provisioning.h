/*
 * BLE Provisioning Module
 * 
 * Used ONLY for initial device setup
 * After provisioning, BLE is disabled to save memory
 */

 #ifndef BLE_PROVISIONING_H
 #define BLE_PROVISIONING_H
 
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "config.h"
 
 // Global BLE objects
 NimBLEServer* pServer = nullptr;
 NimBLECharacteristic* pStatusChar = nullptr;
 bool bleConnected = false;
 bool provisioningComplete = false;
 
// Credentials received via BLE
String receivedWiFiSSID = "";
String receivedWiFiPassword = "";
String receivedSupabaseURL = "";
String receivedSupabaseKey = "";
String receivedUserID = "";

// ===== FORWARD DECLARATIONS =====
void updateBLEStatus(String status);
void saveProvisioningData();

// ===== BLE SERVER CALLBACKS =====
 
 class ServerCallbacks: public NimBLEServerCallbacks {
     void onConnect(NimBLEServer* pServer) {
         bleConnected = true;
         Serial.println("📱 BLE Client connected");
         updateBLEStatus("connected");
     }
 
     void onDisconnect(NimBLEServer* pServer) {
         bleConnected = false;
         Serial.println("📱 BLE Client disconnected");
         
         if (!provisioningComplete) {
             pServer->startAdvertising();
             Serial.println("🔄 Restarted BLE advertising");
         }
     }
 };
 
 // ===== CHARACTERISTIC CALLBACKS =====
 
 class WiFiCharCallbacks: public NimBLECharacteristicCallbacks {
     void onWrite(NimBLECharacteristic* pCharacteristic) {
         std::string value = pCharacteristic->getValue();
         
         if (value.length() > 0) {
             DynamicJsonDocument doc(512);
             deserializeJson(doc, value.c_str());
             
             receivedWiFiSSID = doc["ssid"].as<String>();
             receivedWiFiPassword = doc["password"].as<String>();
             
             Serial.println("✓ WiFi credentials received");
             updateBLEStatus("wifi_received");
         }
     }
 };
 
 class SupabaseCharCallbacks: public NimBLECharacteristicCallbacks {
     void onWrite(NimBLECharacteristic* pCharacteristic) {
         std::string value = pCharacteristic->getValue();
         
         if (value.length() > 0) {
             DynamicJsonDocument doc(1024);
             deserializeJson(doc, value.c_str());
             
             receivedSupabaseURL = doc["url"].as<String>();
             receivedSupabaseKey = doc["anon_key"].as<String>();
             
             Serial.println("✓ Supabase credentials received");
             updateBLEStatus("supabase_received");
         }
     }
 };
 
 class UserCharCallbacks: public NimBLECharacteristicCallbacks {
     void onWrite(NimBLECharacteristic* pCharacteristic) {
         std::string value = pCharacteristic->getValue();
         
         if (value.length() > 0) {
             DynamicJsonDocument doc(256);
             deserializeJson(doc, value.c_str());
             
             receivedUserID = doc["user_id"].as<String>();
             
             Serial.printf("✓ User ID received: %s\n", receivedUserID.c_str());
             updateBLEStatus("user_received");
             
             // Save all credentials to flash
             saveProvisioningData();
             
             provisioningComplete = true;
             updateBLEStatus("provisioning_complete");
         }
     }
 };
 
 // ===== HELPER FUNCTIONS =====
 
 void updateBLEStatus(String status) {
     if (pStatusChar != nullptr) {
         pStatusChar->setValue(status.c_str());
         pStatusChar->notify();
     }
 }
 
 void saveProvisioningData() {
     Preferences prefs;
     
     // Save WiFi credentials
     prefs.begin("wifi_creds", false);
     prefs.putString("ssid", receivedWiFiSSID);
     prefs.putString("password", receivedWiFiPassword);
     prefs.end();
     
     // Save Supabase credentials
     prefs.begin("supabase", false);
     prefs.putString("url", receivedSupabaseURL);
     prefs.putString("anon_key", receivedSupabaseKey);
     prefs.end();
     
     // Save user ID
     prefs.begin("device", false);
     prefs.putString("user_id", receivedUserID);
     
     // Generate device ID from MAC if not exists
     if (!prefs.isKey("id")) {
         uint8_t mac[6];
         WiFi.macAddress(mac);
         char idBuf[32];
         snprintf(idBuf, sizeof(idBuf), "fountain_%02X%02X%02X", 
                  mac[3], mac[4], mac[5]);
         prefs.putString("id", String(idBuf));
     }
     
     prefs.end();
     
     Serial.println("✓ All credentials saved to flash");
 }
 
 bool checkProvisioningStatus() {
     Preferences prefs;
     
     prefs.begin("wifi_creds", true);
     bool hasWiFi = prefs.isKey("ssid");
     prefs.end();
     
     prefs.begin("supabase", true);
     bool hasSupabase = prefs.isKey("url");
     prefs.end();
     
     prefs.begin("device", true);
     bool hasUser = prefs.isKey("user_id");
     prefs.end();
     
     return hasWiFi && hasSupabase && hasUser;
 }
 
 // ===== MAIN PROVISIONING FUNCTION =====
 
void startBLEProvisioning() {
    Serial.println("════════════════════════════════════════");
    Serial.println("  BLE PROVISIONING MODE");
    Serial.println("════════════════════════════════════════");
    
    // Get MAC address and create unique device name
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char deviceName[32];
    snprintf(deviceName, sizeof(deviceName), "PetFountain-%02X%02X", 
             mac[4], mac[5]);
    
    // Initialize NimBLE
    NimBLEDevice::init(deviceName);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
     
     // Create BLE Server
     pServer = NimBLEDevice::createServer();
     pServer->setCallbacks(new ServerCallbacks());
     
     // Create BLE Service
     NimBLEService* pService = pServer->createService(SERVICE_UUID);
     
     // WiFi Characteristic
     NimBLECharacteristic* pWiFiChar = pService->createCharacteristic(
         WIFI_CHAR_UUID,
         NIMBLE_PROPERTY::WRITE
     );
     pWiFiChar->setCallbacks(new WiFiCharCallbacks());
     
     // Supabase Characteristic
     NimBLECharacteristic* pSupabaseChar = pService->createCharacteristic(
         SUPABASE_CHAR_UUID,
         NIMBLE_PROPERTY::WRITE
     );
     pSupabaseChar->setCallbacks(new SupabaseCharCallbacks());
     
     // User ID Characteristic
     NimBLECharacteristic* pUserChar = pService->createCharacteristic(
         USER_CHAR_UUID,
         NIMBLE_PROPERTY::WRITE
     );
     pUserChar->setCallbacks(new UserCharCallbacks());
     
     // Status Characteristic
     pStatusChar = pService->createCharacteristic(
         STATUS_CHAR_UUID,
         NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
     );
     pStatusChar->setValue("waiting");
     
     // Start service
     pService->start();
     
    // Start advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setMinInterval(100);
    pAdvertising->setMaxInterval(200);
    pAdvertising->start();
    
    Serial.println("✓ BLE Server started");
    Serial.printf("  Device Name: %s\n", deviceName);
    Serial.printf("  Service UUID: %s\n", SERVICE_UUID);
    Serial.println("════════════════════════════════════════");
    Serial.println("\n📱 Waiting for mobile app connection...\n");
     
     // Blink LED to indicate provisioning mode
     unsigned long startTime = millis();
     unsigned long lastBlink = millis();
     while (!provisioningComplete && (millis() - startTime < BLE_TIMEOUT_MS)) {
         // Non-blocking LED blink
         if (millis() - lastBlink >= 500) {
             digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
             lastBlink = millis();
         }
         // Yield to allow BLE stack to process events
         delay(10);
     }
     
     // Turn off LED
     digitalWrite(STATUS_LED, LOW);
     
     // Cleanup
     if (provisioningComplete) {
         Serial.println("\n✓ Provisioning successful!");
         delay(1000);
     } else {
         Serial.println("\n⚠️  Provisioning timeout!");
     }
     
     // Stop BLE and free resources
     NimBLEDevice::deinit(true);
     Serial.println("✓ BLE stopped, memory freed");
 }
 
 #endif // BLE_PROVISIONING_H