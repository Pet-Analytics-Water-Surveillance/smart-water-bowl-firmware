/*
 * Feature Extraction and Pet Identification
 */

 #ifndef FEATURE_MATCHING_H
 #define FEATURE_MATCHING_H
 
 #include <vector>
 #include <HTTPClient.h>
 #include <LittleFS.h>
 #include <ArduinoJson.h>
 #include "config.h"
 
 // Feature vector structure
 struct ImageFeatures {
     uint8_t colorHist[64];
     float brightness;
     float edgeDensity;
     String petId;
     String petName;
 };
 
 // Global reference features
 std::vector<ImageFeatures> referenceFeatures;
 
 // External variables
 extern String supabaseUrl;
 extern String supabaseKey;
 extern String userId;
 extern uint8_t* jpegBuffer;
 
 void initializeStorage() {
     Serial.println("Initializing LittleFS...");
     
     if (!LittleFS.begin(true)) {
         Serial.println("✗ LittleFS mount failed");
         return;
     }
     
     size_t totalBytes = LittleFS.totalBytes();
     size_t usedBytes = LittleFS.usedBytes();
     Serial.printf("✓ LittleFS mounted: %d / %d bytes used\n", usedBytes, totalBytes);
 }
 
 ImageFeatures extractFeatures(uint8_t* imageData, size_t size) {
     ImageFeatures features = {0};
     
     int sampleInterval = max(1, (int)(size / 2000));
     int samples = 0;
     float edgeCount = 0;
     
     for (size_t i = 0; i < size && samples < 2000; i += sampleInterval) {
         uint8_t val = imageData[i];
         
         int r = (val & 0xC0) >> 6;
         int g = (val & 0x30) >> 4;
         int b = (val & 0x0C) >> 2;
         int bin = (r * 16) + (g * 4) + b;
         
         if (bin < 64) {
             features.colorHist[bin]++;
         }
         
         features.brightness += val;
         
         if (i > 0 && abs(imageData[i] - imageData[i-1]) > 30) {
             edgeCount++;
         }
         
         samples++;
     }
     
     if (samples > 0) {
         features.brightness /= samples;
         features.edgeDensity = edgeCount / samples;
     }
     
     return features;
 }
 
 float compareFeatures(const ImageFeatures& ref, const ImageFeatures& captured) {
     float histSimilarity = 0.0;
     for (int i = 0; i < 64; i++) {
         histSimilarity += min(ref.colorHist[i], captured.colorHist[i]);
     }
     histSimilarity /= 2000.0;
     
     float brightDiff = abs(ref.brightness - captured.brightness) / 255.0;
     float brightScore = 1.0 - brightDiff;
     
     float edgeDiff = abs(ref.edgeDensity - captured.edgeDensity);
     float edgeScore = 1.0 - min(edgeDiff, 1.0f);
     
     return (histSimilarity * 0.6) + (brightScore * 0.3) + (edgeScore * 0.1);
 }
 
 String identifyPet(uint8_t* imageData, size_t imageSize) {
     if (referenceFeatures.size() == 0) {
         Serial.println("[Match] No reference features loaded");
         return "unknown";
     }
     
     ImageFeatures capturedFeatures = extractFeatures(imageData, imageSize);
     
     float bestScore = 0.0;
     String bestMatch = "unknown";
     String bestName = "Unknown";
     
     for (const auto& ref : referenceFeatures) {
         float score = compareFeatures(ref, capturedFeatures);
         
 #ifdef DEBUG_FEATURE_MATCHING
         Serial.printf("[Match] %s: %.1f%%\n", ref.petName.c_str(), score * 100);
 #endif
         
         if (score > bestScore) {
             bestScore = score;
             bestMatch = ref.petId;
             bestName = ref.petName;
         }
     }
     
     if (bestScore < MATCH_THRESHOLD) {
         Serial.printf("✗ No confident match (best: %s at %.1f%%)\n", 
                      bestName.c_str(), bestScore * 100);
         return "unknown";
     }
     
     Serial.printf("✓ Matched: %s (%.1f%% confidence)\n", 
                  bestName.c_str(), bestScore * 100);
     return bestMatch;
 }
 
 bool downloadAndExtractFeatures(String petId, String petName, String imageUrl) {
     HTTPClient http;
     
     http.begin(imageUrl);
     http.setTimeout(HTTP_TIMEOUT_MS);
     
     int httpCode = http.GET();
     
     if (httpCode == 200) {
         int len = http.getSize();
         
         if (len > JPEG_BUFFER_SIZE) {
             Serial.printf("  ✗ Image too large: %d bytes\n", len);
             http.end();
             return false;
         }
         
         WiFiClient* stream = http.getStreamPtr();
         size_t bytesRead = stream->readBytes(jpegBuffer, len);
         
         if (bytesRead == len) {
             ImageFeatures features = extractFeatures(jpegBuffer, bytesRead);
             features.petId = petId;
             features.petName = petName;
             
             referenceFeatures.push_back(features);
             
             http.end();
             return true;
         }
     }
     
     http.end();
     return false;
 }
 
 void syncReferenceImages() {
     Serial.println("\n════════════════════════════════════════");
     Serial.println("  SYNCING REFERENCE IMAGES");
     Serial.println("════════════════════════════════════════");
     
     if (WiFi.status() != WL_CONNECTED) {
         Serial.println("✗ WiFi not connected, skipping sync");
         return;
     }
     
     referenceFeatures.clear();
     
     HTTPClient http;
     String url = supabaseUrl + "/rest/v1/pets?user_id=eq." + userId + 
                  "&select=id,name,reference_image_url";
     
     http.begin(url);
     http.addHeader("apikey", supabaseKey);
     http.addHeader("Authorization", "Bearer " + supabaseKey);
     http.setTimeout(HTTP_TIMEOUT_MS);
     
     int httpCode = http.GET();
     
     if (httpCode == 200) {
         String payload = http.getString();
         
         DynamicJsonDocument doc(8192);
         deserializeJson(doc, payload);
         
         JsonArray pets = doc.as<JsonArray>();
         Serial.printf("Found %d pets for user\n\n", pets.size());
         
         for (JsonObject pet : pets) {
             String petId = pet["id"].as<String>();
             String petName = pet["name"].as<String>();
             String imageUrl = pet["reference_image_url"].as<String>();
             
             Serial.printf("  Downloading: %s\n", petName.c_str());
             
             if (downloadAndExtractFeatures(petId, petName, imageUrl)) {
                 Serial.printf("    ✓ Features extracted\n");
             } else {
                 Serial.printf("    ✗ Failed\n");
             }
         }
         
         Serial.printf("\n✓ Sync complete: %d pets loaded\n", referenceFeatures.size());
         Serial.println("════════════════════════════════════════\n");
         
     } else {
         Serial.printf("✗ Failed to fetch pets: HTTP %d\n", httpCode);
     }
     
     http.end();
 }
 
 #endif // FEATURE_MATCHING_H