# DS3231 RTC Test

**Wave**: 3.8 (Hardware Integration)  
**Step**: 2 - DS3231 Alone  
**Purpose**: Test DS3231 RTC module alone to isolate I2C impact on boot sequence

---

## 🎯 Objective

Verify DS3231 Real-Time Clock communicates correctly via I2C and **does NOT interfere with boot mode**.

---

## 🔌 Hardware Setup

**REQUIRED**:
- ESP32 DevKit
- DS3231 RTC module (ZS-042 or similar)
- 4 jumper wires
- USB cable

**Wiring** (NO isolation resistors initially):
```
DS3231 → ESP32
VCC    → 3.3V
GND    → GND
SDA    → GPIO21
SCL    → GPIO22
```

**IMPORTANT**: 
- Disconnect MAX9814 microphone
- Only DS3231 connected during this test

---

## 📋 Test Cases

| Test ID | Description | Expected Result |
|---------|-------------|-----------------|
| ds3231_001 | Initialization | DS3231 detected at 0x68 |
| ds3231_002 | I2C bus scan | Only 1 device found |
| ds3231_003 | Read time | Valid date/time read |
| ds3231_004 | Temperature | Sensor reads reasonable temp |
| ds3231_005 | Power loss check | Battery backup status |
| ds3231_006 | Write time | Time increments correctly |
| ds3231_007 | Stability | 10 consecutive reads work |
| ds3231_008 | Boot mode | Firmware uploads with DS3231 |

---

## 🚀 Run Test

**Step 1**: Wire DS3231 to ESP32 as shown above

**Step 2**: Run test:
```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
& $pio test -e esp32dev --filter test_ds3231 --upload-port COM4
```

---

## ✅ Success Criteria

- All 9 tests PASS
- Firmware uploads successfully WITH DS3231 connected
- DS3231 detected at I2C address 0x68
- Time reads correctly
- I2C communication stable (10/10 reads)

---

## ❌ If Upload Fails (Boot Mode Conflict)

**Symptom**: `Failed to connect to ESP32: Timed out waiting for packet header`

**Solution**: Add 10kΩ series resistors on I2C lines:

```
ESP32                  DS3231
GPIO21 ----[10kΩ]---- SDA
GPIO22 ----[10kΩ]---- SCL
3.3V ----------------- VCC
GND ------------------ GND
```

**Why?**: Limits I2C bus current during boot, prevents SCL/SDA from pulling GPIO0 (BOOT pin) HIGH/LOW at wrong time.

**Retry upload** with resistors in place.

---

## 📊 Expected Output

```
========================================
DS3231 RTC Test - I2C Communication
Wave 3.8: Hardware Integration - Step 2
========================================

⚠️  HARDWARE SETUP:
   DS3231 VCC  → ESP32 3.3V
   DS3231 GND  → ESP32 GND
   DS3231 SDA  → ESP32 GPIO21
   DS3231 SCL  → ESP32 GPIO22
   (No isolation resistors yet)

=== DS3231 Test 001: Initialization ===
✓ DS3231 initialized successfully

=== DS3231 Test 002: I2C Bus Scan ===
Scanning I2C bus (0x01-0x7F)...
  Device found at 0x68 ← DS3231 RTC
Found 1 I2C device(s)
✓ DS3231 detected at correct I2C address

=== DS3231 Test 003: Read Time ===
RTC Time: 2025-11-21 14:23:45
✓ Time read successfully from DS3231

=== DS3231 Test 004: Temperature Sensor ===
DS3231 Temperature: 22.50 °C
✓ Temperature sensor functional

=== DS3231 Test 005: Power Loss Check ===
✓ RTC has valid backup power

=== DS3231 Test 006: Write Time ===
Before: 2025-11-21 14:23:45
After:  2025-11-21 14:23:47
Elapsed: 2 seconds
✓ RTC time increments correctly

=== DS3231 Test 007: Communication Stability ===
Reading time 10 times...
Successful reads: 10/10
✓ I2C communication stable

=== DS3231 Test 008: Boot Mode Compatibility ===
✓ Firmware uploaded successfully with DS3231 connected!
  No boot mode conflict detected
  I2C wiring does NOT interfere with GPIO0 (BOOT)

=== DS3231 Test Summary ===
✓ DS3231 initialized successfully
✓ I2C address 0x68 detected
✓ Time read from RTC
✓ Temperature sensor working
✓ Power backup checked
✓ Time write/read functional
✓ I2C communication stable
✓ Boot mode compatible

>>> DS3231 TEST PASSED <<<
>>> Ready to test MAX9814 microphone <<<

9 Tests 0 Failures 0 Ignored
OK
```

---

## 🔍 Troubleshooting

### DS3231 Not Found (0x68)

**Symptom**: Test 001 fails - "DS3231 not found on I2C bus"

**Solutions**:
1. Check wiring (VCC, GND, SDA, SCL)
2. Verify DS3231 module has pull-up resistors (most modules include 4.7kΩ on SDA/SCL)
3. Try different I2C speed: `Wire.setClock(50000);` in test code
4. Check DS3231 module LED (should be on if powered)

### Time Invalid (Year < 2020)

**Symptom**: Test 003 fails - "RTC year too old"

**Solutions**:
1. Battery dead or missing on DS3231 module
2. First boot - RTC will auto-set to compile time
3. Manually set time: `rtc.adjust(DateTime(2025, 11, 21, 14, 30, 0));`

### Upload Still Fails

**Symptom**: Can't upload even with 10kΩ resistors

**Solutions**:
1. **Manual boot mode**: Hold BOOT, press/release EN, release BOOT
2. Increase resistor value: Try 22kΩ instead of 10kΩ
3. Check for shorts on breadboard
4. Use I2C isolator chip (PCA9515)

---

## 📝 Next Steps

After **DS3231 TEST PASSED**:

1. ✅ Mark Step 2 complete
2. ➡️ Proceed to **Step 3: MAX9814 Microphone Alone**
3. Document if isolation resistors were needed

---

## 📞 DS3231 References

- [DS3231 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)
- I2C Address: 0x68 (fixed, not configurable)
- Operating Voltage: 2.3V - 5.5V (use 3.3V with ESP32)
- Typical Current: 200µA active, 1µA backup
- Temperature Range: -40°C to +85°C
- Accuracy: ±2ppm (±1 minute per year)
