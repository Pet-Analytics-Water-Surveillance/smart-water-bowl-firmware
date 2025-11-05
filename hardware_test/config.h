/*
 * Configuration and Pin Definitions
 */

 #ifndef CONFIG_H
 #define CONFIG_H
 
 #include <Arduino.h>
 
 // ===== FIRMWARE VERSION =====
 #define FIRMWARE_VERSION "2.0.0"
 
// ===== PIN DEFINITIONS (Updated - Match Hardware Test) =====
// RD-03 Radar Sensor (UART Communication)
#define RD03_RX_PIN       44      // ESP32 RX ← RD-03 TX
#define RD03_TX_PIN       43      // ESP32 TX → RD-03 RX
#define RD03_BAUD_RATE    115200  // RD-03 default baud rate

// Ultrasonic Water Level Sensor
#define ULTRASONIC_TRIG   1       // Trigger pin (TX)
#define ULTRASONIC_ECHO   2       // Echo pin (RX)

// Status LED
#define STATUS_LED        7       // Visual status indicator

// Pump Control
#define PUMP_RELAY        8       // Relay control for water pump

// I2C pins (Grove AI Vision V2)
#define I2C_SDA           5      // I2C Data line
#define I2C_SCL           6      // I2C Clock line
 
 // ===== HARDWARE CONFIGURATION =====
 #define TANK_HEIGHT_CM    25.0
 #define TANK_DIAMETER_CM  12.0
 #define LOW_WATER_THRESHOLD_CM 5.0
 
 // ===== AI VISION SETTINGS =====
 #define CONFIDENCE_THRESHOLD 60    // YOLOv5 detection confidence (0-100)
 #define MATCH_THRESHOLD 0.65       // Feature matching threshold (0.0-1.0)
 
 // ===== SYSTEM SETTINGS =====
 #define MAX_PETS 10                // Maximum pets to store
 #define SYNC_INTERVAL_MS 3600000   // Sync reference images every 1 hour
 #define WATER_READING_SAMPLES 5    // Moving average window
 
 // ===== MEMORY ALLOCATION =====
 #define JPEG_BUFFER_SIZE 30000     // 30KB for captured images
 
 // ===== BLE SERVICE UUIDs =====
 // These UUIDs are standard across all devices
 #define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
 #define WIFI_CHAR_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8"
 #define SUPABASE_CHAR_UUID  "1c95d5e3-d8f7-413a-bf3d-7a2e5d7be87e"
 #define USER_CHAR_UUID      "9a8ca5ed-2b1f-4b5e-9c3d-5e8f7a9d4c3b"
 #define STATUS_CHAR_UUID    "7d4c3b2a-1e9f-4a5b-8c7d-6e5f4a3b2c1d"
 
 // ===== TIMING CONFIGURATION =====
 #define PRESENCE_TIMEOUT_MS     5000   // Max wait for pet detection
 #define DRINKING_TIMEOUT_MS     30000  // Max drinking session time
 #define BLE_TIMEOUT_MS          300000 // 5 min BLE provisioning timeout
 
 // ===== NETWORK SETTINGS =====
 #define WIFI_CONNECT_TIMEOUT_MS 30000  // 30 seconds to connect
 #define HTTP_TIMEOUT_MS         10000  // 10 seconds for HTTP requests
 #define MAX_RETRY_ATTEMPTS      3      // Retry failed uploads
 
 // ===== DEBUG SETTINGS =====
 // Uncomment to enable verbose logging
 // #define DEBUG_AI_VISION
 // #define DEBUG_FEATURE_MATCHING
 // #define DEBUG_SENSORS
 // #define DEBUG_SUPABASE
 
 #endif // CONFIG_H