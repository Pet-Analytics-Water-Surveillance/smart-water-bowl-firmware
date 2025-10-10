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
 
 void initializeAIVision() {
     Serial.println("Initializing AI Vision V2...");
     
     // Allocate JPEG buffer in PSRAM
     jpegBuffer = (uint8_t*)ps_malloc(JPEG_BUFFER_SIZE);
     if (!jpegBuffer) {
         Serial.println("✗ Failed to allocate JPEG buffer");
         while(1) { 
             digitalWrite(STATUS_LED, HIGH);
             delay(100);
             digitalWrite(STATUS_LED, LOW);
             delay(100);
         }
     }
     
     // Initialize I2C
     Wire.begin(I2C_SDA, I2C_SCL);
     Wire.setClock(100000);
     
     // Initialize AI Vision
     if (!AI.begin()) {
         Serial.println("✗ AI Vision V2 initialization failed");
         while(1) { 
             digitalWrite(STATUS_LED, HIGH);
             delay(100);
             digitalWrite(STATUS_LED, LOW);
             delay(100);
         }
     }
     
     Serial.println("✓ AI Vision V2 initialized");
     Serial.println("  Model: YOLOv5 Pet Detection");
     Serial.println("  Resolution: 224x224");
 }
 
 DetectionResult detectPet() {
     DetectionResult result = {false, 0, {0, 0, 0, 0}, 0};
     
     // Invoke YOLOv5 model
     if (!AI.invoke()) {
         if (AI.boxes().size() > 0) {
             auto box = AI.boxes()[0];
             
             if (box.score >= CONFIDENCE_THRESHOLD) {
                 result.detected = true;
                 result.confidence = box.score;
                 result.bbox[0] = box.x;
                 result.bbox[1] = box.y;
                 result.bbox[2] = box.w;
                 result.bbox[3] = box.h;
                 
 #ifdef DEBUG_AI_VISION
                 Serial.printf("[AI] Pet detected: %d%% confidence\n", box.score);
                 Serial.printf("[AI] BBox: x=%d, y=%d, w=%d, h=%d\n", 
                              box.x, box.y, box.w, box.h);
 #endif
             }
         }
     }
     
     return result;
 }
 
 DetectionResult detectPetWithImage() {
     DetectionResult result = {false, 0, {0, 0, 0, 0}, 0};
     
     // Invoke with image capture
     if (!AI.invoke(1, false, true)) {
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
     }
     
     return result;
 }
 
 #endif // AI_VISION_H