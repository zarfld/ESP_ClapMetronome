# MAX9814 Microphone Test

**Wave**: 3.8 (Hardware Integration)  
**Step**: 3 - MAX9814 Alone  
**Purpose**: Test MAX9814 microphone alone to isolate ADC impact on boot sequence

---

## 🎯 Objective

Verify MAX9814 AGC microphone outputs valid audio signal and **does NOT interfere with boot mode**.

---

## 🔌 Hardware Setup

**REQUIRED**:
- ESP32 DevKit
- MAX9814 AGC microphone module
- 100µF electrolytic capacitor (for power decoupling)
- 3-4 jumper wires
- USB cable

**Wiring** (Direct GPIO control for DS3231 + MAX9814 compatibility):
```
MAX9814 → ESP32
VCC     → 3.3V (place 100µF cap between VCC and GND near MAX9814)
GND     → GND
OUT     → GPIO36 (ADC0)
GAIN    → GPIO26 (software gain control)
AR      → GPIO25 (attack/release control)
```

**GAIN Settings** (controlled via GPIO26):
- GPIO26 **LOW**: 40dB gain (recommended, least clipping)
- GPIO26 **FLOAT**: 50dB gain (medium sensitivity)
- GPIO26 **HIGH**: 60dB gain (maximum, risk of saturation)

**Attack/Release** (controlled via GPIO25):
- GPIO25 **LOW**: 1:2000 ratio (fast attack, slow release)
- GPIO25 **HIGH**: 1:4000 ratio (fast attack, very slow release)

**IMPORTANT**: 
- GPIO21/22 are used by DS3231 I2C bus
- MAX9814 uses GPIO25/26 for control (no conflict)
- DS3231 can remain connected during this test

---

## 📋 Test Cases

| Test ID | Description | Expected Result |
|---------|-------------|-----------------|
| max9814_001 | ADC init | GPIO36 ADC reads values |
| max9814_002 | Read signal | ADC shows varying audio |
| max9814_003 | Noise floor | Background noise measured |
| max9814_004 | Clap detection | Peak detected when clapping |
| max9814_005 | Stability | 10 consecutive reads stable |
| max9814_006 | Sampling rate | 8kHz ±50% accuracy (relaxed) |
| max9814_007 | Gain silent | 40dB/50dB/60dB gain adjustment tested |
| max9814_008 | Gain loud | Saturation prevention with loud input |
| max9814_009 | Auto-gain | Adaptive gain control algorithm |
| max9814_010 | Boot mode | Firmware uploads with MAX9814 |

---

## 🚀 Run Test

**Step 1**: Wire MAX9814 to ESP32 as shown above

**Step 2**: Run test:
```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\platformio.exe"
& $pio test -e esp32dev --filter test_max9814 --upload-port COM4
```

**Step 3**: Follow prompts:
- Test 003: Keep environment **QUIET** for 1 second
- Test 004: **CLAP YOUR HANDS** near microphone when prompted

---

## ✅ Success Criteria

- All 11 tests PASS
- Firmware uploads successfully WITH MAX9814 connected
- ADC reads varying signal (not stuck at 0 or 4095)
- Noise floor: 100-3900 ADC units
- Clap detection: Peak > 500 ADC units
- Sampling rate: 4000-12000 Hz (within 50%, relaxed for ESP32 timing)
- Gain adjustment: 40dB < 50dB < 60dB signal range progression
- Auto-gain algorithm functional for silent and loud inputs

---

## ❌ If Upload Fails (Boot Mode Conflict)

**Symptom**: `Failed to connect to ESP32: Timed out waiting for packet header`

**Solution**: Add 100Ω series resistor on OUT pin:

```
ESP32                      MAX9814
GPIO36 ----[100Ω]-------- OUT
3.3V --[100µF]----------- VCC
       |
      GND
GND ---------------------- GND
```

**Why?**: Limits ADC input current during boot, prevents OUT from affecting GPIO0 (BOOT pin).

**Retry upload** with resistor in place.

---

## 📊 Expected Output

```
========================================
MAX9814 Microphone Test - ADC Audio
Wave 3.8: Hardware Integration - Step 3
========================================

⚠️  HARDWARE SETUP:
   MAX9814 VCC  → ESP32 3.3V (+ 100µF cap to GND)
   MAX9814 GND  → ESP32 GND
   MAX9814 OUT  → ESP32 GPIO36 (ADC0)
   MAX9814 GAIN → GND (40dB)
   (No isolation resistor yet)

⚠️  IMPORTANT: Disconnect DS3231 during this test

=== MAX9814 Test 001: ADC Initialization ===
ADC36 sample: 1823 (0x71F)
✓ ADC initialized on GPIO36

=== MAX9814 Test 002: Read Audio Signal ===
Sampling 100 times at 8000 Hz...
  Average:       1820 ADC units
  Min:           1805 ADC units
  Max:           1842 ADC units
  Range:         37 ADC units
  Peak-to-Peak:  ~74 ADC units
✓ ADC reading varying signal

=== MAX9814 Test 003: Noise Floor ===
Please keep environment QUIET for 1 second...
  Noise Floor Average: 1822 ADC units
  Noise Floor Range:   45 ADC units
  Min: 1798, Max: 1843
✓ Noise floor measured

=== MAX9814 Test 004: Clap Detection ===
>>> PLEASE CLAP YOUR HANDS NEAR THE MICROPHONE <<<
Listening for 3 seconds...
  Peak detected: 2456 ADC units at 1234 ms
✓ Clap detection functional

=== MAX9814 Test 005: Signal Stability ===
Reading 10 batches...
  Batch 1: avg = 1820
  Batch 2: avg = 1818
  Batch 3: avg = 1822
  ...
  Batch 10: avg = 1819
✓ Signal stable over 10 batches

=== MAX9814 Test 006: Sampling Rate ===
  Target:  8000 Hz
  Actual:  7956 Hz
  Error:   0.6%
  Time:    125628 µs for 1000 samples
✓ Sampling rate achievable

=== MAX9814 Test 007: Gain Adjustment (Silent Input) ===
Testing gain adjustment for silent/quiet environment...
  40dB Gain (LOW): avg=1820, range=37 (min=1805, max=1842)
  50dB Gain (FLOAT): avg=1825, range=52 (min=1799, max=1851)
  60dB Gain (HIGH): avg=1830, range=78 (min=1791, max=1869)
  ✓ Gain progression verified: 40dB < 50dB < 60dB
✓ Gain adjustment for silent input functional

=== MAX9814 Test 008: Gain Adjustment (Loud Input) ===
>>> PLEASE CLAP LOUDLY or SPEAK LOUDLY near microphone <<<
  60dB Gain (HIGH): peak=3856, saturated=245 samples
  40dB Gain (LOW): peak=2456, saturated=0 samples
  ⚠️  60dB gain saturates with loud input - use 40dB or 50dB
  ✓ RECOMMENDATION: Start with 40dB gain for music/loud environments
✓ Gain adjustment for loud input functional

=== MAX9814 Test 009: Auto-Gain Adjustment Algorithm ===
Testing adaptive gain control logic...
  Cycle 1: avg=1820, peak=1842 → Action: MAINTAIN GAIN (optimal range)
  Cycle 2: avg=1818, peak=1845 → Action: MAINTAIN GAIN (optimal range)
  Cycle 3: avg=1822, peak=1850 → Action: MAINTAIN GAIN (optimal range)
✓ Auto-gain adjustment algorithm tested

=== MAX9814 Test 010: Boot Mode Compatibility ===
✓ Firmware uploaded successfully with MAX9814 connected!
  No boot mode conflict detected
  ADC wiring does NOT interfere with GPIO0 (BOOT)

=== MAX9814 Test Summary ===
✓ ADC initialized on GPIO36
✓ Audio signal reading
✓ Noise floor measured
✓ Clap detection functional
✓ Signal stable
✓ Sampling rate accurate
✓ Gain adjustment (silent) tested
✓ Gain adjustment (loud) tested
✓ Auto-gain algorithm verified
✓ Boot mode compatible

>>> MAX9814 TEST PASSED <<<
>>> Gain control functional for both silent and loud inputs <<<
>>> Ready to test DS3231 + MAX9814 together <<<

11 Tests 0 Failures 0 Ignored
OK
```

---

## 🔍 Troubleshooting

### ADC Stuck at Zero

**Symptom**: Test 002 fails - "ADC stuck at zero"

**Solutions**:
1. Check VCC connection (should be 3.3V)
2. Add 100µF capacitor between VCC and GND
3. Verify MAX9814 module LED is on (if equipped)
4. Try different MAX9814 module (may be defective)

### ADC Stuck at Max (4095)

**Symptom**: Test 002 fails - "ADC stuck at max"

**Solutions**:
1. Check for short circuit on OUT pin
2. Reduce GAIN (connect to GND for 40dB)
3. Verify GPIO36 is not connected elsewhere

### No Signal Variation

**Symptom**: Test 002 fails - "ADC not varying"

**Solutions**:
1. Make some noise (talk, clap, tap)
2. Check GAIN setting (try VCC for 60dB)
3. Verify OUT is connected to GPIO36
4. Check AUTO_RELEASE setting

### Clap Not Detected

**Symptom**: Test 004 fails - "No significant audio detected"

**Solutions**:
1. Clap LOUDER near microphone
2. Increase GAIN (connect to VCC for 60dB)
3. Check microphone orientation
4. Verify OUT signal with oscilloscope/multimeter

---

## 📝 Next Steps

After **MAX9814 TEST PASSED**:

1. ✅ Mark Step 3 complete
2. ➡️ Proceed to **Step 4: DS3231 + MAX9814 Together**
3. Document if isolation resistor was needed

---

## 📞 MAX9814 References

- [MAX9814 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX9814.pdf)
- Output: Rail-to-rail analog (0V to VCC)
- Supply Voltage: 2.7V - 5.5V (use 3.3V with ESP32)
- Supply Current: 3mA typical
- Gain: 40dB / 50dB / 60dB (pin selectable)
- Attack/Release Time: 1:2000 or 1:4000 (AR pin)
- Frequency Response: 20Hz - 20kHz
