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
        Serial.println("\n════════════════════════════════════════");
        Serial.println("📱 BLE Client connected!");
        Serial.printf("  Connections: %d\n", pServer->getConnectedCount());
        Serial.println("════════════════════════════════════════\n");
        updateBLEStatus("connected");
    }
    
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
        bleConnected = true;
        Serial.println("\n════════════════════════════════════════");
        Serial.println("📱 BLE Client connected (with details)!");
        Serial.printf("  Connections: %d\n", pServer->getConnectedCount());
        Serial.printf("  Connection ID: %d\n", desc->conn_handle);
        Serial.printf("  MTU: %d\n", NimBLEDevice::getMTU());
        Serial.println("════════════════════════════════════════\n");
        updateBLEStatus("connected");
    }

    void onDisconnect(NimBLEServer* pServer) {
        bleConnected = false;
        Serial.println("\n📱 BLE Client disconnected");
        
        if (!provisioningComplete) {
            pServer->startAdvertising();
            Serial.println("🔄 Restarted BLE advertising\n");
        }
    }
    
    void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) {
        Serial.printf("📶 MTU changed to: %u bytes for connection ID: %d\n", MTU, desc->conn_handle);
    }
};
 
 // ===== CHARACTERISTIC CALLBACKS =====
 
class WiFiCharCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        
        Serial.printf("\n📥 WiFi Characteristic Write - Length: %d bytes\n", value.length());
        
        if (value.length() > 0) {
            DynamicJsonDocument doc(512);
            DeserializationError error = deserializeJson(doc, value.c_str());
            
            if (error) {
                Serial.printf("❌ JSON parse error: %s\n", error.c_str());
                return;
            }
            
            receivedWiFiSSID = doc["ssid"].as<String>();
            receivedWiFiPassword = doc["password"].as<String>();
            
            Serial.println("✓ WiFi credentials received");
            Serial.printf("  SSID: %s\n", receivedWiFiSSID.c_str());
            updateBLEStatus("wifi_received");
        }
    }
};
 
class SupabaseCharCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        
        Serial.printf("\n📥 Supabase Characteristic Write - Length: %d bytes\n", value.length());
        
        if (value.length() > 0) {
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, value.c_str());
            
            if (error) {
                Serial.printf("❌ JSON parse error: %s\n", error.c_str());
                return;
            }
            
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
        
        Serial.printf("\n📥 User Characteristic Write - Length: %d bytes\n", value.length());
        
        if (value.length() > 0) {
            DynamicJsonDocument doc(256);
            DeserializationError error = deserializeJson(doc, value.c_str());
            
            if (error) {
                Serial.printf("❌ JSON parse error: %s\n", error.c_str());
                return;
            }
            
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
    
    // Set MTU size (default is 23, we want larger for JSON data)
    NimBLEDevice::setMTU(512);
    
    Serial.println("✓ NimBLE initialized");
    Serial.printf("  Device Name: %s\n", deviceName);
     
     // Create BLE Server
     pServer = NimBLEDevice::createServer();
     pServer->setCallbacks(new ServerCallbacks());
     
     Serial.println("✓ BLE Server created with callbacks");
     
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
     Serial.println("✓ Service started");
     
    // Configure advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinInterval(100);  // 100ms
    pAdvertising->setMaxInterval(200);  // 200ms
    pAdvertising->setMinPreferred(0x06); // Connection interval
    pAdvertising->setMaxPreferred(0x12);
    
    // Start advertising
    pAdvertising->start();
    
    Serial.println("════════════════════════════════════════");
    Serial.println("✓ BLE Server started and advertising");
    Serial.printf("  Service UUID: %s\n", SERVICE_UUID);
    Serial.printf("  MTU: 512 bytes\n");
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