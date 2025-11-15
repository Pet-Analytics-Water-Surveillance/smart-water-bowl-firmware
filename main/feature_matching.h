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

// Single pet mode optimization
bool singlePetMode = false;
String singlePetId = "";
String singlePetName = "";
 
// External variables
extern String supabaseUrl;
extern String supabaseKey;
extern String userId;
extern String householdId;
extern uint8_t* jpegBuffer;
 
void initializeStorage() {
    Serial.println("Initializing LittleFS...");
    
    // Try to mount without formatting first
    if (!LittleFS.begin(false)) {
        Serial.println("⚠️  LittleFS mount failed, attempting recovery...");
        
        // Try formatting
        if (!LittleFS.begin(true)) {
            Serial.println("✗ LittleFS format failed");
            Serial.println("⚠️  Continuing without storage (reference images will sync from cloud)");
            return;
        }
        
        Serial.println("✓ LittleFS formatted successfully");
    }
    
    size_t totalBytes = LittleFS.totalBytes();
    size_t usedBytes = LittleFS.usedBytes();
    Serial.printf("✓ LittleFS mounted: %d / %d bytes used (%.1f%% full)\n", 
                  usedBytes, totalBytes, (float)usedBytes / totalBytes * 100);
    
    // Check if filesystem is healthy
    if (usedBytes > totalBytes * 0.95) {
        Serial.println("⚠️  LittleFS nearly full, consider clearing old data");
    }
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
    // Check available memory before download
    size_t freeHeap = ESP.getFreeHeap();
    if (freeHeap < 50000) {
        Serial.printf("  ✗ Low memory: %d bytes free\n", freeHeap);
        return false;
    }
    
    HTTPClient http;
    http.begin(imageUrl);
    http.setTimeout(15000);  // 15 second timeout for downloads
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setReuse(false);  // Don't reuse connection
    http.setUserAgent("ESP32");
    
    int httpCode = http.GET();
    
    if (httpCode != 200) {
        Serial.printf("  ✗ HTTP error: %d\n", httpCode);
        http.end();
        return false;
    }
    
    int len = http.getSize();
    
#ifdef DEBUG_FEATURE_MATCHING
    Serial.printf("  HTTP Content-Length: %d\n", len);
    String contentType = http.header("Content-Type");
    Serial.printf("  Content-Type: %s\n", contentType.c_str());
#endif
    
    // Safety check: ensure size is valid and within bounds
    if (len <= 0 || len > JPEG_BUFFER_SIZE) {
        Serial.printf("  ✗ Invalid size: %d bytes (max: %d)\n", len, JPEG_BUFFER_SIZE);
        http.end();
        return false;
    }
    
    // Clear buffer before reading
    memset(jpegBuffer, 0, JPEG_BUFFER_SIZE);
    
    WiFiClient* stream = http.getStreamPtr();
    
    // Read data in chunks with proper waiting
    size_t totalRead = 0;
    unsigned long downloadStart = millis();
    const unsigned long downloadTimeout = 15000;  // 15 second timeout
    
    while (totalRead < len && millis() - downloadStart < downloadTimeout) {
        // Check if data is available or connection is alive
        if (stream->available() > 0) {
            // Read available data (up to remaining bytes needed)
            size_t remaining = len - totalRead;
            size_t available = stream->available();
            size_t toRead = (available < remaining) ? available : remaining;
            
            int bytesRead = stream->read(jpegBuffer + totalRead, toRead);
            if (bytesRead > 0) {
                totalRead += bytesRead;
            }
        } else if (!stream->connected()) {
            // Connection closed
            break;
        } else {
            // Wait a bit for more data
            delay(10);
            yield();
        }
        
        yield();  // Watchdog reset
    }
    
    http.end();
    
    if (totalRead != len) {
        Serial.printf("  ✗ Incomplete download: %d/%d bytes\n", totalRead, len);
        return false;
    }
    
    // Extract features
    ImageFeatures features = extractFeatures(jpegBuffer, totalRead);
    features.petId = petId;
    features.petName = petName;
    
    // Check if we have space in vector
    if (referenceFeatures.size() >= MAX_PETS * 3) {
        Serial.println("  ✗ Reference storage full");
        return false;
    }
    
    referenceFeatures.push_back(features);
    
    Serial.printf("  ✓ Loaded (%d bytes, %d KB free)\n", totalRead, freeHeap / 1024);
    
    // Small delay to allow memory to settle
    delay(100);
    
    return true;
}
 
void syncReferenceImages() {
    Serial.println("\n════════════════════════════════════════");
    Serial.println("  SYNCING REFERENCE IMAGES");
    Serial.println("════════════════════════════════════════");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("✗ WiFi not connected, skipping sync");
        return;
    }
    
    // Print memory before starting
    Serial.printf("Memory before sync: %d KB free heap, %d KB free PSRAM\n", 
                  ESP.getFreeHeap() / 1024, ESP.getFreePsram() / 1024);
    
    referenceFeatures.clear();
    referenceFeatures.reserve(MAX_PETS * 3); // Pre-allocate to reduce fragmentation
    
    HTTPClient http;
    http.setReuse(false); // Prevent connection reuse issues
    
    // First, get all pets in household
    String petsUrl = supabaseUrl + "/rest/v1/pets?household_id=eq." + householdId + 
                     "&select=id,name";
     
    http.begin(petsUrl);
    http.addHeader("apikey", supabaseKey);
    http.addHeader("Authorization", "Bearer " + supabaseKey);
    http.setTimeout(HTTP_TIMEOUT_MS);
     
    int httpCode = http.GET();
    Serial.printf("Fetching pets: %d\n", httpCode);
    
    if (httpCode != 200) {
        Serial.printf("✗ Failed to fetch pets: HTTP %d\n", httpCode);
        http.end();
        return;
    }
    
    String payload = http.getString();
    http.end();
    
    // Use static JSON document allocated on heap with proper size
    DynamicJsonDocument* petsDoc = new DynamicJsonDocument(8192);
    if (!petsDoc) {
        Serial.println("✗ Failed to allocate JSON document");
        return;
    }
    
    DeserializationError error = deserializeJson(*petsDoc, payload);
    
    if (error) {
        Serial.printf("✗ JSON parse error: %s\n", error.c_str());
        delete petsDoc;
        return;
    }
    
    JsonArray pets = petsDoc->as<JsonArray>();
    Serial.printf("Found %d pets\n", pets.size());
    
    if (pets.size() == 0) {
        Serial.println("⚠️  No pets registered!");
        Serial.println("   Use 'Train AI' in the mobile app to add pet photos.");
        Serial.println("════════════════════════════════════════\n");
        delete petsDoc;
        singlePetMode = false;
        return;
    }
    
    // Check for single pet mode optimization
    if (pets.size() == 1) {
        singlePetMode = true;
        singlePetId = pets[0]["id"].as<String>();
        singlePetName = pets[0]["name"].as<String>();
        Serial.println("\n🎯 SINGLE PET MODE ENABLED!");
        Serial.printf("   Only one pet registered: %s\n", singlePetName.c_str());
        Serial.println("   ✓ Skipping AI detection & feature matching");
        Serial.println("   ✓ All events will be logged to this pet");
        Serial.println("   ✓ Power consumption optimized\n");
    } else {
        singlePetMode = false;
        Serial.println();
    }
    
    int totalPhotos = 0;
    int failedPhotos = 0;
    
    // For each pet, fetch all training photos
    for (JsonObject pet : pets) {
        String petId = pet["id"].as<String>();
        String petName = pet["name"].as<String>();
        
        Serial.printf("📷 %s:\n", petName.c_str());
        
        // Check memory before fetching photos
        size_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < 40000) {
            Serial.printf("  ⚠️  Low memory (%d KB), skipping remaining pets\n", freeHeap / 1024);
            break;
        }
        
        // Query pet_photos table for all photos of this pet
        String photosUrl = supabaseUrl + "/rest/v1/pet_photos?pet_id=eq." + petId + 
                          "&select=thumbnail_url&order=uploaded_at.desc&limit=3";
        
        http.begin(photosUrl);
        http.addHeader("apikey", supabaseKey);
        http.addHeader("Authorization", "Bearer " + supabaseKey);
        http.setTimeout(HTTP_TIMEOUT_MS);
        
        int photoCode = http.GET();
        
        if (photoCode == 200) {
            String photoPayload = http.getString();
            http.end();
            
            // Use separate document for photos
            DynamicJsonDocument* photosDoc = new DynamicJsonDocument(6144);
            if (!photosDoc) {
                Serial.println("  ✗ Failed to allocate photos JSON document");
                continue;
            }
            
            DeserializationError photoError = deserializeJson(*photosDoc, photoPayload);
            
            if (!photoError) {
                JsonArray photos = photosDoc->as<JsonArray>();
                int photoCount = photos.size();
                
                if (photoCount == 0) {
                    Serial.printf("  ⚠️  No training photos found\n");
                } else {
                    Serial.printf("  Found %d training photo(s)\n", photoCount);
                    
                    for (JsonObject photo : photos) {
                        String thumbnailUrl = photo["thumbnail_url"].as<String>();
                        
                        if (thumbnailUrl.length() > 0) {
                            // Small delay between downloads to prevent overwhelming the system
                            delay(200);
                            
                            if (downloadAndExtractFeatures(petId, petName, thumbnailUrl)) {
                                totalPhotos++;
                            } else {
                                failedPhotos++;
                            }
                            
                            // Watchdog reset
                            yield();
                        }
                    }
                }
            } else {
                Serial.printf("  ✗ Photos JSON parse error: %s\n", photoError.c_str());
            }
            
            delete photosDoc;
        } else {
            http.end();
            Serial.printf("  ✗ Failed to fetch photos: HTTP %d\n", photoCode);
        }
        
        Serial.println();
        
        // Watchdog reset between pets
        yield();
    }
    
    delete petsDoc;
    
    if (singlePetMode) {
        Serial.println("✓ Single pet mode active - no photos needed");
    } else {
        Serial.printf("✓ Sync complete: %d photos loaded, %d failed\n", totalPhotos, failedPhotos);
        Serial.printf("  %d unique pets can be recognized\n", referenceFeatures.size());
    }
    Serial.printf("  Memory after sync: %d KB free heap\n", ESP.getFreeHeap() / 1024);
    Serial.println("════════════════════════════════════════\n");
}

// Get the single pet's ID (for single pet mode)
String getSinglePetId() {
    if (singlePetMode) {
        return singlePetId;
    }
    return "";
}

// Check if system is in single pet mode
bool isSinglePetMode() {
    return singlePetMode;
}
 
 #endif // FEATURE_MATCHING_H