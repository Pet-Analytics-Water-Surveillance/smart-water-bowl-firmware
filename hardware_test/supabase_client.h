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
     deviceId = prefs.getString("id", "");
     prefs.end();
     
     if (supabaseUrl.length() > 0 && supabaseKey.length() > 0) {
         Serial.println("✓ Supabase client initialized");
         Serial.printf("  User ID: %s\n", userId.c_str());
         Serial.printf("  Device ID: %s\n", deviceId.c_str());
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
     String url = supabaseUrl + "/rest/v1/drinking_events";
     
     http.begin(url);
     http.addHeader("apikey", supabaseKey);
     http.addHeader("Authorization", "Bearer " + supabaseKey);
     http.addHeader("Content-Type", "application/json");
     http.addHeader("Prefer", "return=minimal");
     http.setTimeout(HTTP_TIMEOUT_MS);
     
     StaticJsonDocument<512> doc;
     doc["user_id"] = userId;
     doc["pet_id"] = petId;
     doc["device_id"] = deviceId;
     doc["timestamp"] = timestamp;
     doc["water_consumed_ml"] = waterConsumedMl;
     
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