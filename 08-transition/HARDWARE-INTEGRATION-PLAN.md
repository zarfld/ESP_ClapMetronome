# Hardware Integration Plan - Wave 3.8

**Date**: 2025-11-21  
**Phase**: 08-transition (Deployment)  
**Standard**: ISO/IEC/IEEE 12207:2017 (Transition Process)

---

## 🎯 Objective

Integrate MAX9814 microphone and DS3231 RTC with ESP32, resolving the boot mode conflict discovered during initial testing.

---

## ⚠️ Problem Statement

### Observed Issue

**Symptom**: ESP32 refuses to enter boot mode (upload fails) when MAX9814 + DS3231 are connected  
**Discovery Date**: 2025-11-21 (during Wave 3.6 testing)  
**Workaround**: Used fresh, unwired ESP32 for software testing  

### Root Cause Analysis

**Suspected Causes**:
1. **GPIO0 (BOOT) interference**: External component pulling GPIO0 HIGH during boot
2. **EN (RESET) interference**: Capacitive load or pull-up/down conflict
3. **Power draw**: MAX9814 + DS3231 exceeding USB power budget during enumeration
4. **I2C bus conflict**: SDA/SCL lines preventing clean boot sequence

**Critical GPIO Pins**:
- **GPIO0 (BOOT)**: Must be LOW during reset to enter boot mode
- **EN (RESET)**: Active LOW reset, must have clean edges
- **GPIO21 (I2C SDA)**: DS3231 data line
- **GPIO22 (I2C SCL)**: DS3231 clock line
- **GPIO36 (ADC0)**: MAX9814 audio output

---

## 🔍 Diagnostic Strategy

### Step 1: Baseline Test (ESP32 Only)

**Objective**: Confirm clean ESP32 boots and uploads without peripherals

**Procedure**:
1. Disconnect ALL external components
2. Connect ESP32 via USB
3. Upload test firmware: `pio run -e esp32dev -t upload`
4. Verify serial output

**Expected**: Upload successful, serial monitor shows boot messages

---

### Step 2: DS3231 Alone

**Objective**: Isolate I2C RTC impact on boot sequence

**Wiring**:
```
DS3231 → ESP32
VCC    → 3.3V
GND    → GND
SDA    → GPIO21 (with 10kΩ pull-up to 3.3V if not on module)
SCL    → GPIO22 (with 10kΩ pull-up to 3.3V if not on module)
```

**Test Procedure**:
1. Connect DS3231 as above
2. Attempt firmware upload
3. If **fails**: Add 10kΩ series resistors on SDA/SCL
4. If **succeeds**: Run I2C scanner test

**Test Code** (`test/test_ds3231_basic/test_ds3231.cpp`):
```cpp
#include <unity.h>
#include <Wire.h>
#include <RTClib.h>

RTC_DS3231 rtc;

void test_ds3231_init(void) {
    TEST_ASSERT_TRUE_MESSAGE(rtc.begin(), "DS3231 not found on I2C bus");
    Serial.println("✓ DS3231 initialized");
}

void test_ds3231_read_time(void) {
    DateTime now = rtc.now();
    TEST_ASSERT_GREATER_THAN(0, now.year());
    Serial.printf("✓ RTC time: %04d-%02d-%02d %02d:%02d:%02d\n",
        now.year(), now.month(), now.day(),
        now.hour(), now.minute(), now.second());
}

void setup() {
    delay(2000);
    Wire.begin(21, 22);  // SDA=21, SCL=22
    
    UNITY_BEGIN();
    RUN_TEST(test_ds3231_init);
    RUN_TEST(test_ds3231_read_time);
    UNITY_END();
}

void loop() {}
```

**Success Criteria**:
- ✅ Upload successful
- ✅ DS3231 detected on I2C bus
- ✅ Time read successfully

**If Upload Fails**:
- Add 10kΩ series resistor on SDA (between DS3231 and GPIO21)
- Add 10kΩ series resistor on SCL (between DS3231 and GPIO22)
- Reasoning: Limits I2C bus current during boot, prevents GPIO0/EN interference

---

### Step 3: MAX9814 Alone

**Objective**: Isolate ADC microphone impact on boot sequence

**Wiring**:
```
MAX9814 → ESP32
VCC     → 3.3V (via 100µF capacitor to GND)
GND     → GND
OUT     → GPIO36 (ADC0)
GAIN    → GND (40dB gain) or VCC (50dB) or floating (60dB)
AR      → GND (no auto-release) or VCC (auto-release)
```

**Test Procedure**:
1. Connect MAX9814 as above
2. Attempt firmware upload
3. If **fails**: Add 100Ω series resistor on OUT pin
4. If **succeeds**: Run ADC sampling test

**Test Code** (`test/test_max9814_basic/test_max9814.cpp`):
```cpp
#include <unity.h>

const int MIC_PIN = 36;  // ADC0
const int SAMPLE_COUNT = 100;

void test_max9814_adc_read(void) {
    uint32_t sum = 0;
    uint16_t min_val = 4095;
    uint16_t max_val = 0;
    
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        uint16_t sample = analogRead(MIC_PIN);
        sum += sample;
        if (sample < min_val) min_val = sample;
        if (sample > max_val) max_val = sample;
        delayMicroseconds(125);  // 8kHz sampling
    }
    
    uint16_t avg = sum / SAMPLE_COUNT;
    uint16_t range = max_val - min_val;
    
    Serial.printf("✓ ADC: avg=%d, min=%d, max=%d, range=%d\n", 
        avg, min_val, max_val, range);
    
    TEST_ASSERT_GREATER_THAN(0, avg);
    TEST_ASSERT_GREATER_THAN(10, range);  // Should see some variation
}

void test_max9814_noise_floor(void) {
    // Sample in quiet environment
    uint32_t sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += analogRead(MIC_PIN);
        delayMicroseconds(125);
    }
    
    uint16_t avg = sum / 1000;
    Serial.printf("✓ Noise floor: %d ADC units\n", avg);
    
    // Noise floor should be relatively stable (within 10% of ADC range)
    TEST_ASSERT_GREATER_THAN(100, avg);   // Not stuck at zero
    TEST_ASSERT_LESS_THAN(3900, avg);     // Not stuck at max
}

void setup() {
    delay(2000);
    analogReadResolution(12);  // 12-bit ADC (0-4095)
    
    UNITY_BEGIN();
    RUN_TEST(test_max9814_adc_read);
    RUN_TEST(test_max9814_noise_floor);
    UNITY_END();
}

void loop() {}
```

**Success Criteria**:
- ✅ Upload successful
- ✅ ADC reads non-zero values
- ✅ ADC shows variation (not stuck)

**If Upload Fails**:
- Add 100Ω series resistor on OUT (between MAX9814 and GPIO36)
- Reasoning: Limits current draw during boot, prevents power budget issues

---

### Step 4: Combined Test (DS3231 + MAX9814)

**Objective**: Verify both components work together without boot conflict

**Wiring** (with isolation components from Steps 2-3):
```
DS3231:
  VCC → 3.3V
  GND → GND
  SDA → [10kΩ resistor] → GPIO21
  SCL → [10kΩ resistor] → GPIO22

MAX9814:
  VCC → 3.3V (via 100µF capacitor)
  GND → GND
  OUT → [100Ω resistor] → GPIO36
  GAIN → GND (40dB)
```

**Test Procedure**:
1. Connect both components with isolation resistors
2. Attempt firmware upload
3. Run combined test

**Test Code** (`test/test_hardware_integration/test_full_system.cpp`):
```cpp
#include <unity.h>
#include <Wire.h>
#include <RTClib.h>

const int MIC_PIN = 36;
RTC_DS3231 rtc;

void test_both_components_init(void) {
    // Initialize I2C
    Wire.begin(21, 22);
    
    // Test DS3231
    TEST_ASSERT_TRUE_MESSAGE(rtc.begin(), "DS3231 not found");
    Serial.println("✓ DS3231 initialized");
    
    // Test MAX9814 ADC
    analogReadResolution(12);
    uint16_t sample = analogRead(MIC_PIN);
    TEST_ASSERT_GREATER_THAN(0, sample);
    Serial.printf("✓ MAX9814 ADC reading: %d\n", sample);
}

void test_simultaneous_operation(void) {
    // Read time
    DateTime now = rtc.now();
    Serial.printf("RTC: %04d-%02d-%02d %02d:%02d:%02d\n",
        now.year(), now.month(), now.day(),
        now.hour(), now.minute(), now.second());
    
    // Sample audio
    uint32_t sum = 0;
    for (int i = 0; i < 100; i++) {
        sum += analogRead(MIC_PIN);
        delayMicroseconds(125);
    }
    uint16_t avg = sum / 100;
    
    Serial.printf("Audio level: %d ADC units\n", avg);
    
    TEST_ASSERT_GREATER_THAN(2000, now.year());
    TEST_ASSERT_GREATER_THAN(0, avg);
}

void setup() {
    delay(2000);
    
    UNITY_BEGIN();
    RUN_TEST(test_both_components_init);
    RUN_TEST(test_simultaneous_operation);
    UNITY_END();
}

void loop() {}
```

**Success Criteria**:
- ✅ Upload successful with both components
- ✅ DS3231 responds on I2C bus
- ✅ MAX9814 ADC samples correctly
- ✅ No interference between I2C and ADC

---

## 🛠️ Isolation Components Summary

### If Boot Issues Persist

| Component | Pin | Isolation | Purpose |
|-----------|-----|-----------|---------|
| DS3231 | SDA (GPIO21) | 10kΩ series resistor | Limit I2C current during boot |
| DS3231 | SCL (GPIO22) | 10kΩ series resistor | Prevent SCL from affecting GPIO0 |
| MAX9814 | OUT (GPIO36) | 100Ω series resistor | Limit ADC input current |
| MAX9814 | VCC | 100µF capacitor to GND | Decouple power, prevent inrush |

### Schematic

```
ESP32                  DS3231
GPIO21 ----[10kΩ]---- SDA
GPIO22 ----[10kΩ]---- SCL
3.3V ----------------- VCC
GND ------------------ GND

ESP32                  MAX9814
GPIO36 ----[100Ω]---- OUT
3.3V --[100µF]------- VCC
       |
      GND
GND ------------------ GND
```

---

## 📊 Test Execution Checklist

### Pre-Test Preparation

- [ ] Backup current working ESP32 (keep separate for software development)
- [ ] Use dedicated ESP32 for hardware testing
- [ ] Have multimeter ready for voltage checks
- [ ] Have 10kΩ and 100Ω resistors available
- [ ] Have 100µF capacitor available

### Test Sequence

- [ ] **Step 1**: Baseline ESP32 upload (no peripherals)
- [ ] **Step 2**: DS3231 alone
  - [ ] Upload attempt without resistors
  - [ ] If fails: Add 10kΩ series resistors on SDA/SCL
  - [ ] Run I2C scanner test
  - [ ] Verify RTC time reads correctly
- [ ] **Step 3**: MAX9814 alone
  - [ ] Upload attempt without resistor
  - [ ] If fails: Add 100Ω series resistor on OUT
  - [ ] Run ADC sampling test
  - [ ] Verify audio signal present
- [ ] **Step 4**: Both components together
  - [ ] Upload with isolation components from Steps 2-3
  - [ ] Run combined test
  - [ ] Verify no interference

### Success Criteria (All Must Pass)

- [ ] Firmware uploads successfully with all hardware connected
- [ ] DS3231 I2C communication stable
- [ ] MAX9814 ADC samples correctly
- [ ] No boot mode conflicts
- [ ] No watchdog resets during operation

---

## 🚀 Next Steps After Successful Integration

1. **Update main.cpp**:
   - Integrate TimingManager (DS3231)
   - Integrate AudioDetection (MAX9814)
   - Run full application with MQTT telemetry

2. **Performance Validation**:
   - Audio detection latency <20ms
   - BPM calculation accuracy
   - Web server responsiveness
   - MQTT telemetry publishing

3. **Long-Term Stability Test**:
   - Run for 24 hours continuous
   - Monitor for memory leaks
   - Check for watchdog resets
   - Verify MQTT reconnection

---

## 📝 Documentation Updates Needed

After successful integration:
- [ ] Update README.md with hardware wiring diagram
- [ ] Document isolation components (if needed)
- [ ] Add troubleshooting section for boot issues
- [ ] Create BOM (Bill of Materials) with part numbers
- [ ] Update architecture diagram with actual pin assignments

---

## ⚠️ Known Risks

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|------------|
| Boot mode conflict unresolvable | Low | High | Use external I2C isolator chip (PCA9515) |
| Power budget exceeded | Medium | Medium | Use external 3.3V regulator (AMS1117) |
| I2C bus instability | Low | Medium | Add 4.7kΩ pull-ups on SDA/SCL |
| ADC noise from I2C | Low | Low | Add RC filter on ADC input |

---

## 📞 Support Resources

**ESP32 Boot Mode**:
- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf) - Section 2.4 (Strapping Pins)
- GPIO0 must be LOW during boot to enter download mode

**I2C Troubleshooting**:
- [I2C Bus Specification](https://www.nxp.com/docs/en/user-guide/UM10204.pdf)
- Standard pull-up: 4.7kΩ for 100kHz, 2.2kΩ for 400kHz

**MAX9814**:
- [Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX9814.pdf)
- Typical supply current: 3mA @ 3.3V

**DS3231**:
- [Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/DS3231.pdf)
- I2C address: 0x68, typical current: 200µA

---

**Author**: GitHub Copilot (AI Agent)  
**Date**: 2025-11-21  
**Status**: Ready for execution  
**Estimated Time**: 2-3 hours
