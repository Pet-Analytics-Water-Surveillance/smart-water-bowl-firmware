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
⚠️ **UNRESOLVED** - Waiting for database schema check

## Other Successes Today
✅ PSRAM enabled - Memory allocation working
✅ BLE provisioning fixed and working
✅ Device shows up as "PetFountain-XXXX" 
✅ WiFi connection successful
✅ All other systems initialized properly

