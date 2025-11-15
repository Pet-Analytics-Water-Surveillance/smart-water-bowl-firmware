# Heap Corruption Fix

## Problem

System was crashing with heap corruption error:
```
CORRUPT HEAP: Bad tail at 0x3fced180. Expected 0xbaad5678 got 0x65707974
assert failed: multi_heap_free multi_heap_poisoning.c:279 (head != NULL)
```

This occurred when:
1. Motion detected
2. AI detection failed multiple times
3. Automatic I2C recovery was triggered
4. `Wire.end()` was called while AI object had active memory references
5. Heap corruption when AI tried to access invalidated buffers

## Root Cause

**Runtime I2C bus recovery is unsafe with SSCMA library!**

The `Wire.end()` call in `recoverI2CBus()` invalidates I2C buffers and internal state that the SSCMA `AI` object is still referencing. When the library tries to use those references later, it causes heap corruption.

## Solution

**REMOVED all runtime I2C recovery logic** from `detectPet()` and `detectPetWithImage()` functions.

### What Was Removed:
- ❌ Error counting (`i2cErrorCount`)
- ❌ Automatic recovery triggers during normal operation
- ❌ Calls to `recoverI2CBus()` after AI failures
- ❌ Runtime re-initialization of AI object

### What Was Kept:
- ✅ Startup retry logic (safe - happens before AI is active)
- ✅ I2C bus scanning on startup
- ✅ Initial AI.begin() with 3 retries
- ✅ `recoverI2CBus()` function (but marked as startup-only)

## Code Changes

**Before (BROKEN):**
```cpp
if (invokeResult != 0) {
    i2cErrorCount++;
    
    // If we have repeated failures, try recovery
    if (i2cErrorCount >= 3) {
        recoverI2CBus();  // ⚠️ CAUSES HEAP CORRUPTION!
        AI.begin();
    }
    return result;
}
```

**After (FIXED):**
```cpp
if (invokeResult != 0) {
    // Just fail gracefully - don't try recovery
#ifdef DEBUG_AI_VISION
    Serial.println("[AI] ⚠️  Invoke failed");
#endif
    return result;
}
```

## Why This is Safe

1. **Startup retries are OK**: `recoverI2CBus()` during initialization is safe because the AI object hasn't been fully activated yet
2. **No runtime recovery**: Once the system is running, we never call `Wire.end()` which could corrupt active AI object state
3. **Graceful failures**: If I2C fails during operation, the detection simply returns empty results
4. **User can restart**: If persistent I2C issues occur, user can press reset button to re-initialize cleanly

## Alternative Solutions Considered (but rejected)

### Option 1: Smart AI object cleanup before recovery
```cpp
// This doesn't work - SSCMA library has no cleanup API
AI.cleanup();  // No such function exists!
Wire.end();
Wire.begin();
AI.begin();
```
❌ SSCMA library has no proper cleanup/destructor mechanism

### Option 2: Create new AI object
```cpp
AI.~SSCMA();  // Destructor
new (&AI) SSCMA();  // Placement new
AI.begin();
```
❌ Too complex, risks other memory issues

### Option 3: Watchdog timer reset
```cpp
if (too_many_failures) {
    ESP.restart();  // Nuclear option
}
```
❌ Too disruptive to user experience

## Best Practice Going Forward

**Never call `Wire.end()` while any I2C device objects are active!**

If you need I2C recovery in the future:
1. Do it only during initialization
2. Ensure all I2C objects are destroyed/cleaned up first
3. Or use ESP.restart() to reset everything cleanly

## Testing

After this fix:
- ✅ System boots normally
- ✅ AI detection works
- ✅ No heap corruption
- ✅ Graceful handling of I2C failures
- ✅ If I2C issues persist, user can press reset button

If you see detection failures:
1. Check power supply (3.3V @ 2A recommended)
2. Check I2C wiring (SDA=GPIO5, SCL=GPIO6)
3. Press reset button to reinitialize
4. Check for loose connections

