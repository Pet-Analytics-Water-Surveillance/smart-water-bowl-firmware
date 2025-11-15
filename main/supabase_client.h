/*
 * Supabase Client
 * Real-time event pushing to cloud
 */

 #ifndef SUPABASE_CLIENT_H
 #define SUPABASE_CLIENT_H
 
 #include <HTTPClient.h>
 #include <Preferences.h>
 #include <ArduinoJson.h>
 #include "config.h"
 
// Global credentials
String supabaseUrl = "";
String supabaseKey = "";
String userId = "";
String householdId = "";
String deviceId = "";
 
 void initializeSupabase() {
     Serial.println("Initializing Supabase client...");
     
     Preferences prefs;
     
     prefs.begin("supabase", true);
     supabaseUrl = prefs.getString("url", "");
     supabaseKey = prefs.getString("anon_key", "");
     prefs.end();
     
    prefs.begin("device", true);
    userId = prefs.getString("user_id", "");
    householdId = prefs.getString("household_id", "");
    deviceId = prefs.getString("id", "");
    prefs.end();
    
    if (supabaseUrl.length() > 0 && supabaseKey.length() > 0) {
        Serial.println("✓ Supabase client initialized");
        Serial.printf("  User ID: %s\n", userId.c_str());
        Serial.printf("  Household ID: %s\n", householdId.c_str());
        
        // Validate Device ID format
        Serial.println("");
        Serial.println("════════════════════════════════════════");
        Serial.println("🔍 DEVICE UUID VALIDATION");
        Serial.println("════════════════════════════════════════");
        Serial.printf("  Loaded Device ID: %s\n", deviceId.c_str());
        
        // Check if it looks like a UUID (8-4-4-4-12 format with hyphens)
        bool isValidUUID = (deviceId.length() == 36 && 
                           deviceId.charAt(8) == '-' && 
                           deviceId.charAt(13) == '-' && 
                           deviceId.charAt(18) == '-' && 
                           deviceId.charAt(23) == '-');
        
        if (isValidUUID) {
            Serial.println("  Status: ✅ VALID UUID FORMAT");
            Serial.println("  ✓ Compatible with Supabase database");
            Serial.println("  ✓ Data uploads will work correctly");
        } else {
            Serial.println("  Status: ⚠️⚠️⚠️ INVALID UUID FORMAT ⚠️⚠️⚠️");
            Serial.println("");
            Serial.println("  ⚠️  Device ID is not a valid UUID!");
            Serial.println("  ⚠️  Expected format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx");
            Serial.println("  ⚠️  Current format: NOT UUID (likely fountain_XXXXXX)");
            Serial.println("");
            Serial.println("  🚨 CRITICAL: Data uploads will FAIL with HTTP 400!");
            Serial.println("  🚨 Supabase requires UUID format for device_id column");
            Serial.println("");
            Serial.println("  📱 SOLUTION: Re-provision device using updated mobile app");
            Serial.println("     The app will assign a proper UUID from Supabase");
        }
        Serial.println("════════════════════════════════════════");
        Serial.println("");
    } else {
        Serial.println("✗ Supabase credentials missing");
    }
 }
 
bool insertDrinkingEvent(String petId, int waterConsumedMl, String timestamp) {
    if (supabaseUrl.length() == 0) {
        Serial.println("[Supabase] ✗ Not configured");
        return false;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Supabase] ✗ WiFi not connected");
        return false;
    }
    
    HTTPClient http;
    String url = supabaseUrl + "/rest/v1/hydration_events";
     
     http.begin(url);
     http.addHeader("apikey", supabaseKey);
     http.addHeader("Authorization", "Bearer " + supabaseKey);
     http.addHeader("Content-Type", "application/json");
     http.addHeader("Prefer", "return=minimal");
     http.setTimeout(HTTP_TIMEOUT_MS);
     
    StaticJsonDocument<512> doc;
    doc["pet_id"] = petId;
    doc["device_id"] = deviceId;
    doc["timestamp"] = timestamp;
    doc["amount_ml"] = waterConsumedMl;
     
     String jsonData;
     serializeJson(doc, jsonData);
     
 #ifdef DEBUG_SUPABASE
     Serial.println("[Supabase] Payload: " + jsonData);
 #endif
     
     for (int attempt = 0; attempt < MAX_RETRY_ATTEMPTS; attempt++) {
         int httpCode = http.POST(jsonData);
         
         if (httpCode == 201 || httpCode == 200) {
             Serial.println("[Supabase] ✓ Event logged successfully");
             http.end();
             return true;
         }
         
         Serial.printf("[Supabase] ✗ HTTP %d (attempt %d/%d)\n", 
                      httpCode, attempt + 1, MAX_RETRY_ATTEMPTS);
         
         if (httpCode == 400 || httpCode == 401 || httpCode == 403) {
             String response = http.getString();
             Serial.println("[Supabase] Error: " + response);
             break;
         }
         
         if (attempt < MAX_RETRY_ATTEMPTS - 1) {
             delay(2000 * (attempt + 1));
         }
     }
     
     http.end();
     return false;
 }
 
 #endif // SUPABASE_CLIENT_H