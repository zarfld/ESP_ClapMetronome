# Baseline ESP32 Test

**Wave**: 3.8 (Hardware Integration)  
**Step**: 1 - Baseline  
**Purpose**: Establish that ESP32 boots and uploads cleanly without peripherals

---

## 🎯 Objective

Verify ESP32 functions correctly with **NO external components** connected. This establishes a baseline before testing DS3231 and MAX9814.

---

## ⚠️ CRITICAL: Hardware Setup

**REQUIRED**:
- ESP32 DevKit (bare board)
- USB cable (data + power)
- **NO external components**

**DISCONNECT**:
- ❌ DS3231 RTC
- ❌ MAX9814 microphone
- ❌ Any breadboard wiring
- ❌ Any external sensors

**ONLY connect**: ESP32 → USB cable → Computer

---

## 📋 Test Cases

| Test ID | Description | Expected Result |
|---------|-------------|-----------------|
| baseline_001 | Boot success | Firmware uploads and runs |
| baseline_002 | Serial communication | UART0 functional |
| baseline_003 | Chip info | Model, cores, MAC, heap |
| baseline_004 | GPIO state | Critical pins available |
| baseline_005 | Memory health | No leaks, stable heap |
| baseline_006 | Timing accuracy | millis()/micros() working |
| baseline_007 | ADC functional | Analog reads working |
| baseline_008 | I2C bus init | Bus initializes, no devices |

---

## 🚀 Run Test

```powershell
# Upload to ESP32 on COM4
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
& $pio test -e esp32dev --filter test_baseline --upload-port COM4
```

---

## ✅ Success Criteria

- All 9 tests PASS
- Firmware uploads successfully
- Serial output visible
- No watchdog resets
- No boot loops

---

## 🔍 What This Validates

1. **Upload Process**: ESP32 enters boot mode correctly (GPIO0 LOW during reset)
2. **Serial Monitor**: UART0 communication functional
3. **GPIO Availability**: Pins 21, 22, 36 available for peripherals
4. **I2C Bus**: Wire library initializes without errors
5. **ADC**: Analog reads functional (for MAX9814)
6. **Memory**: Heap stable, no leaks
7. **Timing**: Accurate delays (for audio sampling)

---

## 📊 Expected Output

```
========================================
ESP32 Baseline Test - No Peripherals
Wave 3.8: Hardware Integration - Step 1
========================================

⚠️  CRITICAL: Ensure NO external components connected!
   - Disconnect DS3231 RTC
   - Disconnect MAX9814 microphone
   - Only ESP32 + USB cable

=== Baseline Test 001: Boot Success ===
✓ Boot sequence completed

=== Baseline Test 002: Serial Communication ===
Testing serial output...
✓ Serial communication functional

=== Baseline Test 003: Chip Information ===
Chip Model: ESP32 with 2 CPU cores
WiFi/BT/BLE
Silicon Revision: 1
Flash: 4MB external
MAC Address: XX:XX:XX:XX:XX:XX
CPU Frequency: 240 MHz
Free Heap: 295396 bytes
✓ Chip information retrieved

=== Baseline Test 004: GPIO State ===
GPIO0 (BOOT): HIGH
GPIO21 (SDA): Available
GPIO22 (SCL): Available
GPIO36 (ADC0): 1823 ADC units
✓ All critical GPIO pins available

=== Baseline Test 005: Memory Health ===
Initial Free Heap: 295396 bytes
Final Free Heap: 295396 bytes
✓ Memory allocation/deallocation working

=== Baseline Test 006: Timing Accuracy ===
delay(100): Elapsed 100 ms
delayMicroseconds(1000): Elapsed 1000 us
✓ Timing functions accurate

=== Baseline Test 007: ADC Functionality ===
ADC36 (floating): avg=1823, min=1810, max=1835, range=25
✓ ADC reading analog values

=== Baseline Test 008: I2C Bus Initialization ===
I2C bus initialized (SDA=21, SCL=22, 100kHz)
Scanning I2C bus...
Found 0 I2C device(s)
✓ I2C bus functional (no devices as expected)

=== Baseline Test Summary ===
✓ ESP32 boots successfully without peripherals
✓ Serial communication working
✓ Chip information retrieved
✓ GPIO pins available
✓ Memory healthy
✓ Timing accurate
✓ ADC functional
✓ I2C bus initialized

>>> BASELINE TEST PASSED <<<
>>> Ready to test peripherals <<<

9 Tests 0 Failures 0 Ignored
OK
```

---

## ❌ Troubleshooting

### Upload Fails

**Symptom**: `Failed to connect to ESP32: Timed out waiting for packet header`

**Solution**:
1. Hold BOOT button (GPIO0)
2. Press and release EN (RESET) button
3. Release BOOT button
4. Retry upload

### Serial Not Working

**Symptom**: No output in serial monitor

**Solution**:
1. Check baud rate: 115200
2. Check USB cable (must be data cable, not charge-only)
3. Check COM port in Device Manager

### Tests Fail

**Symptom**: Tests fail but upload works

**Solution**:
1. Verify **NO peripherals connected**
2. Check for breadboard shorts
3. Press EN button to reset ESP32
4. Re-run test

---

## 📝 Next Steps

After **BASELINE TEST PASSED**:

1. ✅ Mark Step 1 complete
2. ➡️ Proceed to **Step 2: DS3231 RTC Alone**
3. Document baseline results (heap size, timing accuracy)

---

## 📞 References

- ESP32 Datasheet: [Strapping Pins (Section 2.4)](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- GPIO0 (BOOT): Must be LOW during reset to enter boot mode
- GPIO36 (ADC0): Input-only pin, no internal pull-up/down
