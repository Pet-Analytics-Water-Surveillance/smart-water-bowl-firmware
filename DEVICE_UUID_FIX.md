# Device UUID Fix

## Problem

Getting HTTP 400 error when uploading drinking events:
```
[Supabase] ✗ HTTP 400
[Supabase] Error: {"code":"22P02","details":null,"hint":null,
                   "message":"invalid input syntax for type uuid: \"fountain_000000\""}
```

**Cause:** The firmware was generating a device ID from MAC address (`fountain_XXXXXX`) which is just a string, not a valid UUID format that Supabase expects.

---

## Solution

### Firmware Changes ✅ (Already Fixed)

The firmware now expects to receive the **device UUID from the mobile app** during BLE provisioning instead of generating its own.

**Modified files:**
- `ble_provisioning.h` - Added `device_id` field to user credentials

**What changed:**
```cpp
// OLD (BROKEN):
snprintf(idBuf, "fountain_%02X%02X%02X", mac[3], mac[4], mac[5]);
// Generated: "fountain_A1B2C3" ← Not a UUID!

// NEW (FIXED):
receivedDeviceID = doc["device_id"].as<String>();
// Receives: "550e8400-e29b-41d4-a716-446655440000" ← Valid UUID!
```

---

## Mobile App Requirements 🚨

### ⚠️ CRITICAL: Update BLE Provisioning

The mobile app **MUST** include the `device_id` UUID when sending user credentials via BLE.

### Updated JSON Format

**User Characteristic (USER_CHAR_UUID) payload:**

```json
{
  "user_id": "550e8400-e29b-41d4-a716-446655440000",
  "household_id": "660e8400-e29b-41d4-a716-446655440001",
  "device_id": "770e8400-e29b-41d4-a716-446655440002"  // ← ADD THIS!
}
```

### Where to Get device_id

The mobile app should:
1. Create a new device record in Supabase `devices` table during setup
2. Store the UUID returned by Supabase
3. Send that UUID to the firmware via BLE

**Example flow:**

```typescript
// 1. Mobile app creates device in Supabase
const { data: device } = await supabase
  .from('devices')
  .insert({
    name: 'Pet Fountain',
    household_id: currentHouseholdId,
    type: 'fountain'
  })
  .select()
  .single();

// 2. Send device UUID to firmware
const userCreds = {
  user_id: currentUserId,
  household_id: currentHouseholdId,
  device_id: device.id  // ← This is the UUID!
};

// 3. Base64 encode and send via BLE
const encoded = btoa(JSON.stringify(userCreds));
await userCharacteristic.writeValue(encoded);
```

---

## Database Schema

### `devices` table (should already exist)

```sql
CREATE TABLE devices (
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
  household_id UUID REFERENCES households(id),
  name TEXT,
  type TEXT,
  created_at TIMESTAMP DEFAULT NOW()
);
```

### `hydration_events` table

```sql
CREATE TABLE hydration_events (
  id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
  pet_id UUID REFERENCES pets(id),
  device_id UUID REFERENCES devices(id),  -- ← Must be UUID type
  timestamp TIMESTAMP,
  amount_ml INTEGER,
  created_at TIMESTAMP DEFAULT NOW()
);
```

---

## Testing

### 1. Check Current Device ID

Look at firmware serial output during initialization:
```
Initializing Supabase client...
✓ Supabase client initialized
  User ID: 550e8400-e29b-41d4-a716-446655440000
  Household ID: 660e8400-e29b-41d4-a716-446655440001
  Device ID: fountain_A1B2C3  ← BAD! Not a UUID
```

**Good:**
```
  Device ID: 770e8400-e29b-41d4-a716-446655440002  ← Valid UUID format
```

### 2. Test Data Upload

After fixing, you should see:
```
→ STATE: DATA_UPLOAD
[Supabase] ✓ Event logged successfully
```

Instead of:
```
[Supabase] ✗ HTTP 400
[Supabase] Error: invalid input syntax for type uuid
```

---

## Backward Compatibility

**Fallback behavior** (if mobile app doesn't send `device_id`):
- Firmware will generate `fountain_XXXXXX` from MAC address
- **This will fail at Supabase** with UUID error
- User will see warning in serial output:
  ```
  ⚠️  No device_id provided, generating from MAC address
    Generated ID: fountain_A1B2C3 (⚠️  This may not be a valid UUID!)
  ```

**Recommendation:** Always send proper UUID from mobile app!

---

## Migration for Existing Devices

If devices are already provisioned with old firmware:

### Option 1: Re-provision (Recommended)
1. User opens mobile app
2. Taps "Reset Device" or "Re-setup"
3. Goes through BLE provisioning again
4. New firmware receives proper UUID

### Option 2: Manual UUID Assignment
1. Generate UUID for device in Supabase
2. Update device preferences via serial commands (custom script)
3. Or create OTA update mechanism

---

## Serial Commands (Optional Enhancement)

Consider adding a serial command to manually set device UUID:

```cpp
// In main.ino loop or serial handler:
if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    if (cmd.startsWith("SET_UUID:")) {
        String newUuid = cmd.substring(9);
        Preferences prefs;
        prefs.begin("device", false);
        prefs.putString("id", newUuid);
        prefs.end();
        Serial.println("✓ Device UUID updated to: " + newUuid);
        Serial.println("  Restart device for changes to take effect");
    }
}
```

Usage:
```
SET_UUID:770e8400-e29b-41d4-a716-446655440002
```

---

## Summary

### Firmware ✅ Fixed (Updated with Enhanced Alerts)
- Now receives `device_id` from mobile app
- Stores UUID properly
- Sends UUID to Supabase
- **NEW:** Validates UUID format during provisioning
- **NEW:** Validates UUID format on every boot
- **NEW:** Provides clear alerts for invalid/missing UUIDs
- **NEW:** Shows prominent warnings about HTTP 400 errors

### Mobile App ✅ UPDATED
- ✅ Creates device in Supabase BEFORE provisioning
- ✅ Sends `device_id` UUID via BLE
- ✅ Updated user credentials JSON structure
- See: `DEVICE_UUID_APP_UPDATE.md` for details

### Database ✓ Should be OK
- Ensure `device_id` column in `hydration_events` is UUID type
- Ensure `devices` table exists with UUID primary key

---

## Contact Points

**Mobile App Team:** Need to update BLE provisioning to include `device_id`

**Backend Team:** Verify database schema has correct UUID types

**Firmware:** ✅ Already fixed in this commit

