/*
 * Clear Provisioning Credentials
 * 
 * Upload this sketch to force the device back into provisioning mode
 * Then re-upload the main sketch
 */

#include <Preferences.h>

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n═══════════════════════════════════");
    Serial.println("  CLEARING PROVISIONING DATA");
    Serial.println("═══════════════════════════════════\n");
    
    Preferences prefs;
    
    // Clear WiFi credentials
    prefs.begin("wifi_creds", false);
    prefs.clear();
    prefs.end();
    Serial.println("✓ WiFi credentials cleared");
    
    // Clear Supabase credentials
    prefs.begin("supabase", false);
    prefs.clear();
    prefs.end();
    Serial.println("✓ Supabase credentials cleared");
    
    // Clear device data
    prefs.begin("device", false);
    prefs.clear();
    prefs.end();
    Serial.println("✓ Device data cleared");
    
    Serial.println("\n✓ ALL CREDENTIALS ERASED");
    Serial.println("═══════════════════════════════════\n");
    Serial.println("Now upload the main sketch to enter provisioning mode");
}

void loop() {
    // Nothing to do
    delay(1000);
}

