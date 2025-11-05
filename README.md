# Smart Pet Bowl Firmware

<div align="center">

**Version 1.0.0**  
*Real-Time Cloud Architecture*

**By Team P.A.W.S.**  
🌐 [teampaws.app](https://teampaws.app)

---

</div>

## 📋 Overview

This firmware powers the **Smart Pet Bowl** - an intelligent IoT pet water fountain that automatically detects, identifies, and tracks your pets' drinking habits in real-time. Using AI vision, advanced sensors, and cloud connectivity, it provides valuable insights into your pets' hydration patterns.

### Key Features

- 🤖 **AI-Powered Pet Detection** - YOLOv5 object detection model running on edge
- 🎯 **Pet Identification** - Feature matching algorithm to distinguish between multiple pets
- 📊 **Real-Time Tracking** - Instant cloud sync of drinking events
- 💧 **Water Consumption Monitoring** - Precise ultrasonic water level measurement
- 📱 **BLE Provisioning** - Easy setup via mobile app
- 🔄 **Cloud-First Architecture** - Seamless integration with Supabase backend
- ⚡ **FreeRTOS Multi-tasking** - Concurrent operation for optimal performance

---

## 🛠️ Hardware Requirements

### Microcontroller
- **ESP32-S3** (with PSRAM support)
  - Dual-core processor for parallel tasks
  - WiFi + BLE connectivity
  - Sufficient memory for AI inference

### Sensors & Modules

| Component | Model/Type | Purpose |
|-----------|------------|---------|
| AI Vision Camera | Seeed Grove AI Vision V2 (YOLOv5) | Pet detection & image capture |
| Radar Sensor | Ai-Thinker RD-03 (24GHz FMCW) | Presence detection via UART |
| Ultrasonic Sensor | HC-SR04 or similar | Water level measurement |
| Status LED | Any standard LED | System status indication |
| Pump Relay | 5V relay module | Water fountain control |

### Pin Configuration

```cpp
// RD-03 Radar (UART @ 115200 baud)
RD03_RX_PIN      = GPIO 44  // ESP32 RX ← RD-03 TX
RD03_TX_PIN      = GPIO 43  // ESP32 TX → RD-03 RX

// Ultrasonic Water Level
ULTRASONIC_TRIG  = GPIO 1   // Ultrasonic trigger
ULTRASONIC_ECHO  = GPIO 2   // Ultrasonic echo

// Status Indicator
STATUS_LED       = GPIO 7   // Status LED

// Pump Control
PUMP_RELAY       = GPIO 8   // Pump relay

// I2C (Grove AI Vision V2)
I2C_SDA          = GPIO 5   // I2C data (AI camera)
I2C_SCL          = GPIO 6   // I2C clock (AI camera)
```

**⚠️ Important**: RD-03 uses UART communication (not a simple digital pin). See `RD03_INTEGRATION_GUIDE.md` for details.

---

## 🏗️ Software Architecture

### System Components

```
┌─────────────────────────────────────────────┐
│          Main Application (main.ino)        │
├─────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────────────┐ │
│  │ BLE Module   │  │   WiFi Manager       │ │
│  │ (First Boot) │  │   (Normal Operation) │ │
│  └──────────────┘  └──────────────────────┘ │
├─────────────────────────────────────────────┤
│        State Machine & FreeRTOS Tasks       │
├─────────────────────────────────────────────┤
│  ┌──────────┐ ┌──────────┐ ┌─────────────┐  │
│  │ AI Vision│ │ Sensors  │ │  Supabase   │  │
│  │ (YOLOv5) │ │ (Radar + │ │   Client    │  │
│  │          │ │ Ultra.)  │ │             │  │
│  └──────────┘ └──────────┘ └─────────────┘  │
├─────────────────────────────────────────────┤
│         Feature Matching Engine             │
└─────────────────────────────────────────────┘
```

### State Machine Flow

```
IDLE
  ↓ [Motion Detected]
PRESENCE_DETECTED
  ↓ [Pet Detected by AI]
PET_DETECTION
  ↓ [Image Captured]
FEATURE_MATCHING
  ↓ [Pet Identified]
WATER_MEASUREMENT
  ↓ [Pet Leaves or Timeout]
DATA_UPLOAD
  ↓ [Upload Complete]
IDLE
```

### FreeRTOS Tasks

| Task | Priority | Core | Function |
|------|----------|------|----------|
| State Machine | 2 | Core 1 | Main detection and tracking logic |
| Sensor Monitor | 1 | Core 1 | Continuous sensor polling |
| Network | 1 | Core 0 | WiFi maintenance & cloud sync |

---

## 🚀 Getting Started

### Prerequisites

#### Hardware
- Assembled Smart Pet Bowl with all components connected
- USB-C cable for programming
- Stable WiFi network (2.4 GHz)

#### Software
- [Arduino IDE](https://www.arduino.cc/en/software) (v2.0+) or [PlatformIO](https://platformio.org/)
- ESP32 board support package
- Required libraries (see [Dependencies](#dependencies))

### Dependencies

Install these libraries via Arduino Library Manager or PlatformIO:

```ini
# Core Libraries
- ESP32 Board Support (v2.0.11+)

# AI Vision
- Seeed_Arduino_SSCMA

# Networking
- ArduinoJson (v6.21+)
- HTTPClient (built-in)

# BLE
- NimBLE-Arduino (v1.4.1+)

# Storage
- LittleFS (built-in)
- Preferences (built-in)

# Sensors
- Wire (built-in)

# Cryptography
- mbedtls (built-in)
```

### Installation

1. **Clone or Download** this repository
   ```bash
   git clone https://github.com/your-team/smart-pet-bowl-firmware.git
   cd smart-pet-bowl-firmware
   ```

2. **Open in Arduino IDE**
   - Open `main.ino`
   - Select board: **ESP32S3 Dev Module**
   - Configure settings:
     - Flash Size: 16MB (or your board's capacity)
     - PSRAM: OPI PSRAM (if available)
     - Partition Scheme: Default 4MB with spiffs
     - Upload Speed: 921600

3. **Verify Configuration**
   - Review `config.h` for pin assignments
   - Adjust hardware-specific settings if needed:
     ```cpp
     #define TANK_HEIGHT_CM    25.0  // Your tank height
     #define TANK_DIAMETER_CM  12.0  // Your tank diameter
     ```

4. **Upload Firmware**
   - Connect ESP32 via USB
   - Click **Upload**
   - Wait for compilation and upload

5. **First Boot - BLE Provisioning**
   - Device starts in BLE provisioning mode
   - LED blinks slowly (500ms interval)
   - Device name: `PetFountain`
   - Use the P.A.W.S. mobile app to provision:
     - WiFi credentials
     - Supabase backend URL
     - Supabase anonymous key
     - User ID

6. **Normal Operation**
   - After provisioning, device auto-restarts
   - Connects to WiFi
   - Syncs pet reference images from cloud
   - Ready to detect and track pets!

---

## 🔧 Configuration

### Essential Settings (`config.h`)

#### AI Vision Parameters
```cpp
#define CONFIDENCE_THRESHOLD 60    // YOLOv5 confidence (0-100)
#define MATCH_THRESHOLD 0.65       // Feature matching threshold (0.0-1.0)
```

#### System Limits
```cpp
#define MAX_PETS 10                // Maximum registered pets
#define SYNC_INTERVAL_MS 3600000   // Reference image sync interval (1 hour)
```

#### Timeouts
```cpp
#define PRESENCE_TIMEOUT_MS 5000      // Wait time for pet detection
#define DRINKING_TIMEOUT_MS 30000     // Max drinking session
#define BLE_TIMEOUT_MS 300000         // BLE provisioning timeout
```

#### Network
```cpp
#define WIFI_CONNECT_TIMEOUT_MS 30000  // WiFi connection timeout
#define HTTP_TIMEOUT_MS 10000          // HTTP request timeout
#define MAX_RETRY_ATTEMPTS 3           // Upload retry attempts
```

### Debug Mode

Enable verbose logging by uncommenting in `config.h`:
```cpp
#define DEBUG_AI_VISION          // AI detection details
#define DEBUG_FEATURE_MATCHING   // Matching scores
#define DEBUG_SENSORS            // Sensor readings
#define DEBUG_SUPABASE           // HTTP requests
```

---

## 📡 Cloud Integration

### Supabase Backend

The firmware integrates with a Supabase backend for:
- **Pet registration** - Store pet profiles with reference images
- **Drinking events** - Real-time drinking activity logging
- **Data analytics** - Historical tracking and insights

### Data Structure

#### Drinking Events Table
```sql
drinking_events (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL,
  pet_id UUID NOT NULL,
  device_id VARCHAR NOT NULL,
  timestamp TIMESTAMP NOT NULL,
  water_consumed_ml INTEGER NOT NULL
)
```

#### Pets Table
```sql
pets (
  id UUID PRIMARY KEY,
  user_id UUID NOT NULL,
  name VARCHAR NOT NULL,
  reference_image_url TEXT NOT NULL
)
```

### API Endpoints

The firmware uses these Supabase REST endpoints:

- `GET /rest/v1/pets?user_id=eq.{user_id}` - Fetch registered pets
- `POST /rest/v1/drinking_events` - Log drinking event

---

## 🧠 How It Works

### Detection & Tracking Pipeline

1. **Presence Detection**
   - Radar sensor continuously monitors for motion
   - When motion detected → activate AI camera

2. **Pet Detection**
   - YOLOv5 model analyzes camera feed
   - Detects pets with confidence threshold (default 60%)
   - Captures image when pet confirmed

3. **Pet Identification**
   - Extract features from captured image:
     - 64-bin color histogram
     - Average brightness
     - Edge density
   - Compare against reference features from cloud
   - Match with highest similarity score (>65% threshold)

4. **Water Measurement**
   - Record initial water level (ultrasonic sensor)
   - Wait for pet to drink (monitor presence)
   - Record final water level after pet leaves
   - Calculate consumption: `volume = π × r² × height_change`

5. **Data Upload**
   - Create drinking event record
   - Upload to Supabase via REST API
   - Retry up to 3 times on failure
   - Visual feedback (LED blinks) on success

6. **Return to Idle**
   - Reset state machine
   - Clear temporary data
   - Ready for next detection

### Water Volume Calculation

```cpp
float radius = TANK_DIAMETER_CM / 2.0;
float volumeCm3 = PI * radius * radius * levelCm;
int volumeMl = (int)volumeCm3;  // 1 cm³ ≈ 1 ml
```

### Low Water Alert

- Triggers when water level < 5 cm
- Buzzer beeps (100ms)
- Logged to serial output

---

## 🔒 Security & Privacy

- **BLE Provisioning** - Only runs on first boot, then disabled
- **Credentials Storage** - Encrypted storage via ESP32 Preferences (NVS)
- **HTTPS Ready** - Supabase client supports SSL/TLS
- **Local Processing** - AI inference runs on-device (no image upload)

---

## 📊 Monitoring & Debugging

### Serial Monitor

Baud rate: **115200**

#### Normal Operation Output
```
╔═══════════════════════════════════════════╗
║   SMART PET FOUNTAIN v2.0                ║
║   Real-Time Cloud Architecture           ║
╚═══════════════════════════════════════════╝

✓ Device provisioned - entering normal operation
✓ WiFi connected
  IP Address: 192.168.1.100
✓ Supabase client initialized
✓ Sync complete: 2 pets loaded
✓ System initialized and ready

→ STATE: PRESENCE_DETECTED
→ STATE: PET_DETECTION
  Confidence: 85%
→ STATE: FEATURE_MATCHING
  Image captured: 15234 bytes
✓ Matched: Fluffy (73.2% confidence)
  Initial water: 18.5 cm
→ STATE: WATER_MEASUREMENT
  Waiting for pet to drink...
  Final water: 17.8 cm
  Water consumed: 78 ml
→ STATE: DATA_UPLOAD
[Supabase] ✓ Event logged successfully
→ STATE: IDLE
```

### LED Status Indicators

| Pattern | Meaning |
|---------|---------|
| Slow blink (500ms) | BLE provisioning mode |
| Solid ON | Processing detection |
| Fast blink (100ms, 3x) | Upload successful |
| Rapid blink | Error/initialization failure |

---

## 🤝 Contributing

This firmware is part of the **Team P.A.W.S.** Smart Pet Bowl project.

### Development Setup

1. Fork the repository
2. Create a feature branch
   ```bash
   git checkout -b feature/your-feature
   ```
3. Make changes and test thoroughly
4. Commit with descriptive messages
   ```bash
   git commit -m "Add: new feature description"
   ```
5. Push and create pull request

### Code Style

- Follow Arduino/C++ naming conventions
- Use meaningful variable names
- Comment complex logic
- Keep functions focused and concise
- Document public APIs

---

## 📝 License

Copyright © 2025 Team P.A.W.S.  
All rights reserved.

This project is part of a capstone/academic project.  
For licensing inquiries, visit [teampaws.app](https://teampaws.app)

---

## 📞 Support & Contact

- **Website**: [teampaws.app](https://teampaws.app)
- **Documentation**: Visit our website for full documentation
- **Issues**: For firmware issues, please use the GitHub issue tracker

---

## 🙏 Acknowledgments

### Libraries & Frameworks
- **Seeed Studio** - SSCMA AI vision library
- **Espressif** - ESP32 Arduino framework
- **h2zero** - NimBLE Bluetooth library
- **Supabase** - Backend-as-a-Service platform

### Hardware Partners
- ESP32-S3 by Espressif Systems
- Seeed Studio for AI camera modules

---

## 🔄 Version History

### v1.0.0 (Current)
- ✨ Complete with cloud-first architecture
- 🚀 YOLOv5 AI vision integration
- 📱 BLE provisioning system
- ⚡ FreeRTOS multi-tasking
- 🔄 Real-time Supabase sync
- 💧 Improved water measurement accuracy

---

<div align="center">

**Made with ❤️ by Team P.A.W.S.**

*Keeping your pets healthy, one sip at a time.* 🐾

</div>

