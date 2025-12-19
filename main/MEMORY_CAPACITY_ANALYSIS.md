# Memory Capacity Analysis for Pet Pictures

## Current Memory Available (from your device)

### PSRAM (Fast RAM for image processing)
- **Total**: 8,388,608 bytes (8 MB)
- **Free**: ~8,386,000 bytes
- **Usage**: Temporary image capture and processing

### Flash Storage (LittleFS - Persistent storage)
- **Total**: 1,572,864 bytes (~1.5 MB)
- **Used**: 8,192 bytes (almost empty)
- **Available**: ~1,564,672 bytes (~1.5 MB)

### Heap (Regular RAM)
- **Total**: 328,172 bytes (~320 KB)
- **Free**: ~289,860 bytes (~283 KB)

---

## Current System Design: **Feature Extraction** (Very Efficient!)

Your firmware **does NOT store full images** on the device. Instead, it uses a smart approach:

### What Gets Stored Per Pet:
```cpp
struct ImageFeatures {
    uint8_t colorHist[64];     // 64 bytes  - Color histogram
    float brightness;           // 4 bytes   - Average brightness
    float edgeDensity;          // 4 bytes   - Edge detection metric
    String petId;               // ~40 bytes - UUID string
    String petName;             // ~20 bytes - Pet name
}
// Total per pet: ~132 bytes
```

### Memory Efficiency:
- **Per pet**: ~132 bytes (feature vector only)
- **Maximum pets** (config.h): 10 pets
- **Total memory for 10 pets**: ~1,320 bytes (1.3 KB)

### Current Capacity:
✅ **With feature extraction (current method):**
- **Maximum**: ~1,000+ pets theoretically
- **Configured limit**: 10 pets (set in config.h)
- **Memory used for 10 pets**: Only 1.3 KB!

---

## Alternative: Storing Full JPEG Images

If you wanted to store actual pet images on the device:

### Option 1: Store in PSRAM (Temporary - Lost on restart)
- **Image size**: ~30,000 bytes (30 KB) each
- **PSRAM capacity**: 8,388,608 bytes (8 MB)
- **Maximum pets**: ~279 images
- **Downside**: Lost when device restarts

### Option 2: Store in Flash (LittleFS - Persistent)
- **Image size**: ~30,000 bytes (30 KB) each
- **Flash capacity**: 1,572,864 bytes (1.5 MB)
- **Maximum pets**: ~52 images
- **Downside**: Flash has limited write cycles

### Option 3: Compressed Thumbnails (Recommended if storing images)
- **Thumbnail size**: ~5,000 bytes (5 KB) each at 64x64 resolution
- **Flash capacity**: 1,572,864 bytes (1.5 MB)
- **Maximum pets**: ~314 images
- **Best balance**: Persistent storage + reasonable quantity

---

## Recommendation: **Stick with Current Feature Extraction Method**

### Why Current Method is Better:

✅ **Memory Efficient**: 132 bytes vs 30 KB per pet (227x smaller!)  
✅ **Fast Matching**: Feature comparison is instant  
✅ **Scalable**: Can handle many pets without memory issues  
✅ **Cloud-First**: Full images stay in Supabase (unlimited storage)  
✅ **No Flash Wear**: No repeated writes to flash storage  

### How It Works:
1. Device downloads reference image from Supabase
2. Extracts features (color histogram, brightness, edges)
3. Stores only the tiny feature vector
4. Discards the full image
5. Future captures are compared against stored features

---

## To Increase Pet Capacity

If you need more than 10 pets, edit `config.h` line 43:

**Current:**
```cpp
#define MAX_PETS 10
```

**Change to:** (example for 50 pets)
```cpp
#define MAX_PETS 50
```

**Memory impact**: 50 pets × 132 bytes = 6.6 KB (still negligible!)

---

## Memory Usage Summary

| Storage Type | Capacity | Current Usage | What's Stored |
|--------------|----------|---------------|---------------|
| **PSRAM** | 8 MB | 30 KB | Temporary JPEG buffer for capture |
| **Flash (LittleFS)** | 1.5 MB | 8 KB | Configuration, logs (could store images) |
| **Heap (RAM)** | 320 KB | ~40 KB | Feature vectors, system state |

---

## Bottom Line

### Current Capacity:
🐕 **10 pets configured** (only 1.3 KB memory)  
🐕 **Can easily support 100+ pets** if needed  
🐕 **Images stored in cloud** (unlimited)  

### To Store Full Images on Device:
- **Flash storage**: ~52 full images (30 KB each)
- **Or**: ~314 thumbnails (5 KB each)
- **Not recommended**: Feature extraction is more efficient

---

## Date: November 8, 2025
**Device**: ESP32-S3 with 8MB PSRAM  
**Firmware**: v2.0.0

