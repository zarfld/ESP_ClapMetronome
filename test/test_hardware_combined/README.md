# Hardware Combined Test - DS3231 + MAX9814

**Wave**: 3.8 (Hardware Integration)  
**Step**: 4 - Both Components Together  
**Purpose**: Verify DS3231 RTC and MAX9814 microphone work together without interference

---

## 🎯 Objective

Test DS3231 and MAX9814 simultaneously to ensure:
- Both devices initialize without conflict
- I2C reads work during ADC sampling
- ADC sampling works during I2C activity
- No electrical interference or cross-talk
- Combined functionality (clap detection with timestamp)

---

## 🔌 Hardware Setup

**REQUIRED**:
- ESP32 DevKit
- DS3231 RTC module
- MAX9814 AGC microphone module
- 100µF electrolytic capacitor (for MAX9814 power decoupling)
- 6-8 jumper wires
- USB cable

**Wiring**:

```
DS3231 → ESP32
VCC    → 3.3V
GND    → GND
SDA    → GPIO21
SCL    → GPIO22

MAX9814 → ESP32
VCC     → 3.3V (+ 100µF cap to GND)
GND     → GND
OUT     → GPIO36 (ADC0)
GAIN    → GPIO32 (software control)
AR      → GPIO33 (attack/release)
```

**IMPORTANT**: 
- **BOTH** devices must be connected for this test
- Pin assignments finalized (no conflicts):
  - GPIO21/22: DS3231 I2C (SDA/SCL)
  - GPIO36: MAX9814 ADC input
  - GPIO32/33: MAX9814 gain control
- No isolation resistors needed (verified in Steps 2 & 3)

---

## 📋 Test Cases

| Test ID | Description | Expected Result |
|---------|-------------|-----------------|
| combined_001 | Both init | DS3231 and MAX9814 initialize |
| combined_002 | I2C during ADC | RTC reads work while sampling audio |
| combined_003 | ADC during I2C | Audio sampling works during RTC reads |
| combined_004 | No interference | Signal quality maintained for both |
| combined_005 | Clap + timestamp | Audio detection with RTC time |
| combined_006 | Stress test | Heavy load on both devices |
| combined_007 | Boot mode | Firmware uploads with both connected |

---

## 🚀 Run Test

**Step 1**: Wire both DS3231 and MAX9814 as shown above

**Step 2**: Verify connections:
- DS3231 module LED should be on (if equipped)
- MAX9814 module LED should be on (if equipped)
- Check 100µF capacitor on MAX9814 VCC

**Step 3**: Run test:
```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
& $pio test -e esp32dev --filter test_hardware_combined --upload-port COM4
```

**Step 4**: Follow prompts:
- Test 005: **CLAP YOUR HANDS** near microphone when prompted

---

## ✅ Success Criteria

- All 8 tests PASS
- Firmware uploads successfully with BOTH devices connected
- DS3231 I2C reads successful during ADC sampling
- MAX9814 ADC sampling stable during I2C reads
- No significant interference (ADC average shift < 100 units)
- Clap detection produces valid RTC timestamp
- Stress test: < 5% error rate
- Boot mode: No conflicts

---

## 📊 Expected Output

```
========================================
Hardware Combined Test
DS3231 RTC + MAX9814 Microphone
Wave 3.8: Hardware Integration - Step 4
========================================

⚠️  HARDWARE SETUP:
   DS3231:
     VCC → ESP32 3.3V
     GND → ESP32 GND
     SDA → ESP32 GPIO21
     SCL → ESP32 GPIO22

   MAX9814:
     VCC → ESP32 3.3V (+ 100µF cap)
     GND → ESP32 GND
     OUT → ESP32 GPIO36 (ADC0)
     GAIN → ESP32 GPIO32
     AR → ESP32 GPIO33

   BOTH devices must be connected!

=== Combined Test 001: Both Devices Initialize ===
DS3231 RTC: ✓ OK
MAX9814 ADC: 1820 (0x71C)
✓ Both devices initialized successfully

=== Combined Test 002: I2C Stable During ADC Sampling ===
Starting continuous ADC sampling in background...
  Read 1: 2025-11-21 14:32:45 ✓
  Read 2: 2025-11-21 14:32:45 ✓
  Read 3: 2025-11-21 14:32:46 ✓
  ...
  Read 10: 2025-11-21 14:32:48 ✓
ADC samples taken: 500
✓ I2C stable during continuous ADC sampling

=== Combined Test 003: ADC Stable During I2C Reads ===
  Iteration 1: RTC=14:32:48, ADC avg=1822 (min=1805, max=1840)
  Iteration 2: RTC=14:32:48, ADC avg=1818 (min=1800, max=1838)
  ...
  Iteration 10: RTC=14:32:50, ADC avg=1820 (min=1803, max=1842)
✓ ADC stable during continuous I2C reads

=== Combined Test 004: No Cross-Talk or Interference ===
Baseline ADC (minimal I2C activity):
  Avg: 1820, Range: 35 (min=1805, max=1840)

ADC with heavy I2C activity:
  Avg: 1822, Range: 38 (min=1804, max=1842)

Difference: Avg=2, Range=3
Range change: 8.6%
✓ No interference detected between DS3231 and MAX9814

=== Combined Test 005: Clap Detection + Timestamp ===
>>> PLEASE CLAP YOUR HANDS NEAR THE MICROPHONE <<<
Listening for 5 seconds...

  Peak detected:
    Value: 2456 ADC units
    Relative time: 1234 ms
    RTC timestamp: 2025-11-21 14:32:52
✓ Clap detection with RTC timestamp functional

=== Combined Test 006: Simultaneous Operation Stress ===
Running heavy load on both devices for 5 seconds...

Stress Test Results:
  I2C reads: 200
  ADC reads: 2000
  Errors: 0
  Error rate: 0.00%
✓ Both devices operate reliably under stress

=== Combined Test 007: Boot Mode Compatibility ===
✓ Firmware uploaded successfully with BOTH devices connected!
  DS3231 (I2C on GPIO21/22) + MAX9814 (ADC on GPIO36)
  No boot mode conflicts detected
  No isolation resistors needed

=== Hardware Combined Test Summary ===
✓ Both devices initialize
✓ I2C stable during ADC sampling
✓ ADC stable during I2C reads
✓ No interference detected
✓ Clap detection with timestamp
✓ Stress test passed
✓ Boot mode compatible

========================================
>>> HARDWARE INTEGRATION COMPLETE <<<
========================================
DS3231 RTC + MAX9814 Microphone
Both devices work together without issues!

✓ Ready for main application integration
========================================

8 Tests 0 Failures 0 Ignored
OK
```

---

## 🔍 Troubleshooting

### One Device Not Detected

**Symptom**: Test 001 fails - "DS3231 not detected" or "MAX9814 ADC stuck"

**Solutions**:
1. Check power connections (both devices need 3.3V)
2. Verify wiring matches diagram exactly
3. Check I2C address: `pio device monitor` then I2C scan
4. Test each device individually (Steps 2 & 3)

### I2C Errors During ADC Sampling

**Symptom**: Test 002 fails - "I2C reads failed during ADC sampling"

**Solutions**:
1. Check GPIO21/22 connections (should be secure)
2. Add 4.7kΩ pull-up resistors on SDA/SCL (if needed)
3. Reduce ADC sampling rate (increase delay)
4. Check power supply stability (use USB hub with power)

### ADC Unstable During I2C

**Symptom**: Test 003 fails - "ADC stuck at zero/max during I2C"

**Solutions**:
1. Check 100µF capacitor on MAX9814 VCC
2. Verify GPIO36 connection (should be direct)
3. Check ground connections (common ground essential)
4. Try different GPIO for gain control (if interference)

### Interference Detected

**Symptom**: Test 004 fails - "ADC average shifted significantly"

**Solutions**:
1. Check for ground loops (single common ground point)
2. Shorten wires (reduce crosstalk)
3. Add 100nF ceramic capacitor near ESP32 3.3V pin
4. Separate power wiring for DS3231 and MAX9814
5. Keep I2C wires away from ADC input wire

### Clap Not Timestamped

**Symptom**: Test 005 fails - "Invalid RTC timestamp"

**Solutions**:
1. Check DS3231 battery (CR2032 installed?)
2. Verify RTC time was set (run test_ds3231 first)
3. Check I2C communication reliability
4. Reduce I2C clock speed if needed

### High Error Rate Under Stress

**Symptom**: Test 006 fails - "Error rate too high under stress"

**Solutions**:
1. Check power supply (use 2A+ USB charger)
2. Add bulk capacitors (100µF-1000µF) near ESP32
3. Check for loose connections
4. Verify both devices work individually

---

## 📝 Next Steps

After **HARDWARE INTEGRATION COMPLETE**:

1. ✅ Mark Wave 3.8 Step 4 complete
2. ➡️ Document final wiring configuration
3. ➡️ Update main application with pin assignments
4. ➡️ Integrate TimingManager (DS3231)
5. ➡️ Integrate AudioDetection (MAX9814)
6. ➡️ Full system integration test
7. ➡️ Long-term stability test (24-hour run)

---

## 🎉 Success Indicators

**Wave 3.8 Complete When**:
- [x] Step 1: Baseline test (9/9 passed)
- [x] Step 2: DS3231 alone (9/9 passed)
- [x] Step 3: MAX9814 alone (11/11 passed)
- [ ] **Step 4: Combined test (8/8 passed)** ← **YOU ARE HERE**
- [ ] Final wiring documented
- [ ] Main application updated

---

## 📞 Hardware References

**DS3231 RTC**:
- I2C Address: 0x68
- Supply: 2.3V - 5.5V (use 3.3V)
- Current: 200µA typical
- Accuracy: ±2ppm (±1 minute/year)
- Battery: CR2032 (optional, for timekeeping)

**MAX9814 Microphone**:
- Supply: 2.7V - 5.5V (use 3.3V)
- Current: 3mA typical
- Output: Rail-to-rail analog (0V - VCC)
- Gain: 40dB/50dB/60dB selectable
- Frequency: 20Hz - 20kHz

**Pin Assignments (Final)**:
- GPIO21: DS3231 SDA (I2C data)
- GPIO22: DS3231 SCL (I2C clock)
- GPIO36: MAX9814 OUT (ADC input)
- GPIO32: MAX9814 GAIN (software control)
- GPIO33: MAX9814 AR (attack/release)
