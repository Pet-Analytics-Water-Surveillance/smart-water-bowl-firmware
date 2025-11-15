/*
 * AI Vision V2 Integration
 * YOLOv5 Pet Detection
 */

 #ifndef AI_VISION_H
 #define AI_VISION_H
 
 #include <Wire.h>
 #include <Seeed_Arduino_SSCMA.h>
 #include <mbedtls/base64.h>
 #include "config.h"
 
 // Global AI object
 SSCMA AI;
 
 // Global image buffer (allocated in PSRAM)
 uint8_t* jpegBuffer = nullptr;
 
 // Detection result
 struct DetectionResult {
     bool detected;
     int confidence;
     int bbox[4];  // x, y, width, height
     size_t imageSize;
 };
 
// I2C device scanner - useful for debugging
bool scanI2CDevice(uint8_t address) {
    Wire.beginTransmission(address);
    return (Wire.endTransmission() == 0);
}

// I2C bus recovery function (ONLY safe to use during startup, before AI object is active)
void recoverI2CBus() {
    Serial.println("🔄 Attempting I2C bus recovery...");
    
    // NOTE: Do NOT call this while AI object is active - causes heap corruption!
    // This is only safe during initialization retries
    
    // Reset I2C bus
    Wire.end();
    delay(100);
    
    // Re-initialize with longer timeout
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);  // Use safe 100kHz speed
    Wire.setTimeOut(1000);  // 1 second timeout
    
    delay(500);  // Give devices time to stabilize
    
    Serial.println("  ✓ I2C bus reset complete");
}

void initializeAIVision() {
    Serial.println("Initializing AI Vision V2...");
    
    // Check available memory before allocation
    Serial.println("\n📊 Memory before JPEG buffer allocation:");
    Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("  Free PSRAM: %d bytes\n", ESP.getFreePsram());
    Serial.printf("  PSRAM size: %d bytes\n", ESP.getPsramSize());
    Serial.printf("  Needed: %d bytes\n", JPEG_BUFFER_SIZE);
    
    // Check if PSRAM is available
    if (ESP.getPsramSize() == 0) {
        Serial.println("⚠️  WARNING: PSRAM not available!");
        Serial.println("   Make sure to enable PSRAM in Arduino IDE:");
        Serial.println("   Tools > PSRAM > 'OPI PSRAM' or 'QSPI PSRAM'");
    }
    
    // Initialize I2C FIRST - before allocating memory
    Serial.println("[INIT] Initializing I2C bus...");
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(100000);  // Use conservative 100kHz for stability
    Wire.setTimeOut(1000);  // Set 1 second timeout
    delay(500);  // Give I2C time to stabilize
    Serial.println("  ✓ I2C bus ready");
    
    // Scan for I2C devices
    Serial.println("\n🔍 Scanning I2C bus...");
    bool foundGroveAI = false;
    bool foundOtherDevices = false;
    
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        if (scanI2CDevice(addr)) {
            Serial.printf("  ✓ Found device at 0x%02X", addr);
            
            if (addr == 0x62) {
                Serial.println(" ← Grove AI Vision V2 ✓");
                foundGroveAI = true;
            } else {
                Serial.println(" ← Unknown device");
                foundOtherDevices = true;
                
                // Identify common I2C devices
                if (addr == 0x28) {
                    Serial.println("     (0x28 is often: CAP1188 touch sensor or TMP117 temp sensor)");
                } else if (addr == 0x40 || addr == 0x41) {
                    Serial.println("     (0x40/0x41 are often: Si7021, HTU21D humidity sensors)");
                }
            }
        }
    }
    
    Serial.println();
    
    if (!foundGroveAI) {
        Serial.println("  ⚠️  Grove AI Vision V2 NOT FOUND at 0x62!");
        Serial.println("\n🚨 CRITICAL: Grove AI must be at address 0x62 to work!");
        Serial.println("\n💡 MOST LIKELY CAUSE:");
        Serial.println("  → You need to upload a model via SenseCraft AI first!");
        Serial.println("\n📝 Steps to fix:");
        Serial.println("  1. Connect Grove AI V2 to your COMPUTER via USB-C");
        Serial.println("  2. Go to: https://seeed-studio.github.io/SenseCraft-Web-Toolkit/");
        Serial.println("  3. Select a model (Object Detection or Pet Detection)");
        Serial.println("  4. Click 'Deploy Model' and wait for upload to complete");
        Serial.println("  5. Disconnect from computer, reconnect to ESP32");
        Serial.println("  6. Power cycle and try again");
        
        if (foundOtherDevices) {
            Serial.println("\n⚠️  Other I2C devices were found, but not Grove AI!");
            Serial.println("   This suggests I2C bus is working, but Grove AI is:");
            Serial.println("   - Not connected properly, OR");
            Serial.println("   - Doesn't have a model uploaded, OR");
            Serial.println("   - Wrong wiring (check SDA=GPIO5, SCL=GPIO6)");
        } else {
            Serial.println("\n⚠️  NO I2C devices found - check wiring and power!");
        }
        
        Serial.println("\n  Retrying in 3 seconds...");
        delay(3000);
        
        // Try I2C recovery
        recoverI2CBus();
    } else {
        Serial.println("  ✓ Grove AI Vision V2 found and ready!");
    }
    
    // Allocate JPEG buffer in PSRAM
    Serial.printf("Allocating %d bytes for JPEG buffer...\n", JPEG_BUFFER_SIZE);
    jpegBuffer = (uint8_t*)ps_malloc(JPEG_BUFFER_SIZE);
    if (!jpegBuffer) {
        Serial.println("\n❌❌❌ CRITICAL ERROR ❌❌❌");
        Serial.println("✗ Failed to allocate JPEG buffer");
        Serial.printf("  Requested: %d bytes\n", JPEG_BUFFER_SIZE);
        Serial.printf("  Free PSRAM: %d bytes\n", ESP.getFreePsram());
        Serial.printf("  Free heap: %d bytes\n", ESP.getFreeHeap());
        Serial.println("\nPossible solutions:");
        Serial.println("  1. Enable PSRAM in Arduino IDE (Tools > PSRAM)");
        Serial.println("  2. Restart the device");
        Serial.println("  3. Check for memory leaks");
        while(1) { 
            digitalWrite(STATUS_LED, HIGH);
            delay(100);
            digitalWrite(STATUS_LED, LOW);
            delay(100);
        }
    }
     
    // Initialize AI Vision with retry logic
    Serial.println("[INIT] Initializing AI Vision V2...");
    int retries = 3;
    bool initSuccess = false;
    
    for (int attempt = 1; attempt <= retries; attempt++) {
        Serial.printf("  Attempt %d/%d...\n", attempt, retries);
        
        // AI.begin() returns true (non-zero) on SUCCESS, false on failure
        if (AI.begin()) {
            initSuccess = true;
            break;
        } else {
            Serial.printf("  ✗ Attempt %d failed\n", attempt);
            
            if (attempt < retries) {
                Serial.println("  💡 Retrying after delay...");
                delay(2000);
                
                // Try I2C recovery between attempts
                recoverI2CBus();
            }
        }
    }
    
    if (!initSuccess) {
        Serial.println("\n❌ AI Vision V2 initialization FAILED after all retries");
        Serial.println("\n🔧 TROUBLESHOOTING CHECKLIST:");
        Serial.println("  [ ] Power supply provides at least 2A current");
        Serial.println("  [ ] Voltage stays stable at 3.3V (measure with multimeter)");
        Serial.println("  [ ] Add 100-470µF capacitor between 3.3V and GND near Grove AI");
        Serial.println("  [ ] I2C connections: SDA=GPIO5, SCL=GPIO6");
        Serial.println("  [ ] Grove AI has separate/better power source");
        Serial.println("  [ ] Power rails are not shared with high-current devices");
        while(1) { 
            digitalWrite(STATUS_LED, HIGH);
            delay(100);
            digitalWrite(STATUS_LED, LOW);
            delay(100);
        }
    }
     
    Serial.println("✓ AI Vision V2 initialized successfully!");
    Serial.println("  Model: YOLOv5 Pet Detection");
    Serial.println("  Resolution: 224x224");
}
 
DetectionResult detectPet() {
    DetectionResult result = {false, 0, {0, 0, 0, 0}, 0};
    
    // Invoke YOLOv5 model
    int invokeResult = AI.invoke();
    
    if (invokeResult != 0) {
        // AI.invoke() failed - possible I2C communication error
#ifdef DEBUG_AI_VISION
        Serial.println("[AI] ⚠️  Invoke failed");
#endif
        return result;
    }
    
    if (AI.boxes().size() > 0) {
        auto box = AI.boxes()[0];
        
#ifdef DEBUG_AI_VISION
        Serial.printf("[AI] Box found: %d%% confidence (threshold: %d%%)\n", 
                     box.score, CONFIDENCE_THRESHOLD);
        Serial.printf("[AI] BBox: x=%d, y=%d, w=%d, h=%d\n", 
                     box.x, box.y, box.w, box.h);
#endif
        
        if (box.score >= CONFIDENCE_THRESHOLD) {
            result.detected = true;
            result.confidence = box.score;
            result.bbox[0] = box.x;
            result.bbox[1] = box.y;
            result.bbox[2] = box.w;
            result.bbox[3] = box.h;
            
            Serial.printf("[AI] ✓ Detection accepted: %d%% confidence\n", box.score);
        } else {
            Serial.printf("[AI] ✗ Detection rejected: %d%% < %d%% threshold\n", 
                         box.score, CONFIDENCE_THRESHOLD);
        }
    } else {
#ifdef DEBUG_AI_VISION
        Serial.println("[AI] No boxes detected by YOLO");
#endif
    }
    
    return result;
}
 
 DetectionResult detectPetWithImage() {
     DetectionResult result = {false, 0, {0, 0, 0, 0}, 0};
     
     // Invoke with image capture
     int invokeResult = AI.invoke(1, false, true);
     
     if (invokeResult != 0) {
         // Invoke failed
#ifdef DEBUG_AI_VISION
         Serial.println("[AI] ⚠️  Invoke with image failed");
#endif
         return result;
     }
     
     if (AI.boxes().size() > 0) {
         auto box = AI.boxes()[0];
         
         if (box.score >= CONFIDENCE_THRESHOLD) {
             String base64Image = AI.last_image();
             
             if (base64Image.length() > 0) {
                 // Decode Base64 to JPEG
                 size_t outputLen = 0;
                 int ret = mbedtls_base64_decode(
                     jpegBuffer, JPEG_BUFFER_SIZE, &outputLen,
                     (const unsigned char*)base64Image.c_str(), 
                     base64Image.length()
                 );
                 
                 if (ret == 0 && outputLen > 0) {
                     result.detected = true;
                     result.confidence = box.score;
                     result.bbox[0] = box.x;
                     result.bbox[1] = box.y;
                     result.bbox[2] = box.w;
                     result.bbox[3] = box.h;
                     result.imageSize = outputLen;
                     
 #ifdef DEBUG_AI_VISION
                     Serial.printf("[AI] Captured image: %d bytes\n", outputLen);
                     Serial.printf("[AI] Confidence: %d%%\n", box.score);
 #endif
                 }
             }
         }
     }
     
     return result;
 }
 
 #endif // AI_VISION_H