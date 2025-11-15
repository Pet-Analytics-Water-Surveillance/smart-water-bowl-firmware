# Power Supply Troubleshooting Guide

## 🔴 Problem: Intermittent "I2C Device Not Found" Errors

**Cause:** Your Grove AI Vision module is experiencing power brownouts during high current draw periods.

## Current Setup Analysis

**Your Power Budget:**
- Power Supply: 3.3V @ 1A = **3.3W total**
- Grove AI Vision: **200-400mA peak** (camera + AI processing)
- RD-03 Radar: **100-150mA**
- ESP32-S3: **80-260mA** (WiFi active)
- Water Pump (when on): **100-300mA** (depends on pump)
- **Total Peak Current: ~850mA - 1.1A** ⚠️ **EXCEEDS YOUR 1A SUPPLY!**

### Why This Causes I2C Failures

1. **Voltage Drop:** When all devices draw current simultaneously, the 3.3V rail can drop to 3.0V or lower
2. **I2C Communication:** Grove AI requires stable 3.3V for reliable I2C
3. **Brownout:** Momentary voltage dips cause the Grove AI to lose I2C communication
4. **Result:** "Device not found" errors

---

## ✅ SOLUTIONS (Ranked by Effectiveness)

### Solution 1: Upgrade Power Supply (BEST)
**Recommended:** Use a **3.3V @ 2A or 3A** power supply

**Why:** Provides headroom for current spikes

**Where to buy:**
- Any 3.3V regulated power supply rated 2A+
- Alternatively: Use 5V supply + LM1117-3.3 or AMS1117-3.3 regulator (with heat sink)

**Cost:** $5-15

---

### Solution 2: Add Bulk Capacitors (EASY & CHEAP) ⭐
**Add capacitors near the Grove AI module:**
- **1x 470µF electrolytic capacitor** (rated 6.3V or higher)
- **1x 100nF ceramic capacitor** (0.1µF)

**Why:** Capacitors act as local energy storage, providing current during spikes

**How to install:**
```
      Grove AI
         |
    [3.3V]   [GND]
      |       |
     [+]     [-]  ← 470µF electrolytic (watch polarity!)
      |       |
      └───┬───┘
          |
         [≡]  ← 100nF ceramic
```

**Where to solder:**
- As close to the Grove AI power pins as possible
- Or on your breadboard/PCB power rails

**Cost:** $1-3

---

### Solution 3: Separate Power Rails
**Use dedicated power for high-current devices:**

```
[Power Supply 1: 3.3V 1A]
    ├─ ESP32-S3
    ├─ Grove AI Vision
    └─ RD-03 Radar

[Power Supply 2: 5V or 12V]
    └─ Water Pump (via relay)
```

**Why:** Isolates pump noise and current from sensitive electronics

**Cost:** $5-10 for second PSU

---

### Solution 4: Power Sequencing (SOFTWARE - Already Implemented!)
The firmware now includes:
- ✅ I2C bus recovery function
- ✅ Automatic retry with delays
- ✅ Error counting and adaptive recovery
- ✅ Extended I2C timeouts
- ✅ I2C bus scanning on startup

---

## 🔧 Quick Diagnosis Steps

### 1. Measure Voltage Under Load
Use a multimeter:
1. Connect to 3.3V rail
2. Monitor voltage while Grove AI is capturing images
3. **If voltage drops below 3.1V:** You have a power problem!

### 2. Check Serial Monitor Output
Look for these messages:
```
⚠️  No I2C devices found!
💡 POWER ISSUE? Check:
   - Power supply current rating (need 2A+ recommended)
```

### 3. Test Without Pump
Temporarily disable pump to see if issues persist:
- If issues go away → Pump is causing voltage drops
- If issues persist → Core power supply is insufficient

---

## 📋 Immediate Action Plan

### Quick Fix (30 minutes):
1. **Add capacitors** near Grove AI (Solution 2)
2. **Upload new firmware** (already includes software recovery)
3. **Test system**

### Better Fix (1-2 hours):
1. Get a **3.3V @ 2A power supply** (Solution 1)
2. **Keep the capacitors** for extra stability
3. **Separate pump power** if possible (Solution 3)

### Long-term Fix:
Design proper power distribution with:
- Dedicated LDO regulators for each subsystem
- Proper PCB power plane design
- EMI filtering

---

## 🔬 Technical Details

### Grove AI Vision Current Profile
```
Idle:           ~50mA
Standby:        ~80mA
Camera capture: ~200mA
AI inference:   ~300-400mA (peak)
```

### I2C Voltage Requirements
- **Minimum VCC:** 3.0V
- **Typical VCC:** 3.3V
- **Maximum VCC:** 3.6V
- **Below 3.0V:** Unreliable operation, communication errors

### Power Supply Requirements
For reliable operation:
- **Voltage regulation:** ±3% (3.2V - 3.4V)
- **Current capability:** 2x peak load (for margin)
- **Output capacitance:** 100-1000µF recommended
- **Ripple:** <50mV peak-to-peak

---

## 🎯 Expected Results After Fixes

**Before:**
```
[AI] ⚠️  Invoke failed (error count: 3)
⚠️  No I2C devices found!
I2C Device not found (intermittent)
```

**After:**
```
🔍 Scanning I2C bus...
  ✓ Found device at 0x62
✓ AI Vision V2 initialized successfully!
[AI] ✓ Detection accepted: 87% confidence
```

---

## 📞 Still Having Issues?

If problems persist after trying these solutions:

1. **Check I2C wiring:**
   - SDA = GPIO5
   - SCL = GPIO6
   - Pull-up resistors (2.2kΩ - 4.7kΩ) on both lines

2. **Check for shorts/corrosion** on power connections

3. **Try different Grove AI module** (hardware defect possible)

4. **Measure actual current draw** with ammeter

---

## ✅ Firmware Improvements Already Applied

Your firmware now includes:
- ✅ **I2C bus scanner** - Shows all devices on startup
- ✅ **Automatic I2C recovery** - Resets bus when errors occur
- ✅ **Retry logic** - 3 attempts with delays
- ✅ **Error tracking** - Counts failures and adapts
- ✅ **Extended timeouts** - 1 second I2C timeout
- ✅ **Diagnostic messages** - Clear power issue warnings

The software can't fix hardware power issues, but it can make the system more resilient!

---

**TL;DR:** Add a 470µF capacitor near the Grove AI and upgrade to a 2A power supply. This will solve 95% of your I2C issues. 🎯

