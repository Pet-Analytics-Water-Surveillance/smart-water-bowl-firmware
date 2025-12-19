# Image Transfer Process: Grove AI Vision V2 → ESP32-S3

## Date: November 8, 2025

---

## Complete Transfer Flow

```
┌─────────────────────────────────────────────────────────────────┐
│                    IMAGE TRANSFER PROCESS                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────┐         I2C Bus (400 KB/s)             │
│  │  Grove AI Vision V2 │◄──────────────────────────┐            │
│  │  (Himax WE2 chip)   │                           │            │
│  └─────────────────────┘                           │            │
│           │                                         │            │
│           │ 1. Camera captures frame                │            │
│           │    (224x224 pixels)                     │            │
│           ▼                                         │            │
│   ┌───────────────┐                                 │            │
│   │  YOLOv5 Model │                                 │            │
│   │  (on AI chip) │                                 │            │
│   └───────────────┘                                 │            │
│           │                                         │            │
│           │ 2. Detects pet (bounding box)           │            │
│           │    Confidence: 60-100%                  │            │
│           ▼                                         │            │
│   ┌───────────────┐                                 │            │
│   │  JPEG Encoder │                                 │            │
│   │  (on AI chip) │                                 │            │
│   └───────────────┘                                 │            │
│           │                                         │            │
│           │ 3. Compresses to JPEG (~10-30 KB)       │            │
│           ▼                                         │            │
│   ┌───────────────┐                                 │            │
│   │ Base64 Encoder│                                 │            │
│   └───────────────┘                                 │            │
│           │                                         │            │
│           │ 4. Encodes as Base64 string             │            │
│           │    (for safe I2C transfer)              │            │
│           ▼                                         │            │
│   ╔═════════════════════════════════════╗           │            │
│   ║  I2C TRANSMISSION                   ║───────────┘            │
│   ║  Base64 string chunks sent via I2C  ║                        │
│   ║  Speed: ~100-400 KB/s               ║                        │
│   ║  Transfer time: ~100-500ms          ║                        │
│   ╚═════════════════════════════════════╝                        │
│           │                                                       │
│           ▼                                                       │
│  ┌──────────────────────┐                                        │
│  │   ESP32-S3           │                                        │
│  │   (Main Controller)  │                                        │
│  └──────────────────────┘                                        │
│           │                                                       │
│           │ 5. Receives Base64 string via I2C                    │
│           ▼                                                       │
│   ┌────────────────────┐                                         │
│   │  Base64 Decoder    │                                         │
│   │  (mbedTLS library) │                                         │
│   └────────────────────┘                                         │
│           │                                                       │
│           │ 6. Decodes to raw JPEG bytes                         │
│           ▼                                                       │
│   ┌────────────────────┐                                         │
│   │  PSRAM Buffer      │                                         │
│   │  (30 KB allocated) │                                         │
│   │  jpegBuffer[]      │                                         │
│   └────────────────────┘                                         │
│           │                                                       │
│           │ 7. JPEG stored in memory                             │
│           ▼                                                       │
│   ┌────────────────────┐                                         │
│   │ Feature Extraction │                                         │
│   │ (color, brightness,│                                         │
│   │  edge detection)   │                                         │
│   └────────────────────┘                                         │
│           │                                                       │
│           │ 8. Extract features, discard JPEG                    │
│           ▼                                                       │
│       IDENTIFICATION                                             │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Technical Details

### Hardware Connection

**Physical Connection:**
- **I2C SDA (Data)**: GPIO 5 (ESP32) ↔ SDA (Grove AI)
- **I2C SCL (Clock)**: GPIO 6 (ESP32) ↔ SCL (Grove AI)
- **Power**: 5V and GND shared

**I2C Configuration:**
```cpp
Wire.begin(I2C_SDA, I2C_SCL);  // SDA=5, SCL=6
Wire.setClock(100000);          // 100 kHz (conservative, can go up to 400 kHz)
```

### Step-by-Step Code Flow

#### Step 1: Request Image Capture (ESP32 → Grove AI)

```cpp
// In ai_vision.h, line 123
DetectionResult detectPetWithImage() {
    // Invoke YOLOv5 with image capture enabled
    //                    times, filter, capture
    if (!AI.invoke(1,    false,  true)) {
        //                        ↑
        //                        └── Tells Grove AI to capture JPEG
```

**What happens:**
- ESP32 sends I2C command to Grove AI Vision
- Grove AI captures frame from camera sensor
- YOLOv5 model runs on the Grove AI chip (not ESP32)
- If pet detected, JPEG encoding begins

#### Step 2: Image Encoding (Grove AI Chip)

**On the Grove AI Vision module (Himax WE2 chip):**
1. Camera sensor captures 224×224 RGB frame
2. YOLOv5 model detects pet (bounding box + confidence)
3. If confidence ≥ 60%, encode full frame as JPEG
4. JPEG compression reduces size (typically 10-30 KB)
5. Convert JPEG bytes to Base64 string (safe for I2C)

**Why Base64?**
- I2C is byte-oriented but can have issues with binary data
- Base64 ensures safe transmission (no null bytes, control chars)
- Standard practice for embedded I2C image transfer

#### Step 3: I2C Transfer (Grove AI → ESP32)

```cpp
// In ai_vision.h, line 128
String base64Image = AI.last_image();
//                   ↑
//                   └── Reads Base64 string via I2C
```

**What happens:**
- ESP32 requests image data via I2C
- Grove AI sends Base64 string in chunks
- SSCMA library handles chunked I2C reads
- Transfer takes ~100-500ms depending on image size

**Transfer speed:**
- I2C clock: 100-400 kHz
- Typical throughput: ~100-400 KB/s
- 30KB image: ~100-300ms transfer time

#### Step 4: Base64 Decoding (ESP32)

```cpp
// In ai_vision.h, lines 133-137
int ret = mbedtls_base64_decode(
    jpegBuffer,              // Output: PSRAM buffer
    JPEG_BUFFER_SIZE,        // Max 30,000 bytes
    &outputLen,              // Actual decoded size
    (const unsigned char*)base64Image.c_str(),  // Input: Base64 string
    base64Image.length()     // Input length
);
```

**What happens:**
- mbedTLS library decodes Base64 → raw JPEG bytes
- JPEG data written directly to PSRAM buffer
- `outputLen` contains actual JPEG size (typically 10-30 KB)
- Original Base64 string discarded to free memory

#### Step 5: Feature Extraction (ESP32)

```cpp
// In feature_matching.h, line 45
ImageFeatures extractFeatures(uint8_t* imageData, size_t size) {
    // Processes jpegBuffer contents
    // Extracts color histogram, brightness, edge density
}
```

**What happens:**
- JPEG bytes sampled for features (~2000 pixel samples)
- Color histogram computed (64 bins)
- Brightness and edge density calculated
- Original JPEG remains in buffer for potential cloud upload

#### Step 6: Image Disposal

After feature extraction, the JPEG in `jpegBuffer` can be:
- **Kept temporarily** for cloud upload (optional)
- **Overwritten** by next capture
- **Never saved to flash** (too large, unnecessary)

---

## Memory Journey

### Where the Image Exists

| Location | Format | Size | Duration |
|----------|--------|------|----------|
| **Grove AI Vision** | Raw RGB Frame | ~150 KB | Milliseconds (encoded immediately) |
| **Grove AI Vision** | JPEG Compressed | ~10-30 KB | Temporary (until transfer complete) |
| **Grove AI Vision** | Base64 String | ~13-40 KB | Temporary (during I2C transfer) |
| **ESP32 Heap** | Base64 String | ~13-40 KB | Temporary (during decode) |
| **ESP32 PSRAM** | JPEG Bytes | ~10-30 KB | Until next capture (~5-30 seconds) |
| **ESP32 Heap** | Feature Vector | ~132 bytes | Permanent (until sync) |
| **Cloud (Supabase)** | Original JPEG | ~10-30 KB | Permanent (optional) |

---

## Performance Metrics

### Typical Transfer Times

| Stage | Time | Bottleneck |
|-------|------|------------|
| Camera capture | ~50ms | Camera sensor |
| YOLOv5 inference | ~100ms | AI chip processing |
| JPEG encoding | ~30ms | Compression algorithm |
| Base64 encoding | ~10ms | String conversion |
| I2C transfer | ~100-300ms | **I2C speed (slowest step)** |
| Base64 decoding | ~20ms | CPU processing |
| Feature extraction | ~10ms | Simple math operations |
| **Total** | **~320-510ms** | **End-to-end** |

---

## Why This Approach?

### ✅ Advantages

1. **Centralized Processing**: ESP32 handles all business logic
2. **Feature Extraction**: Can process image for identification
3. **Cloud Upload Ready**: JPEG available if needed
4. **Debugging**: Can log/upload images for analysis
5. **Flexibility**: Can add more image processing later

### ❌ Alternative: Process on Grove AI Only

**Could we keep image on Grove AI and just send detection result?**

```cpp
// Hypothetical - NOT what we do
DetectionResult result = {
    .detected = true,
    .confidence = 85,
    .bbox = {100, 50, 80, 120}
    // No image transferred
};
```

**Problems:**
- ❌ Can't identify WHICH pet (just "a pet was detected")
- ❌ Can't extract features for matching
- ❌ Can't upload evidence image to cloud
- ❌ Can't debug false positives/negatives

---

## Image Size Variations

### Factors Affecting JPEG Size

| Condition | Typical Size | Reason |
|-----------|-------------|--------|
| **Simple background** | ~8-12 KB | High compression ratio |
| **Complex scene** | ~20-30 KB | Low compression ratio |
| **Low light** | ~10-15 KB | Less detail to encode |
| **Bright, detailed** | ~25-35 KB | More detail preserved |

### Example from Your Device

```
14:40:34.718 -> [AI] Captured image: 18,432 bytes
14:40:34.718 -> [AI] Confidence: 82%
```

This is typical! ~18 KB JPEG successfully transferred via I2C.

---

## Troubleshooting

### Common Issues

#### 1. Image Transfer Timeout
**Symptom**: `detectPetWithImage()` returns empty result

**Possible causes:**
- I2C communication failure
- Grove AI Vision not responding
- Buffer too small for image

**Debug:**
```cpp
#define DEBUG_AI_VISION  // Enable in config.h
// Will show: [AI] Captured image: XX bytes
```

#### 2. Base64 Decode Failure
**Symptom**: `mbedtls_base64_decode` returns error code

**Possible causes:**
- Corrupted I2C transmission
- Invalid Base64 string
- Buffer overflow

**Check:**
```cpp
if (ret != 0) {
    Serial.printf("Base64 decode error: %d\n", ret);
}
```

#### 3. Empty Image Data
**Symptom**: `base64Image.length() == 0`

**Possible causes:**
- YOLOv5 didn't detect anything
- Confidence below threshold
- Grove AI didn't capture image

**Solution:**
- Lower CONFIDENCE_THRESHOLD in config.h
- Check camera lens for obstructions
- Verify good lighting conditions

---

## Optimizations

### Current Optimizations

✅ **Direct to PSRAM**: Decoded JPEG goes to PSRAM (not heap)  
✅ **No intermediate copy**: Base64 decoded directly to buffer  
✅ **Discard after use**: JPEG not kept after feature extraction  
✅ **I2C clock**: Set conservatively to 100kHz (reliable)  

### Possible Future Optimizations

#### Option 1: Increase I2C Speed
```cpp
Wire.setClock(400000);  // 400 kHz (4× faster)
// Reduces transfer time from ~300ms to ~75ms
```
**Risk**: Less reliable at higher speeds, depends on wiring quality

#### Option 2: Request Smaller Images
```cpp
// If Grove AI supports it (check SSCMA docs)
AI.setImageSize(160, 160);  // Smaller = faster transfer
```
**Trade-off**: Lower resolution for feature extraction

#### Option 3: Skip Image Transfer Sometimes
```cpp
// Only transfer image every 3rd detection
if (detectionCount % 3 == 0) {
    detectPetWithImage();  // Full image
} else {
    detectPet();           // Metadata only
}
```
**Trade-off**: Slower learning, less data for debugging

---

## Comparison: I2C vs Other Protocols

| Protocol | Speed | Complexity | Wiring | Use Case |
|----------|-------|------------|--------|----------|
| **I2C** (Current) | 100-400 KB/s | Simple | 2 wires | ✅ Short distance, low-medium speed |
| **SPI** | 1-10 MB/s | Medium | 4 wires | High-speed image transfer |
| **UART** | 115-921 KB/s | Simple | 2 wires | Alternative to I2C |
| **WiFi/BT** | 1-100 MB/s | Complex | Wireless | Long distance, high overhead |

**Why I2C for this project:**
- ✅ Grove AI Vision V2 uses I2C (no choice)
- ✅ 100-300ms transfer time is acceptable
- ✅ Simple 2-wire connection
- ✅ Reliable for <1 meter distances

---

## Summary

### Yes, the image IS transferred! Here's the proof:

1. **Line 123** in `ai_vision.h`: `AI.invoke(1, false, true)` - Requests image capture
2. **Line 128** in `ai_vision.h`: `String base64Image = AI.last_image()` - Retrieves via I2C
3. **Lines 133-137**: `mbedtls_base64_decode()` - Decodes to JPEG bytes
4. **Line 146**: `result.imageSize = outputLen` - Confirms image received (~10-30 KB)
5. **Your console log**: `[AI] Captured image: 18,432 bytes` - Proof of transfer!

### Transfer Summary

- **Protocol**: I2C (2-wire serial)
- **Format**: Base64-encoded JPEG
- **Size**: 10-30 KB typical
- **Speed**: ~100-300ms transfer time
- **Destination**: ESP32 PSRAM buffer (30 KB allocated)
- **Purpose**: Feature extraction for pet identification

The Grove AI Vision V2 is essentially a **smart camera** that:
1. Captures images
2. Runs YOLOv5 detection
3. Sends both detection results AND image to ESP32

This gives you the **best of both worlds**: AI acceleration on the Grove chip + flexible processing on the ESP32! 🚀

---

**Document Version**: 1.0  
**Last Updated**: November 8, 2025  
**Hardware**: ESP32-S3 + Grove AI Vision V2 (I2C @ 100kHz)

