/*
 * WiFi Connection Manager
 */

 #ifndef WIFI_MANAGER_H
 #define WIFI_MANAGER_H
 
 #include <WiFi.h>
 #include <Preferences.h>
 #include <time.h>
 #include "config.h"
 
void connectToWiFi() {
    Preferences prefs;
    prefs.begin("wifi_creds", true);
    String ssid = prefs.getString("ssid", "");
    String password = prefs.getString("password", "");
    prefs.end();
    
    if (ssid.length() == 0) {
        Serial.println("✗ No WiFi credentials found");
        return;
    }
    
    Serial.printf("Connecting to WiFi: %s\n", ssid.c_str());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
     
     unsigned long startTime = millis();
     while (WiFi.status() != WL_CONNECTED && 
            (millis() - startTime < WIFI_CONNECT_TIMEOUT_MS)) {
         delay(500);
         Serial.print(".");
     }
     
     Serial.println();
     
     if (WiFi.status() == WL_CONNECTED) {
         Serial.println("✓ WiFi connected");
         Serial.printf("  IP Address: %s\n", WiFi.localIP().toString().c_str());
         Serial.printf("  Signal: %d dBm\n", WiFi.RSSI());
     } else {
         Serial.println("✗ WiFi connection failed");
     }
 }
 
 void maintainWiFiConnection() {
     if (WiFi.status() != WL_CONNECTED) {
         Serial.println("⚠️  WiFi disconnected, reconnecting...");
         WiFi.reconnect();
         
         unsigned long startTime = millis();
         while (WiFi.status() != WL_CONNECTED && 
                (millis() - startTime < WIFI_CONNECT_TIMEOUT_MS)) {
             delay(500);
         }
         
         if (WiFi.status() == WL_CONNECTED) {
             Serial.println("✓ WiFi reconnected");
         } else {
             Serial.println("✗ WiFi reconnection failed");
         }
     }
 }
 
 void setupNTP() {
     Serial.println("Synchronizing time with NTP...");
     
     configTime(0, 0, "pool.ntp.org", "time.nist.gov");
     
     struct tm timeinfo;
     int attempts = 0;
     while (!getLocalTime(&timeinfo) && attempts < 10) {
         delay(500);
         Serial.print(".");
         attempts++;
     }
     
     Serial.println();
     
     if (getLocalTime(&timeinfo)) {
         Serial.println("✓ Time synchronized");
         char timeStr[64];
         strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
         Serial.printf("  Current time: %s UTC\n", timeStr);
     } else {
         Serial.println("⚠️  Time sync failed");
     }
 }
 
 String getISOTimestamp() {
     time_t now;
     struct tm timeinfo;
     
     if (!getLocalTime(&timeinfo)) {
         return String(millis());
     }
     
     char buffer[25];
     strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
     return String(buffer);
 }
 
 #endif // WIFI_MANAGER_H