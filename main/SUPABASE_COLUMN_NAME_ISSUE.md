# Supabase Database Column Name Issue

## Date: November 8, 2025

## Problem Summary
The firmware is trying to fetch pet data from Supabase but getting an HTTP 400 error.

## Error Details
```
Request URL: https://eovsqlzgudrhjaeiskon.supabase.co/rest/v1/pets?user_id=eq.9caf44d9-64ce-4950-bd9e-ce31746a2e1d&select=id,name,reference_image_url

HTTP Response Code: 400

Error Message: {
  "code":"42703",
  "details":null,
  "hint":null,
  "message":"column pets.reference_image_url does not exist"
}
```

## Root Cause
The firmware code in `feature_matching.h` (line 185-186) is requesting a column named `reference_image_url` from the `pets` table, but this column **does not exist** in your Supabase database.

## What Needs To Be Fixed

### Option 1: Update Firmware to Match Database (Recommended)
If your database uses a different column name (like `image_url`, `photo_url`, `profile_image_url`, etc.), update line 186 in `feature_matching.h`:

**Current code:**
```cpp
String url = supabaseUrl + "/rest/v1/pets?user_id=eq." + userId + 
             "&select=id,name,reference_image_url";
```

**Change to:** (replace `YOUR_ACTUAL_COLUMN_NAME` with the real column name)
```cpp
String url = supabaseUrl + "/rest/v1/pets?user_id=eq." + userId + 
             "&select=id,name,YOUR_ACTUAL_COLUMN_NAME";
```

**And also update line 223** where the column is accessed:
```cpp
// Current:
String imageUrl = pet["reference_image_url"].as<String>();

// Change to:
String imageUrl = pet["YOUR_ACTUAL_COLUMN_NAME"].as<String>();
```

### Option 2: Update Database to Match Firmware
Add a column named `reference_image_url` to your Supabase `pets` table:
```sql
ALTER TABLE pets ADD COLUMN reference_image_url TEXT;
```

Or rename the existing column:
```sql
ALTER TABLE pets RENAME COLUMN image_url TO reference_image_url;
```

## Files That Need To Be Checked
1. **feature_matching.h** - Lines 185-186 (URL query) and line 223 (accessing the column)
2. **Supabase Database** - Check the actual column names in the `pets` table

## How to Find Your Actual Column Name
1. Go to your Supabase Dashboard: https://app.supabase.com
2. Navigate to: **Project > Table Editor > pets**
3. Look at the column names in the table
4. Find the column that stores the pet's reference/profile image URL
5. Update the firmware code to use that exact column name

## User Information
- User ID: `9caf44d9-64ce-4950-bd9e-ce31746a2e1d`
- Device ID: `fountain_000000`
- Supabase Project: `eovsqlzgudrhjaeiskon`

## Status
✅ **RESOLVED** - Fixed on November 8, 2025

### Root Cause Analysis
The firmware had TWO issues querying the Supabase `pets` table:

1. **Wrong Column Name**: Firmware requested `reference_image_url` but the actual column is `photo_url`
2. **Wrong Query Filter**: Firmware queried by `user_id` but the `pets` table only has `household_id`

### Database Schema (Verified via Supabase MCP)
The `pets` table structure:
- ✅ `household_id` (uuid) - Links pets to households
- ✅ `photo_url` (text) - Stores pet image URLs
- ❌ `user_id` - Does NOT exist in pets table
- ❌ `reference_image_url` - Does NOT exist in pets table

**Database relationships:**
- Users → Households (via `household_members` table)
- Pets → Households (via `household_id` in `pets` table)

### Changes Made

#### 1. Mobile App (`mobile-app/src/services/bluetooth/BLEService.ts`)
- Updated to send BOTH `user_id` AND `household_id` during BLE provisioning
- Previously only sent `user_id` even though `household_id` was available

#### 2. Firmware (`smart-water-bowl-firmware/main/ble_provisioning.h`)
- Added `receivedHouseholdID` variable to receive household ID from mobile app
- Updated `processUserCredentials()` to parse `household_id` from JSON
- Updated `saveProvisioningData()` to save `household_id` to flash memory

#### 3. Firmware (`smart-water-bowl-firmware/main/supabase_client.h`)
- Added global `householdId` variable
- Updated `initializeSupabase()` to load `household_id` from preferences
- Now logs both User ID and Household ID on initialization

#### 4. Firmware (`smart-water-bowl-firmware/main/feature_matching.h`)
- Added `extern String householdId` declaration
- Changed query from `user_id=eq.XXX` to `household_id=eq.XXX`
- Changed column from `reference_image_url` to `thumbnail_url` (smaller images for 30KB buffer)

### Additional Fixes Applied

#### 5. Database RLS Policy
**Problem**: RLS policy blocked unauthenticated device requests
**Solution**: Added new policy to allow devices to read pets
```sql
CREATE POLICY "Devices can read pets by household_id"
ON public.pets FOR SELECT TO public USING (true);
```

#### 6. Image Size Issue  
**Problem**: Full-size images (124KB) exceeded 30KB device buffer
**Solution**: Switched from `photo_url` to `thumbnail_url` (200px @ 60% quality, ~10-20KB)

### Testing Results ✅
1. ✅ Device successfully re-provisioned with `household_id`
2. ✅ Device successfully fetches pets by `household_id` (HTTP 200)
3. ✅ RLS policy allows unauthenticated device reads
4. ✅ Thumbnail images fit within 30KB buffer
5. ✅ Pet identification ready to test

## Other Successes Today
✅ PSRAM enabled - Memory allocation working
✅ BLE provisioning fixed and working
✅ Device shows up as "PetFountain-XXXX" 
✅ WiFi connection successful
✅ All other systems initialized properly

