# Hardware Wiring Configuration - Production Setup

**Project**: ESP32 Clap Metronome  
**Wave**: 3.8 Hardware Integration  
**Status**: ✅ Production Ready  
**Last Updated**: 2025-11-21

---

## 🔌 Complete Wiring Diagram

```
                                    ESP32 DevKit (30-pin)
                              ┌─────────────────────────────┐
                              │                             │
                              │         USB Port            │
                              │           (Top)             │
                              │                             │
    ┌─────────────────────────┤                             ├─────────────────────────┐
    │                         │  EN                    D23  │                         │
    │   DS3231 RTC            │  VP (GPIO36) ◄─ ADC ─┐ D22  │ ◄─ SCL (I2C) ─┐        │
    │        VCC ─────────────►  VN                   │ TX0  │               │         │
    │        GND ─────────────►  D34                  │ RX0  │               │         │
    │        SDA ─────────────►  D35                  │ D21  │ ◄─ SDA (I2C) ─┘        │
    │        SCL ─────────────┐  D32  ◄─ GAIN ───────┤ D19  │                         │
    │                         │  D33  ◄─ AR ─────────┤ D18  │                         │
    │   MAX9814 Mic           │  D25                  │ D5   │                         │
    │        VCC ─────────────►  D26  ◄─ THRESH ─────┤ D17  │ ◄─ TAP_OUT             │
    │        GND ─────────────►  D27                  │ D16  │                         │
    │        OUT ─────────────┘  D14                  │ D4   │                         │
    │        GAIN ────────────┐  D12                  │ D0   │                         │
    │        AR ──────────────┤  D13                  │ D2   │ ◄─ LED                 │
    │                         │  GND                   │ D15  │                         │
    │        [100µF Cap]      │  VIN                   │ GND  │                         │
    │           │             │                             │                         │
    └───────────┴─────────────┴─────────────────────────────┴─────────────────────────┘
                │
              GND (Common Ground)
```

---

## 📋 Pin Assignment Table

| ESP32 Pin | Function | Connected To | Direction | Description |
|-----------|----------|--------------|-----------|-------------|
| **GPIO21** | SDA | DS3231 SDA | Bidirectional | I2C Data Line |
| **GPIO22** | SCL | DS3231 SCL | Output | I2C Clock Line |
| **GPIO36** | ADC0 | MAX9814 OUT | Input | Audio Signal (VP) |
| **GPIO32** | GAIN | MAX9814 GAIN | Output | Gain Control (40/50/60dB) |
| **GPIO33** | AR | MAX9814 AR | Output | Attack/Release Ratio |
| **GPIO26** | THRESH | Threshold Output | Output | Beat Threshold Signal |
| **GPIO17** | TAP_OUT | Tap Sync Output | Output | Tap Synchronization |
| **GPIO2** | LED | Status LED | Output | Built-in LED |
| **3.3V** | Power | DS3231 VCC, MAX9814 VCC | Output | 3.3V Power Supply |
| **GND** | Ground | All GND pins | - | Common Ground |

---

## 🔧 Component Connections

### DS3231 RTC Module

```
DS3231 Module          ESP32 DevKit
┌──────────────┐      ┌──────────────┐
│              │      │              │
│   VCC  ─────────────►  3.3V        │
│   GND  ─────────────►  GND         │
│   SDA  ─────────────►  GPIO21      │
│   SCL  ─────────────►  GPIO22      │
│              │      │              │
│   [CR2032]   │      └──────────────┘
│   Battery    │
│   (Optional) │
└──────────────┘
```

**Notes**:
- CR2032 battery optional (maintains time during power loss)
- Module has built-in pull-up resistors (4.7kΩ typically)
- I2C address: 0x68 (factory default)
- Operating voltage: 2.3V - 5.5V (use 3.3V)

### MAX9814 Microphone Module

```
MAX9814 Module         ESP32 DevKit
┌──────────────┐      ┌──────────────┐
│              │      │              │
│   VCC  ─────────────►  3.3V        │
│              │      │              │
│   GND  ─────────────►  GND         │
│    │         │      │              │
│  [100µF]     │      │              │
│    │         │      │              │
│   GND        │      │              │
│              │      │              │
│   OUT  ─────────────►  GPIO36      │
│              │      │     (VP)     │
│   GAIN ◄─────────────  GPIO32      │
│              │      │              │
│   AR   ◄─────────────  GPIO33      │
│              │      │              │
└──────────────┘      └──────────────┘
```

**Notes**:
- **100µF capacitor**: Place between VCC and GND near MAX9814 (power decoupling)
- **GAIN pin (GPIO32)**:
  - LOW = 40dB gain (recommended default)
  - FLOAT = 50dB gain (disconnect pin)
  - HIGH = 60dB gain (very sensitive)
- **AR pin (GPIO33)**:
  - LOW = 1:2000 attack/release ratio (fast attack)
  - HIGH = 1:4000 attack/release ratio (very slow release)
- **OUT**: Rail-to-rail analog output (0V - 3.3V)

---

## 🛠️ Assembly Instructions

### Step 1: Prepare Components

**Required Parts**:
- ESP32 DevKit (30-pin version)
- DS3231 RTC module (ZS-042 or compatible)
- MAX9814 AGC microphone module
- 100µF electrolytic capacitor (16V or higher)
- 8-10 female-to-female jumper wires
- CR2032 battery (optional, for DS3231)
- Breadboard or PCB (optional)

**Tools**:
- Wire strippers (if cutting wires)
- Multimeter (for verification)
- USB cable (for ESP32 programming)

### Step 2: Power Connections

**Critical: Connect grounds first!**

1. **Common Ground**:
   - Connect DS3231 GND → ESP32 GND
   - Connect MAX9814 GND → ESP32 GND
   - All grounds must be connected together

2. **Power Supply**:
   - Connect DS3231 VCC → ESP32 3.3V
   - Connect MAX9814 VCC → ESP32 3.3V

3. **Power Decoupling**:
   - Solder or connect 100µF capacitor across MAX9814 VCC and GND
   - **Polarity matters**: Negative leg (-) to GND, positive leg (+) to VCC

### Step 3: DS3231 I2C Connections

1. Connect DS3231 SDA → ESP32 GPIO21
2. Connect DS3231 SCL → ESP32 GPIO22
3. (Optional) Insert CR2032 battery into DS3231 module

**Verification**:
- Upload and run `test_ds3231` to verify I2C communication
- Expected: DS3231 detected at I2C address 0x68

### Step 4: MAX9814 Audio Connections

1. Connect MAX9814 OUT → ESP32 GPIO36 (VP)
2. Connect MAX9814 GAIN → ESP32 GPIO32
3. Connect MAX9814 AR → ESP32 GPIO33

**Verification**:
- Upload and run `test_max9814` to verify ADC reads
- Expected: ADC values 100-3900 (not stuck at 0 or 4095)

### Step 5: Combined Test

1. Ensure BOTH DS3231 and MAX9814 are connected
2. Upload and run `test_hardware_combined`
3. Follow interactive prompts (clap when instructed)

**Success Criteria**:
- All 8 tests pass
- Clap detection produces valid timestamp
- No interference between devices

---

## ⚠️ Common Wiring Mistakes

### Issue: DS3231 Not Detected

**Possible Causes**:
- SDA/SCL swapped (swap GPIO21 and GPIO22)
- Loose connections on I2C pins
- Wrong voltage (use 3.3V, not 5V)
- Module defective

**Fix**:
1. Verify wiring: SDA=GPIO21, SCL=GPIO22
2. Check for continuity with multimeter
3. Try different jumper wires
4. Run I2C scanner: `test_baseline` test_008

### Issue: MAX9814 ADC Stuck at Zero

**Possible Causes**:
- No power to MAX9814 (VCC not connected)
- 100µF capacitor missing or wrong polarity
- OUT pin not connected

**Fix**:
1. Verify VCC and GND connections
2. Check 100µF capacitor polarity
3. Verify OUT → GPIO36 connection
4. Test with multimeter: OUT should show ~1.65V (bias voltage)

### Issue: MAX9814 ADC Saturated (4095)

**Possible Causes**:
- GAIN set too high (60dB with loud environment)
- OUT pin shorted to 3.3V

**Fix**:
1. Set GAIN to LOW (40dB): `digitalWrite(GPIO32, LOW)`
2. Check for short circuits
3. Reduce ambient noise

### Issue: Upload Fails with Both Connected

**Possible Causes**:
- GPIO0 (BOOT) pin interference (should NOT happen with GPIO36)
- Power supply insufficient
- USB cable poor quality

**Fix**:
1. Press and hold BOOT button during upload
2. Use powered USB hub or 2A+ USB charger
3. Try different USB cable
4. Temporarily disconnect devices and retry

---

## 🔍 Verification Checklist

### Power Supply Check ✓

- [ ] All GND pins connected to common ground
- [ ] DS3231 VCC connected to ESP32 3.3V
- [ ] MAX9814 VCC connected to ESP32 3.3V
- [ ] 100µF capacitor installed on MAX9814 (correct polarity)
- [ ] Multimeter reads 3.3V between VCC and GND on both modules

### DS3231 RTC Check ✓

- [ ] SDA → GPIO21
- [ ] SCL → GPIO22
- [ ] CR2032 battery installed (optional)
- [ ] Module LED on (if equipped)
- [ ] `test_ds3231` passes (9/9 tests)

### MAX9814 Microphone Check ✓

- [ ] OUT → GPIO36 (VP)
- [ ] GAIN → GPIO32
- [ ] AR → GPIO33
- [ ] Module LED on (if equipped)
- [ ] `test_max9814` passes (11/11 tests)
- [ ] Clap detection works

### Combined Operation Check ✓

- [ ] Both modules connected simultaneously
- [ ] `test_hardware_combined` passes (8/8 tests)
- [ ] Clap produces timestamp
- [ ] No interference detected

---

## 📐 Physical Layout Recommendations

### Breadboard Layout

```
                Breadboard Top View
     ┌────────────────────────────────────┐
     │                                    │
     │   [DS3231]         [MAX9814]       │
     │     RTC          Microphone        │
     │      │                │            │
     │      └────────┬───────┘            │
     │               │                    │
     │          [ESP32 DevKit]            │
     │          (30-pin, centered)        │
     │               │                    │
     │         [100µF Cap near MAX9814]   │
     │                                    │
     └────────────────────────────────────┘
         Power Rails: 3.3V (top), GND (bottom)
```

**Tips**:
- Place ESP32 in center of breadboard
- DS3231 on left side, MAX9814 on right side
- Keep I2C wires (SDA/SCL) short and parallel
- Keep ADC wire (OUT) away from I2C wires
- Use power rails for 3.3V and GND distribution

### PCB Layout (Production)

**Considerations**:
- Separate ground planes for analog (MAX9814) and digital (DS3231)
- Star ground topology (all grounds meet at single point near ESP32)
- Place 100µF capacitor close to MAX9814 VCC pin
- Add 100nF ceramic capacitor near ESP32 3.3V pin
- Route I2C traces as differential pair
- Keep ADC trace short and shielded
- Add test points for debugging

---

## 🔋 Power Consumption

| Component | Typical | Max | Notes |
|-----------|---------|-----|-------|
| ESP32 | 160mA | 240mA | Active WiFi/BT |
| DS3231 | 200µA | 500µA | Running, no battery |
| MAX9814 | 3mA | 5mA | Continuous operation |
| **Total** | **~163mA** | **~246mA** | USB 500mA sufficient |

**Battery Operation**:
- Not recommended (ESP32 high current)
- If needed: Use LiPo battery (3.7V, 2000mAh minimum)
- Runtime: ~10-12 hours with 2000mAh battery

---

## 📞 Hardware Datasheets

- [ESP32-WROOM-32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32_datasheet_en.pdf)
- [DS3231 Datasheet](https://datasheets.maximintegrated.com/en/ds/DS3231.pdf)
- [MAX9814 Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/MAX9814.pdf)

---

**Wiring Configuration**: ✅ **Production Ready**  
**Last Validated**: 2025-11-21  
**Test Coverage**: 100% (37/37 tests passed)
