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
#include <esp_heap_caps.h>
#include "config.h"

extern "C" {
    #include "mbedtls/base64.h"
}

// Base64 decode using ESP32 mbedTLS crypto library
String base64_decode(const char* input) {
    size_t input_len = strlen(input);
    size_t output_len = (input_len * 3) / 4;  // Max output size
    unsigned char* output = (unsigned char*)malloc(output_len + 1);  // +1 for null terminator
    
    if (!output) {
        Serial.println("❌ Failed to allocate memory for Base64 decode");
        return String("");
    }
    
    size_t actual_output_len = 0;
    int ret = mbedtls_base64_decode(output, output_len, &actual_output_len, 
                                    (const unsigned char*)input, input_len);
    
    String result = "";
    if (ret == 0 && actual_output_len > 0) {
        output[actual_output_len] = '\0';  // Null terminate
        result = String((char*)output);
    } else {
        Serial.printf("❌ Base64 decode failed with error: %d (input_len: %d)\n", ret, input_len);
    }
    
    free(output);
    return result;
}
 
// Forward declare callback classes
class ServerCallbacks;
class WiFiCharCallbacks;
class SupabaseCharCallbacks;
class UserCharCallbacks;

// Global BLE objects
NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pStatusChar = nullptr;
NimBLECharacteristic* pWiFiChar = nullptr;      // Store for manual checking
NimBLECharacteristic* pSupabaseChar = nullptr; // Store for manual checking
NimBLECharacteristic* pUserChar = nullptr;     // Store for manual checking
bool bleConnected = false;
bool provisioningComplete = false;

// Static callback instances that persist
ServerCallbacks* serverCallbacks = nullptr;
WiFiCharCallbacks* wifiCallbacks = nullptr;
SupabaseCharCallbacks* supabaseCallbacks = nullptr;
UserCharCallbacks* userCallbacks = nullptr;
 
// Credentials received via BLE
String receivedWiFiSSID = "";
String receivedWiFiPassword = "";
String receivedSupabaseURL = "";
String receivedSupabaseKey = "";
String receivedUserID = "";
String receivedHouseholdID = "";

// ===== FORWARD DECLARATIONS =====
void updateBLEStatus(String status);
void saveProvisioningData();

// ===== BLE SERVER CALLBACKS =====
 
class ServerCallbacks: public NimBLEServerCallbacks {
    // NEW signature for NimBLE-Arduino v2.x
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
        // FIRST thing - update status so mobile app can see this fired
        updateBLEStatus("firmware_connected");
        
        Serial.println("\n🔔🔔🔔 onConnect callback FIRED! 🔔🔔🔔");  // DEBUG - very obvious
        Serial.println("════════════════════════════════════════");
        bleConnected = true;
        Serial.println("📱 BLE Client connected!");
        Serial.printf("  Connections: %d\n", pServer->getConnectedCount());
        Serial.printf("  MTU: %d bytes\n", NimBLEDevice::getMTU());
        Serial.printf("  Client Address: %s\n", connInfo.getAddress().toString().c_str());
        Serial.printf("  Connection ID: %d\n", connInfo.getConnHandle());
        Serial.println("════════════════════════════════════════\n");
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
        Serial.println("\n🔔 onDisconnect callback triggered!");  // DEBUG
        Serial.printf("  Client Address: %s\n", connInfo.getAddress().toString().c_str());
        
        bleConnected = false;
        Serial.println("📱 BLE Client disconnected");
        
        if (!provisioningComplete) {
            NimBLEDevice::startAdvertising();
            Serial.println("🔄 Restarted BLE advertising\n");
        }
    }
    
    void onMTUChange(uint16_t MTU, NimBLEConnInfo& connInfo) {
        Serial.printf("🔔 onMTUChange callback! MTU: %u bytes, Client: %s\n", 
                     MTU, connInfo.getAddress().toString().c_str());
    }
};
 
 // ===== CHARACTERISTIC CALLBACKS =====
 
// Forward declarations
void processWiFiCredentials(const std::string& value);
void processSupabaseCredentials(const std::string& value);
void processUserCredentials(const std::string& value);

class WiFiCharCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        Serial.println("\n");
        Serial.println("════════════════════════════════════════");
        Serial.println("🔔🔔🔔 WiFi onWrite CALLED! 🔔🔔🔔");
        Serial.println("════════════════════════════════════════");
        
        std::string value = pCharacteristic->getValue();
        Serial.printf("📥 Received %d bytes\n", value.length());
        
        if (value.length() > 50) {
            Serial.printf("   Raw (first 50): %s...\n", value.substr(0, 50).c_str());
        } else {
            Serial.printf("   Raw data: %s\n", value.c_str());
        }
        
        processWiFiCredentials(value);
    }
};
 
class SupabaseCharCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        Serial.println("\n");
        Serial.println("════════════════════════════════════════");
        Serial.println("🔔🔔🔔 Supabase onWrite CALLED! 🔔🔔🔔");
        Serial.println("════════════════════════════════════════");
        
        std::string value = pCharacteristic->getValue();
        Serial.printf("📥 Received %d bytes\n", value.length());
        Serial.printf("   Raw (first 50): %s...\n", value.substr(0, 50).c_str());
        
        processSupabaseCredentials(value);
    }
};
 
class UserCharCallbacks: public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        Serial.println("\n");
        Serial.println("════════════════════════════════════════");
        Serial.println("🔔🔔🔔 User onWrite CALLED! 🔔🔔🔔");
        Serial.println("════════════════════════════════════════");
        
        std::string value = pCharacteristic->getValue();
        Serial.printf("📥 Received %d bytes\n", value.length());
        Serial.printf("   Raw (first 50): %s...\n", value.substr(0, 50).c_str());
        
        processUserCredentials(value);
    }
};
 
 // ===== HELPER FUNCTIONS =====
 
 void updateBLEStatus(String status) {
     if (pStatusChar != nullptr) {
         pStatusChar->setValue(status.c_str());
         pStatusChar->notify();
     }
 }

// Process WiFi credentials (called from callback or manual check)
void processWiFiCredentials(const std::string& value) {
    if (value.length() > 0) {
        Serial.printf("📦 Raw data received: %d bytes\n", value.length());
        Serial.printf("   First 50 chars: %s\n", value.substr(0, 50).c_str());
        Serial.printf("   Last 10 chars: %s\n", value.substr(value.length() - 10).c_str());
        
        // Check if it looks like Base64 (alphanumeric with +/=)
        bool looksLikeBase64 = true;
        for (size_t i = 0; i < value.length() && i < 20; i++) {
            char c = value[i];
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')) {
                looksLikeBase64 = false;
                break;
            }
        }
        
        Serial.printf("   Looks like Base64: %s\n", looksLikeBase64 ? "YES" : "NO");
        
        String decodedJson;
        if (looksLikeBase64) {
            Serial.println("🔓 Decoding Base64...");
            decodedJson = base64_decode(value.c_str());
            Serial.printf("   Decoded length: %d bytes\n", decodedJson.length());
            if (decodedJson.length() == 0) {
                Serial.println("   ⚠️  Base64 decode returned empty! Trying raw JSON...");
                decodedJson = String(value.c_str());
            }
        } else {
            Serial.println("📝 Data appears to be raw JSON, skipping Base64 decode");
            decodedJson = String(value.c_str());
        }
        
        Serial.printf("   Final JSON to parse: %s\n", decodedJson.c_str());
        
        // Now parse the JSON
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, decodedJson);
        
        if (error) {
            Serial.printf("❌ JSON parse error: %s\n", error.c_str());
            Serial.printf("   Attempted to parse: %s\n", decodedJson.c_str());
            return;
        }
        
        receivedWiFiSSID = doc["ssid"].as<String>();
        receivedWiFiPassword = doc["password"].as<String>();
        
        Serial.println("✅ WiFi credentials received and parsed!");
        Serial.printf("  SSID: %s\n", receivedWiFiSSID.c_str());
        Serial.printf("  Password: %s\n", receivedWiFiPassword.c_str());
        updateBLEStatus("wifi_received");
    }
}

// Process Supabase credentials
void processSupabaseCredentials(const std::string& value) {
    if (value.length() > 0) {
        Serial.printf("📦 Raw data received: %d bytes\n", value.length());
        Serial.printf("   First 50 chars: %s\n", value.substr(0, 50).c_str());
        
        // Check if it looks like Base64
        bool looksLikeBase64 = true;
        for (size_t i = 0; i < value.length() && i < 20; i++) {
            char c = value[i];
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')) {
                looksLikeBase64 = false;
                break;
            }
        }
        
        String decodedJson;
        if (looksLikeBase64) {
            Serial.println("🔓 Decoding Base64...");
            decodedJson = base64_decode(value.c_str());
            if (decodedJson.length() == 0) {
                Serial.println("   ⚠️  Base64 decode returned empty! Trying raw JSON...");
                decodedJson = String(value.c_str());
            }
        } else {
            Serial.println("📝 Data appears to be raw JSON, skipping Base64 decode");
            decodedJson = String(value.c_str());
        }
        
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, decodedJson);
        
        if (error) {
            Serial.printf("❌ JSON parse error: %s\n", error.c_str());
            Serial.printf("   Attempted to parse (first 200): %s\n", decodedJson.substring(0, 200).c_str());
            return;
        }
        
        receivedSupabaseURL = doc["url"].as<String>();
        receivedSupabaseKey = doc["anon_key"].as<String>();
        
        Serial.println("✅ Supabase credentials received and parsed!");
        Serial.printf("  URL: %s\n", receivedSupabaseURL.c_str());
        updateBLEStatus("supabase_received");
    }
}

// Process User credentials
void processUserCredentials(const std::string& value) {
    if (value.length() > 0) {
        Serial.printf("📦 Raw data received: %d bytes\n", value.length());
        Serial.printf("   First 50 chars: %s\n", value.substr(0, 50).c_str());
        
        // Check if it looks like Base64
        bool looksLikeBase64 = true;
        for (size_t i = 0; i < value.length() && i < 20; i++) {
            char c = value[i];
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=')) {
                looksLikeBase64 = false;
                break;
            }
        }
        
        String decodedJson;
        if (looksLikeBase64) {
            Serial.println("🔓 Decoding Base64...");
            decodedJson = base64_decode(value.c_str());
            if (decodedJson.length() == 0) {
                Serial.println("   ⚠️  Base64 decode returned empty! Trying raw JSON...");
                decodedJson = String(value.c_str());
            }
        } else {
            Serial.println("📝 Data appears to be raw JSON, skipping Base64 decode");
            decodedJson = String(value.c_str());
        }
        
        Serial.printf("   Final JSON to parse: %s\n", decodedJson.c_str());
        
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, decodedJson);
        
        if (error) {
            Serial.printf("❌ JSON parse error: %s\n", error.c_str());
            Serial.printf("   Attempted to parse: %s\n", decodedJson.c_str());
            return;
        }
        
        receivedUserID = doc["user_id"].as<String>();
        receivedHouseholdID = doc["household_id"].as<String>();
        
        Serial.println("✅ User ID and Household ID received and parsed!");
        Serial.printf("  User ID: %s\n", receivedUserID.c_str());
        Serial.printf("  Household ID: %s\n", receivedHouseholdID.c_str());
        updateBLEStatus("user_received");
        
        // Save all credentials to flash
        Serial.println("💾 Saving all credentials to flash...");
        saveProvisioningData();
        
        provisioningComplete = true;
        updateBLEStatus("provisioning_complete");
        Serial.println("🎉 PROVISIONING COMPLETE!");
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
     
    // Save user ID and household ID
    prefs.begin("device", false);
    prefs.putString("user_id", receivedUserID);
    prefs.putString("household_id", receivedHouseholdID);
    
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
     
     // Create persistent callback instance
     serverCallbacks = new ServerCallbacks();
     Serial.println("🔧 Registering server callbacks...");
     pServer->setCallbacks(serverCallbacks);
     Serial.printf("✓ BLE Server created with callbacks at: %p\n", serverCallbacks);
     
     // Create BLE Service
     NimBLEService* pService = pServer->createService(SERVICE_UUID);
     Serial.printf("✓ Service created: %s\n", SERVICE_UUID);
     
     // WiFi Characteristic - WRITE property for write-with-response
     pWiFiChar = pService->createCharacteristic(
         WIFI_CHAR_UUID,
         NIMBLE_PROPERTY::WRITE
     );
     wifiCallbacks = new WiFiCharCallbacks();
     pWiFiChar->setCallbacks(wifiCallbacks);
     Serial.printf("✓ WiFi char created: %s\n", WIFI_CHAR_UUID);
     Serial.printf("   Characteristic pointer: %p\n", pWiFiChar);
     Serial.printf("   Callbacks registered at: %p\n", wifiCallbacks);
     
     // Supabase Characteristic
     pSupabaseChar = pService->createCharacteristic(
         SUPABASE_CHAR_UUID,
         NIMBLE_PROPERTY::WRITE
     );
     supabaseCallbacks = new SupabaseCharCallbacks();
     pSupabaseChar->setCallbacks(supabaseCallbacks);
     Serial.printf("✓ Supabase char created: %s\n", SUPABASE_CHAR_UUID);
     Serial.printf("   Characteristic pointer: %p\n", pSupabaseChar);
     Serial.printf("   Callbacks registered at: %p\n", supabaseCallbacks);
     
     // User ID Characteristic
     pUserChar = pService->createCharacteristic(
         USER_CHAR_UUID,
         NIMBLE_PROPERTY::WRITE
     );
     userCallbacks = new UserCharCallbacks();
     pUserChar->setCallbacks(userCallbacks);
     Serial.printf("✓ User char created: %s\n", USER_CHAR_UUID);
     Serial.printf("   Characteristic pointer: %p\n", pUserChar);
     Serial.printf("   Callbacks registered at: %p\n", userCallbacks);
     
     // Status Characteristic with boot timestamp to verify fresh connection
     pStatusChar = pService->createCharacteristic(
         STATUS_CHAR_UUID,
         NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
     );
     
     // Create a unique status with timestamp to prove this is the current firmware
     String initialStatus = "waiting_" + String(millis());
     pStatusChar->setValue(initialStatus.c_str());
     Serial.printf("✓ Status char created: %s\n", STATUS_CHAR_UUID);
     Serial.printf("  Initial status value: %s (length: %d)\n", initialStatus.c_str(), initialStatus.length());
     
     // Start service
     pService->start();
     Serial.println("✓ Service started");
     
    // CRITICAL: Start the BLE server explicitly
    Serial.println("🚀 Starting BLE server...");
    
    // Configure advertising BEFORE starting
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    
    // Build custom advertising data with both name and service UUID
    NimBLEAdvertisementData advData;
    advData.setFlags(0x06); // LE General Discoverable, BR/EDR not supported
    advData.setName(deviceName);
    advData.setCompleteServices(BLEUUID(SERVICE_UUID));
    pAdvertising->setAdvertisementData(advData);
    
    // Set scan response data (sent when scanner requests more info)
    NimBLEAdvertisementData scanData;
    scanData.setName(deviceName);
    scanData.setCompleteServices(BLEUUID(SERVICE_UUID));
    pAdvertising->setScanResponseData(scanData);
    
    pAdvertising->setMinInterval(100);     // 100ms * 0.625 = 62.5ms
    pAdvertising->setMaxInterval(200);     // 200ms * 0.625 = 125ms
    
    Serial.println("✓ Advertising configured");
    Serial.printf("  Broadcasting name: %s\n", deviceName);
    Serial.printf("  Broadcasting service UUID: %s\n", SERVICE_UUID);
    
    // Start advertising
    NimBLEDevice::startAdvertising();
    Serial.println("✓ BLE advertising started via NimBLEDevice");
    
    // Give NimBLE time to fully start
    delay(100);
    
    Serial.println("✓ BLE server is now accepting connections");
    
    Serial.println("════════════════════════════════════════");
    Serial.println("✓ BLE Server ready!");
    Serial.printf("  Device Name: %s\n", deviceName);
    Serial.printf("  Service UUID: %s\n", SERVICE_UUID);
    Serial.printf("  MTU: 512 bytes\n");
    Serial.printf("  Server Address: %s\n", NimBLEDevice::getAddress().toString().c_str());
    Serial.println("════════════════════════════════════════");
    Serial.println("\n📱 Waiting for mobile app connection...\n");
     
     // Blink LED to indicate provisioning mode
     unsigned long startTime = millis();
     unsigned long lastBlink = millis();
     unsigned long lastDebug = millis();
     unsigned long lastCheck = millis();
     size_t lastWiFiLen = 0;
     size_t lastSupabaseLen = 0;
     size_t lastUserLen = 0;
     
     while (!provisioningComplete && (millis() - startTime < BLE_TIMEOUT_MS)) {
         // Manually check characteristic values every 2 seconds (callbacks may not fire)
         if (millis() - lastCheck >= 2000 && bleConnected) {
             if (pWiFiChar != nullptr) {
                 std::string wifiValue = pWiFiChar->getValue();
                 if (wifiValue.length() != lastWiFiLen) {
                     Serial.println("\n🔍 MANUAL CHECK: WiFi char value changed!");
                     Serial.printf("   Old length: %d, New length: %d\n", lastWiFiLen, wifiValue.length());
                     if (wifiValue.length() > 0) {
                         Serial.println("   ⚠️  Data written but callback didn't fire! Triggering manual processing...");
                         processWiFiCredentials(wifiValue);
                     }
                     lastWiFiLen = wifiValue.length();
                 }
             }
             
             if (pSupabaseChar != nullptr) {
                 std::string supabaseValue = pSupabaseChar->getValue();
                 if (supabaseValue.length() != lastSupabaseLen) {
                     Serial.println("\n🔍 MANUAL CHECK: Supabase char value changed!");
                     Serial.printf("   Old length: %d, New length: %d\n", lastSupabaseLen, supabaseValue.length());
                     if (supabaseValue.length() > 0) {
                         Serial.println("   ⚠️  Data written but callback didn't fire! Triggering manual processing...");
                         processSupabaseCredentials(supabaseValue);
                     }
                     lastSupabaseLen = supabaseValue.length();
                 }
             }
             
             if (pUserChar != nullptr) {
                 std::string userValue = pUserChar->getValue();
                 if (userValue.length() != lastUserLen) {
                     Serial.println("\n🔍 MANUAL CHECK: User char value changed!");
                     Serial.printf("   Old length: %d, New length: %d\n", lastUserLen, userValue.length());
                     if (userValue.length() > 0) {
                         Serial.println("   ⚠️  Data written but callback didn't fire! Triggering manual processing...");
                         processUserCredentials(userValue);
                     }
                     lastUserLen = userValue.length();
                 }
             }
             
             lastCheck = millis();
         }
         
         // Print debug info every 5 seconds
         if (millis() - lastDebug >= 5000) {
             Serial.printf("⏳ Still waiting... (Connected: %s)\n", bleConnected ? "YES" : "NO");
             lastDebug = millis();
         }
         
         // Non-blocking LED blink
         if (millis() - lastBlink >= 500) {
             digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
             lastBlink = millis();
         }
         
         // Yield to allow BLE stack to process events
         vTaskDelay(1);  // FreeRTOS yield - allows NimBLE task to run
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
    
    // Print memory before cleanup
    Serial.println("\n📊 Memory before BLE cleanup:");
    Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.printf("  Largest free block: %d bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    
    // Delete callback instances to free memory
    if (serverCallbacks) { delete serverCallbacks; serverCallbacks = nullptr; }
    if (wifiCallbacks) { delete wifiCallbacks; wifiCallbacks = nullptr; }
    if (supabaseCallbacks) { delete supabaseCallbacks; supabaseCallbacks = nullptr; }
    if (userCallbacks) { delete userCallbacks; userCallbacks = nullptr; }
    
    // Stop advertising
    NimBLEDevice::stopAdvertising();
    
    // Disconnect all clients
    if (pServer && pServer->getConnectedCount() > 0) {
        Serial.println("Disconnecting BLE clients...");
        pServer->disconnect(0);  // Disconnect first connection
    }
    
    // Deinitialize NimBLE completely
    NimBLEDevice::deinit(true);
    Serial.println("✓ BLE stopped");
    
    // Give system time to free resources
    delay(1000);
    
    // Force garbage collection
    Serial.println("🧹 Forcing heap compaction...");
    delay(500);
    
    // Print memory after cleanup
    Serial.println("\n📊 Memory after BLE cleanup:");
    Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.printf("  Largest free block: %d bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    Serial.println();
 }
 
 #endif // BLE_PROVISIONING_H