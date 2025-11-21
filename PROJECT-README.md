# ESP_ClapMetronome

**Real-time beat detection with RTC timestamps and adaptive gain control**

[![Phase](https://img.shields.io/badge/Phase-05%20Implementation-blue)](05-implementation/)
[![Tests](https://img.shields.io/badge/Tests-403%2F403%20passing-brightgreen)](test/)
[![Hardware](https://img.shields.io/badge/Hardware-DS3231%20%2B%20MAX9814-orange)](docs/HARDWARE-WIRING-GUIDE.md)
[![Standards](https://img.shields.io/badge/Standards-ISO%2FIEC%2FIEEE%2012207-purple)](.github/copilot-instructions.md)

---

## 🎯 Overview

ESP_ClapMetronome is a **TDD-implemented beat detection system** for ESP32 that detects acoustic beats/claps with RTC timestamps. It features:

- **Real-Time Clock** - DS3231 RTC for accurate timestamps
- **Audio Detection** - MAX9814 microphone with adaptive gain control
- **Auto-Gain** - Automatic adjustment (40/50/60dB) based on signal level
- **Adaptive Threshold** - Dynamic threshold to prevent false positives
- **Telemetry** - 500ms updates with beat count, gain level, signal metrics

Built using **Test-Driven Development** following **ISO/IEC/IEEE 12207:2017** standards and **Extreme Programming** practices.

---

## ⚡ Quick Start

### Hardware Setup

**Required Components**:
- ESP32 DevKit
- DS3231 RTC module (with CR2032 battery)
- MAX9814 microphone module
- USB cable, breadboard, jumper wires

**Wiring**:
```
DS3231 RTC:
  VCC → ESP32 3.3V
  GND → ESP32 GND
  SDA → ESP32 GPIO21
  SCL → ESP32 GPIO22

MAX9814 Microphone:
  VCC → ESP32 3.3V (+ 100µF capacitor)
  GND → ESP32 GND
  OUT → ESP32 GPIO36 (ADC1_CH0)
  GAIN → ESP32 GPIO32 (gain control)
  AR → ESP32 GPIO33 (attack/release)
```

📖 **Full wiring guide**: [docs/HARDWARE-WIRING-GUIDE.md](docs/HARDWARE-WIRING-GUIDE.md)

### Software Setup

**1. Install PlatformIO**:
```bash
# Install PlatformIO CLI or use VS Code extension
pip install platformio
```

**2. Clone and Build**:
```bash
git clone https://github.com/zarfld/ESP_ClapMetronome.git
cd ESP_ClapMetronome
pio run -e esp32dev
```

**3. Upload**:
```bash
pio run -e esp32dev --target upload --upload-port COM4  # Windows
pio run -e esp32dev --target upload --upload-port /dev/ttyUSB0  # Linux
```

**4. Monitor**:
```bash
pio device monitor --port COM4 --baud 115200
```

---

## 🧪 Testing

### Run All Tests

**Native Tests** (366 tests):
```bash
cd test
mkdir -p build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Debug
ctest -C Debug --output-on-failure
```

**ESP32 Hardware Tests** (37 tests):
```bash
# Baseline (9 tests - no hardware)
pio test -e esp32dev --filter test_baseline --upload-port COM4

# DS3231 RTC (9 tests)
pio test -e esp32dev --filter test_ds3231 --upload-port COM4

# MAX9814 Microphone (11 tests)
pio test -e esp32dev --filter test_max9814 --upload-port COM4

# Combined Hardware (8 tests)
pio test -e esp32dev --filter test_hardware_combined --upload-port COM4
```

### Test Status

| Test Suite | Tests | Status | Pass Rate |
|------------|-------|--------|-----------|
| **Native Tests** | 366 | ✅ PASS | 100% |
| ESP32 Baseline | 9 | ✅ PASS | 100% |
| DS3231 RTC | 9 | ✅ PASS | 100% |
| MAX9814 Microphone | 11 | ✅ PASS | 100% |
| Combined Hardware | 8 | ✅ PASS | 100% |
| **Total** | **403** | ✅ **PASS** | **100%** |

📊 **Test details**: [05-implementation/WAVE-3.8-FINAL-COMPLETION.md](05-implementation/WAVE-3.8-FINAL-COMPLETION.md)

---

## 🏗️ Architecture

### System Overview

```
┌─────────────────────────────────────────────────────┐
│              ESP_ClapMetronome                       │
├─────────────────────────────────────────────────────┤
│  ┌──────────────┐       ┌─────────────────┐        │
│  │ AudioDetection│◄──────┤ TimingManager   │        │
│  │  (MAX9814)   │       │   (DS3231 RTC)  │        │
│  └──────────────┘       └─────────────────┘        │
│         │                        │                   │
│         ├─ Adaptive Threshold    ├─ RTC Timestamps  │
│         ├─ AGC (40/50/60dB)     └─ Fallback micros()│
│         ├─ Kick-only Filter                         │
│         └─ False Positive Rejection                 │
│                                                       │
│  Callbacks:                                          │
│  • onBeatDetected() → RTC timestamp, amplitude      │
│  • onTelemetry()    → Every 500ms, gain level       │
└─────────────────────────────────────────────────────┘
```

### Component Hierarchy

```
src/
├── main.cpp                  # Application entry point
├── audio/
│   ├── AudioDetection.cpp    # Beat detection engine
│   ├── AudioSampleBuffer.cpp # Circular buffer
│   └── AudioDetectionState.cpp # FSM state machine
├── timing/
│   ├── TimingManager.cpp     # RTC + fallback timing
│   └── TimingManagerState.cpp
├── bpm/
│   └── BPMCalculation.cpp    # BPM tracking
├── output/
│   └── OutputController.cpp  # Output sync
├── mqtt/
│   └── MQTTClient.cpp        # Telemetry publishing
├── config/
│   └── ConfigurationManager.cpp
└── interfaces/
    ├── ITimingProvider.h
    ├── IBeatEventPublisher.h
    └── IAudioTelemetry.h
```

🏛️ **Architecture docs**: [03-architecture/ARCHITECTURE-SUMMARY.md](03-architecture/ARCHITECTURE-SUMMARY.md)

---

## 📊 Features

### Beat Detection

- **Adaptive Threshold**: Dynamic adjustment prevents false positives
- **4-State FSM**: IDLE → RISE → PEAK → FALL
- **Kick-only Filter**: Optional filtering for kick drums only
- **False Positive Rejection**: 3-layer validation (debounce + amplitude + timing)
- **Latency**: <20ms from mic to beat event

### Audio Processing

- **Sample Rate**: 16kHz (62.5µs intervals)
- **Gain Levels**: 40dB (loud), 50dB (moderate), 60dB (quiet)
- **Auto-Gain**: Adaptive adjustment based on signal level
- **Attack/Release**: Fast (1:2000) for beat detection

### Timing

- **RTC Precision**: DS3231 with ±2ppm accuracy
- **Temperature Compensated**: Automatic TCXO compensation
- **Fallback**: Graceful degradation to `micros()` if RTC unavailable
- **Monotonic Timestamps**: Guaranteed increasing timestamps

### Telemetry

- **Update Rate**: 500ms
- **Metrics**: ADC value, min/max, threshold, gain level, beat count, false positives
- **Callbacks**: Event-driven notifications (no polling)

---

## 🎓 Development Standards

This project follows:

- **ISO/IEC/IEEE 12207:2017** - Software lifecycle processes
- **ISO/IEC/IEEE 29148:2018** - Requirements engineering
- **IEEE 1016-2009** - Software design descriptions
- **ISO/IEC/IEEE 42010:2011** - Architecture description
- **IEEE 1012-2016** - Verification and validation

### XP Practices

- ✅ **Test-Driven Development** - 403 tests, 100% pass rate
- ✅ **Continuous Integration** - Automated builds and tests
- ✅ **Simple Design** - YAGNI principle, minimal complexity
- ✅ **Refactoring** - Clean code, no duplication
- ✅ **Collective Ownership** - Shared codebase responsibility

📖 **Standards guide**: [.github/copilot-instructions.md](.github/copilot-instructions.md)

---

## 📚 Documentation

### Phase Documentation

| Phase | Status | Documentation |
|-------|--------|---------------|
| 01 - Stakeholder Requirements | ✅ Complete | [01-stakeholder-requirements/](01-stakeholder-requirements/) |
| 02 - Requirements | ✅ Complete | [02-requirements/](02-requirements/) |
| 03 - Architecture | ✅ Complete | [03-architecture/](03-architecture/) |
| 04 - Design | ✅ Complete | [04-design/](04-design/) |
| 05 - Implementation | ✅ Wave 3.8 Complete | [05-implementation/](05-implementation/) |
| 06 - Integration | 🔄 In Progress | [06-integration/](06-integration/) |
| 07 - Verification/Validation | ⏳ Planned | [07-verification-validation/](07-verification-validation/) |
| 08 - Transition | ⏳ Planned | [08-transition/](08-transition/) |
| 09 - Operation/Maintenance | ⏳ Planned | [09-operation-maintenance/](09-operation-maintenance/) |

### Key Documents

- **Hardware Wiring**: [docs/HARDWARE-WIRING-GUIDE.md](docs/HARDWARE-WIRING-GUIDE.md)
- **Wave 3.8 Summary**: [05-implementation/WAVE-3.8-FINAL-COMPLETION.md](05-implementation/WAVE-3.8-FINAL-COMPLETION.md)
- **System Integration**: [05-implementation/WAVE-3.8-SYSTEM-INTEGRATION-COMPLETE.md](05-implementation/WAVE-3.8-SYSTEM-INTEGRATION-COMPLETE.md)
- **Architecture Decisions**: [03-architecture/decisions/](03-architecture/decisions/)
- **Design Specifications**: [04-design/components/](04-design/components/)

---

## 🛠️ Hardware Specifications

### ESP32

- **Model**: ESP32-D0WD-V3
- **CPU**: Dual-core Xtensa LX6 @ 240MHz
- **RAM**: 320KB
- **Flash**: 4MB
- **ADC**: 12-bit, 18 channels
- **I2C**: 2 buses

### DS3231 RTC

- **Accuracy**: ±2ppm (±1 minute/year)
- **Temperature Range**: -40°C to +85°C
- **Interface**: I2C (0x68)
- **Power**: 3.3V, ~0.1mA
- **Battery**: CR2032 (10+ years backup)

### MAX9814 Microphone

- **Gain**: 40/50/60dB (software selectable)
- **Output**: Analog (centered ~VCC/2)
- **Attack/Release**: 1:2000 or 1:4000 (software selectable)
- **Interface**: ADC + 2 GPIO control pins
- **Power**: 3.3V, ~3mA

**Total Power**: ~163mA (ESP32 + peripherals)

---

## 🚀 Roadmap

### ✅ Completed

- [x] Phase 01-04: Requirements, architecture, design
- [x] Wave 1: Audio detection (TDD cycles 1-14)
- [x] Wave 2: BPM tracking and timing
- [x] Wave 3.8: Hardware integration (DS3231 + MAX9814)
- [x] Auto-gain algorithm
- [x] RTC timestamp integration

### 🔄 In Progress

- [ ] Long-term stability testing (24-hour run)
- [ ] BPM calculation from RTC timestamps

### ⏳ Planned

- [ ] MQTT telemetry publishing
- [ ] Web interface for configuration
- [ ] NTP time synchronization
- [ ] Manual gain control UI
- [ ] Power optimization
- [ ] Production deployment guide

---

## 🤝 Contributing

This project follows strict **IEEE/ISO/IEC standards** and **XP practices**:

1. **Create GitHub Issue** - Use appropriate template (requirement/design/test)
2. **Write Tests First** - TDD: Red → Green → Refactor
3. **Follow Standards** - Check `.github/instructions/` for phase-specific rules
4. **Maintain Traceability** - Link all code to requirements/issues
5. **Update Documentation** - Keep architecture and design docs current

📖 **Contribution guide**: [CONTRIBUTING.md](CONTRIBUTING.md) (coming soon)

---

## 📄 License

This project is licensed under the **MIT License** - see [LICENSE](LICENSE) for details.

---

## 🙏 Acknowledgments

- **RTClib** by Adafruit - DS3231 driver
- **PlatformIO** - Build system and testing framework
- **Unity** - C unit testing framework
- **IEEE/ISO/IEC** - Software engineering standards
- **Extreme Programming** - Agile practices

---

## 📞 Contact

- **Repository**: https://github.com/zarfld/ESP_ClapMetronome
- **Issues**: https://github.com/zarfld/ESP_ClapMetronome/issues
- **Documentation**: [Project Wiki](https://github.com/zarfld/ESP_ClapMetronome/wiki)

---

**Status**: 🚀 **Wave 3.8 Complete** - Production-ready hardware integration  
**Last Updated**: 2025-11-21  
**Version**: 0.3.8-alpha
