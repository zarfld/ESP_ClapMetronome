# Phase 05: Implementation - Kickoff & TDD Strategy

**Phase**: 05 - Test-Driven Development & Implementation  
**Standard**: ISO/IEC/IEEE 12207:2017 (Implementation Process)  
**XP Practices**: Test-First Programming, Simple Design, Continuous Integration  
**Start Date**: 2025-11-18  
**Status**: 🚀 **INITIATED** - Transitioning from Phase 04 Design  

---

## 🎯 Phase 05 Objectives

1. **Implement all 7 design components** using Red-Green-Refactor TDD cycles
2. **Write tests first** before production code (XP Test-First principle)
3. **Maintain continuous integration** - all tests green before commits
4. **Establish complete traceability**: DES-C-* → SRC → TEST-*
5. **Apply XP Simple Design**: YAGNI, no duplication, clear intent, minimal classes

---

## ❓ Clarification Questions - ANSWERED

### Q1: Which design components should I implement first (dependency order)?

**Answer**: Implementation order follows the **dependency graph** from Phase 04 designs:

#### **Wave 1: Foundation Layer** (No dependencies)
- **DES-C-005: Timing Manager** (#49) - Foundation timing services
  - Priority: **P0 (Critical)** - All components depend on timestamps
  - Interfaces: DES-I-001 (Timestamp Query), DES-I-002 (RTC Health), DES-I-003 (Time Sync)
  - Data Models: DES-D-006 (TimingManagerState)
  - Est. Effort: 8 hours (3 interfaces + hardware abstraction)
  - **START HERE** ✅

#### **Wave 2: Core Detection & Calculation** (Depends on Wave 1)
- **DES-C-001: Audio Detection Engine** (#45)
  - Priority: **P0 (Critical)** - Core beat detection
  - Depends on: DES-C-005 (Timing Manager)
  - Interfaces: DES-I-004 (Beat Event), DES-I-005 (Audio Telemetry)
  - Data Models: DES-D-001 (Audio Buffer), DES-D-002 (Detection State)
  - Est. Effort: 12 hours (complex DSP algorithm + AGC)

- **DES-C-002: BPM Calculation Engine** (#46)
  - Priority: **P0 (Critical)** - BPM from beat intervals
  - Depends on: DES-C-005 (Timing), DES-C-001 (Beat Events)
  - Interfaces: DES-I-006 (BPM Update), DES-I-007 (Tap Addition)
  - Data Models: DES-D-003 (Tap Circular Buffer)
  - Est. Effort: 8 hours (circular buffer + tempo correction)

#### **Wave 3: Configuration & Output** (Depends on Waves 1-2)
- **DES-C-006: Configuration Manager** (#50)
  - Priority: **P1 (High)** - Persistent configuration
  - Depends on: None (foundation)
  - Interfaces: DES-I-008 (Configuration API)
  - Data Models: DES-D-004 (NVS Configuration Schema)
  - Est. Effort: 6 hours (NVS storage + validation)

- **DES-C-004: Output Controller** (#48)
  - Priority: **P1 (High)** - MIDI/Relay outputs
  - Depends on: DES-C-005 (Timing), DES-C-002 (BPM Events)
  - Interfaces: DES-I-013 (Output Trigger)
  - Data Models: DES-D-007 (MIDI Output Packet)
  - Est. Effort: 10 hours (UART MIDI + GPIO relay + synchronization)

#### **Wave 4: Network Services** (Depends on all above)
- **DES-C-003: Web Server & WebSocket** (#47)
  - Priority: **P1 (High)** - User interface
  - Depends on: All components (telemetry aggregator)
  - Interfaces: DES-I-011 (WebSocket Protocol), DES-I-012 (REST API)
  - Data Models: DES-D-005 (WebSocket JSON Telemetry)
  - Est. Effort: 14 hours (async server + WebSocket + REST + HTML UI)

- **DES-C-007: MQTT Telemetry Client** (#44)
  - Priority: **P2 (Medium)** - Optional telemetry
  - Depends on: All components (telemetry publisher)
  - Interfaces: DES-I-009 (MQTT Publish)
  - Est. Effort: 6 hours (PubSubClient + topic management)

**Total Estimated Effort**: ~64 hours (8 working days @ 8 hours/day)

---

### Q2: What testing framework and project structure do you recommend?

**Answer**: Use **PlatformIO native testing** with following structure:

#### **Testing Framework Stack**
```
Testing Framework: Unity (PlatformIO Native)
├─ Unit Tests: GoogleTest-style assertions (TEST, ASSERT_EQ, etc.)
├─ Mocking: Manual mocks (DES-I-* mock implementations from TDD Plan #51)
├─ Integration Tests: PlatformIO native test runner
├─ Hardware Tests: Manual validation (oscilloscope, protocol analyzer)
└─ CI/CD: GitHub Actions (automated test execution)
```

**Why PlatformIO Unity?**
- ✅ Native ESP32 support (runs on actual hardware)
- ✅ Cross-platform (desktop emulation for unit tests)
- ✅ Simple setup (no external dependencies)
- ✅ Fast execution (<5s for 74 unit tests)

#### **Project Structure** (Already established in platformio.ini)

```
ESP_ClapMetronome/
├── platformio.ini           ← Build configuration (ESP32 + ESP8266)
├── src/                     ← Production code (*.cpp, *.h)
│   ├── main.cpp            ← Entry point (setup() + loop())
│   ├── timing/             ← DES-C-005: Timing Manager
│   │   ├── TimingManager.h
│   │   ├── TimingManager.cpp
│   │   └── TimingManagerState.h (DES-D-006)
│   ├── audio/              ← DES-C-001: Audio Detection Engine
│   │   ├── AudioDetection.h
│   │   ├── AudioDetection.cpp
│   │   ├── AudioSampleBuffer.h (DES-D-001)
│   │   └── AudioDetectionState.h (DES-D-002)
│   ├── bpm/                ← DES-C-002: BPM Calculation Engine
│   │   ├── BPMCalculation.h
│   │   ├── BPMCalculation.cpp
│   │   └── TapCircularBuffer.h (DES-D-003)
│   ├── config/             ← DES-C-006: Configuration Manager
│   │   ├── ConfigManager.h
│   │   ├── ConfigManager.cpp
│   │   └── NVSSchema.h (DES-D-004)
│   ├── output/             ← DES-C-004: Output Controller
│   │   ├── OutputController.h
│   │   ├── OutputController.cpp
│   │   └── MIDIPacket.h (DES-D-007)
│   ├── web/                ← DES-C-003: Web Server & WebSocket
│   │   ├── WebServer.h
│   │   ├── WebServer.cpp
│   │   └── WebSocketJSON.h (DES-D-005)
│   ├── mqtt/               ← DES-C-007: MQTT Telemetry Client
│   │   ├── MQTTClient.h
│   │   └── MQTTClient.cpp
│   └── interfaces/         ← Shared interface definitions
│       ├── IBeatEventListener.h (DES-I-004)
│       ├── IBPMUpdateListener.h (DES-I-006)
│       └── ITelemetryProvider.h
│
├── test/                    ← Test code (*.cpp)
│   ├── test_timing/        ← Unit tests for DES-C-005
│   │   ├── test_timestamp_query.cpp (DES-I-001)
│   │   ├── test_rtc_health.cpp (DES-I-002)
│   │   ├── test_time_sync.cpp (DES-I-003)
│   │   └── test_timing_state.cpp (DES-D-006)
│   ├── test_audio/         ← Unit tests for DES-C-001
│   │   ├── test_beat_detection.cpp
│   │   ├── test_audio_buffer.cpp (DES-D-001)
│   │   ├── test_audio_state.cpp (DES-D-002)
│   │   └── test_agc.cpp
│   ├── test_bpm/           ← Unit tests for DES-C-002
│   │   ├── test_bpm_calculation.cpp
│   │   ├── test_tap_buffer.cpp (DES-D-003)
│   │   └── test_tempo_correction.cpp
│   ├── test_config/        ← Unit tests for DES-C-006
│   │   ├── test_nvs_storage.cpp (DES-D-004)
│   │   └── test_factory_reset.cpp
│   ├── test_output/        ← Unit tests for DES-C-004
│   │   ├── test_midi_output.cpp (DES-D-007)
│   │   └── test_relay_output.cpp
│   ├── test_web/           ← Unit tests for DES-C-003
│   │   ├── test_websocket_json.cpp (DES-D-005)
│   │   └── test_rest_api.cpp
│   ├── test_mqtt/          ← Unit tests for DES-C-007
│   │   └── test_mqtt_publish.cpp
│   ├── test_integration/   ← Integration tests (26 scenarios)
│   │   ├── test_audio_to_bpm.cpp (INT-001)
│   │   ├── test_bpm_to_output.cpp (INT-002)
│   │   └── ...
│   └── mocks/              ← Mock implementations (from TDD Plan #51)
│       ├── MockTimingManager.h
│       ├── MockAudioDetection.h
│       ├── MockBPMCalculation.h
│       └── ...
│
├── 04-design/              ← Design specifications (reference)
│   ├── tdd-plan-phase-05.md (#51)
│   └── ...
│
└── 05-implementation/      ← Implementation docs
    ├── PHASE-05-KICKOFF.md (this file)
    ├── implementation-log.md (daily progress)
    └── traceability-matrix.md (DES → SRC → TEST)
```

#### **PlatformIO Configuration Updates Needed**

Add to `platformio.ini`:

```ini
[env:native]
; Desktop testing environment (unit tests only, no hardware)
platform = native
test_framework = unity
test_build_src = yes
build_flags = 
    -D UNIT_TEST
    -std=gnu++17
    -Wall
    -Wextra
lib_deps = 
    google/googletest @ ^1.14.0

[env:esp32dev]
; Existing ESP32 config + test support
test_framework = unity
test_build_src = yes
build_flags = 
    ${env:esp32dev.build_flags}
    -std=gnu++17
```

---

### Q3: Are there specific performance or quality constraints for the implementation?

**Answer**: YES - Enforce constraints from REQ-NF-* and QA scenarios:

#### **Performance Constraints** (from REQ-NF-001 #9, QA-SC-001 #27)

| Metric | Target | Max | Validation Method |
|--------|--------|-----|-------------------|
| **Audio Detection Latency** | <15ms | <20ms | Performance test PERF-001 |
| **BPM Calculation Latency** | <3ms | <5ms | Performance test PERF-002 |
| **Total Loop Time** | <50ms | <100ms | Performance test PERF-003 |
| **WebSocket Broadcast** | <5ms | <10ms | Performance test PERF-006 |
| **MIDI Output Jitter** | <1ms | <5ms | Hardware test HW-002 (oscilloscope) |
| **Memory Usage** | <420KB | <520KB | Performance test PERF-007 |
| **CPU Utilization** | <60% | <80% | Performance test PERF-008 |

**Enforcement Strategy**:
- ✅ Add `ASSERT_LT(latency_ms, 20)` to all performance tests
- ✅ Fail CI pipeline if any performance test exceeds MAX threshold
- ✅ Profile with `esp_timer_get_time()` in production code

#### **Memory Budget** (from Phase 04 designs)

```cpp
// Memory Budget Enforcement (src/main.cpp)
constexpr size_t MEMORY_BUDGET_BYTES = 420 * 1024; // 420 KB
constexpr size_t MEMORY_RESERVE_BYTES = 100 * 1024; // 100 KB safety margin

void setup() {
    size_t heap_size = ESP.getFreeHeap();
    if (heap_size < MEMORY_RESERVE_BYTES) {
        Serial.printf("ERROR: Insufficient memory. Available: %d KB, Required: %d KB\n",
                      heap_size / 1024, MEMORY_RESERVE_BYTES / 1024);
        while (true) { delay(1000); } // Halt
    }
}
```

#### **Quality Constraints** (from ISO/IEC/IEEE 12207:2017)

- **Code Coverage**: Minimum 80% line coverage (74 unit tests + 26 integration tests)
- **Static Analysis**: Zero warnings at `-Wall -Wextra` (already in platformio.ini)
- **Test Pyramid**: 70% unit, 20% integration, 10% E2E/hardware
- **Cyclomatic Complexity**: Max 10 per function (enforced in code review)
- **Function Length**: Max 50 lines (XP Simple Design)

#### **Reliability Constraints** (from REQ-NF-003 #10)

- **RTC Fallback**: Must survive 10 consecutive I2C errors (DES-C-005)
- **Network Resilience**: Auto-reconnect WiFi/MQTT with exponential backoff
- **Watchdog Timer**: Enable ESP32 WDT (2-second timeout for main loop)

---

### Q4: Should I set up CI/CD pipeline integration during this phase?

**Answer**: **YES** - Set up GitHub Actions CI pipeline immediately (Day 1 of Phase 05)

#### **CI/CD Strategy**

**Pipeline Stages**:
1. **Build** - Compile for ESP32 + ESP8266 + native
2. **Unit Tests** - Run 74 unit tests on native platform (fast)
3. **Static Analysis** - clang-tidy + cppcheck (zero warnings)
4. **Integration Tests** - Run 26 integration tests on native (mocked hardware)
5. **Deploy** - Upload firmware binary to GitHub Releases (on tag)

**GitHub Actions Workflow** (create `.github/workflows/ci.yml`):

```yaml
name: CI - Build & Test

on:
  push:
    branches: [ master, develop ]
  pull_request:
    branches: [ master ]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Set up Python
        uses: actions/setup-python@v4
        with:
          python-version: '3.11'
      
      - name: Install PlatformIO
        run: |
          pip install platformio
          pio upgrade
      
      - name: Build ESP32 Firmware
        run: pio run -e esp32dev
      
      - name: Build ESP8266 Firmware
        run: pio run -e nodemcuv2
      
      - name: Run Unit Tests (Native)
        run: pio test -e native
      
      - name: Static Analysis
        run: |
          pio check --skip-packages -e esp32dev
      
      - name: Upload Firmware Artifacts
        uses: actions/upload-artifact@v3
        with:
          name: firmware
          path: .pio/build/*/firmware.bin
      
      - name: Test Coverage Report
        run: |
          pio test -e native --coverage
          # TODO: Upload to Codecov
```

**Quality Gates** (All must pass for merge):
- ✅ Build succeeds for ESP32 + ESP8266
- ✅ All unit tests pass (74/74 green)
- ✅ All integration tests pass (26/26 green)
- ✅ Zero compiler warnings
- ✅ Zero static analysis issues
- ✅ Code coverage ≥80%

---

## 🔄 TDD Workflow (Red-Green-Refactor)

### Standard TDD Cycle for Each Component

```
┌─────────────────────────────────────────────────────────────┐
│ 1. RED: Write Failing Test                                  │
│    - Read design spec (DES-C-*, DES-I-*, DES-D-*)           │
│    - Extract acceptance criteria                            │
│    - Write test case (ASSERT expected behavior)             │
│    - Run test → RED (fails, no implementation yet)          │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│ 2. GREEN: Write Minimal Code                                │
│    - Implement ONLY enough to pass test                     │
│    - No refactoring yet                                      │
│    - Run test → GREEN (passes)                              │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│ 3. REFACTOR: Improve Design                                 │
│    - Apply XP Simple Design (YAGNI, DRY, clear intent)      │
│    - Eliminate code smells                                   │
│    - Run ALL tests → GREEN (ensure no regression)           │
└─────────────────────────────────────────────────────────────┘
                          ↓
                      REPEAT for next test
```

### Example: TDD Cycle for DES-C-005 (Timing Manager)

**Iteration 1: Timestamp Query Interface (DES-I-001)**

```cpp
// Step 1: RED - Write failing test (test/test_timing/test_timestamp_query.cpp)
/**
 * Test: DES-I-001 Timestamp Query - Basic Functionality
 * Requirement: REQ-F-004 (#6) - Time synchronization
 * Design: DES-C-005 (#49), DES-I-001 (#52)
 */
TEST(TimingManager, GetTimestampUs_ReturnsMonotonicTime) {
    TimingManager timing;
    timing.init();
    
    uint64_t ts1 = timing.getTimestampUs();
    delayMicroseconds(1000); // 1ms delay
    uint64_t ts2 = timing.getTimestampUs();
    
    // Acceptance Criteria: Monotonicity guarantee
    ASSERT_GT(ts2, ts1); // ts2 > ts1
    ASSERT_GE(ts2 - ts1, 1000); // ≥ 1ms delta
}

// Run: pio test -e native -f test_timestamp_query
// Result: RED (TimingManager not implemented)
```

```cpp
// Step 2: GREEN - Minimal implementation (src/timing/TimingManager.cpp)
#include "TimingManager.h"
#include <Arduino.h>

void TimingManager::init() {
    // Minimal: Use micros() fallback (no RTC yet)
    state.time_source = 1; // millis() fallback
}

uint64_t TimingManager::getTimestampUs() {
    return micros(); // Simplest possible implementation
}

// Run: pio test -e native -f test_timestamp_query
// Result: GREEN (test passes)
```

```cpp
// Step 3: REFACTOR - Improve design (add RTC detection)
void TimingManager::init() {
    // Detect RTC on I2C bus
    Wire.begin();
    Wire.beginTransmission(RTC_I2C_ADDRESS);
    int err = Wire.endTransmission();
    
    if (err == 0) {
        state.rtc_available = true;
        state.time_source = 0; // RTC
        Serial.println("RTC detected");
    } else {
        state.rtc_available = false;
        state.time_source = 1; // millis() fallback
        Serial.println("RTC not found, using micros()");
    }
}

uint64_t TimingManager::getTimestampUs() {
    if (state.rtc_available) {
        return readRTCTimestampUs(); // DES-I-002 implementation
    } else {
        return micros(); // Fallback
    }
}

// Run: pio test -e native (ALL tests)
// Result: GREEN (no regression)
```

**Iteration 2**: Repeat for next interface (DES-I-002: RTC Health Status)

---

## 📝 Implementation Checklist (Per Component)

### Component Implementation Template

For each component (DES-C-001 through DES-C-007):

#### **Phase A: Setup** (30 minutes)
- [ ] Read design specification (GitHub issue #N)
- [ ] Create source folder structure (`src/<component>/`)
- [ ] Create test folder structure (`test/test_<component>/`)
- [ ] Create mock interfaces (from TDD Plan #51)
- [ ] Add component to `src/main.cpp` (stub only)

#### **Phase B: TDD Implementation** (N hours, varies by component)
- [ ] For each interface (DES-I-*):
  - [ ] Write failing test (RED)
  - [ ] Implement minimal code (GREEN)
  - [ ] Refactor for quality (REFACTOR)
  - [ ] Run ALL tests (ensure no regression)
  - [ ] Commit with traceability tags (e.g., "Implements #52 DES-I-001")
- [ ] For each data model (DES-D-*):
  - [ ] Define struct/class in header
  - [ ] Write serialization tests
  - [ ] Implement accessors/mutators
  - [ ] Test memory layout (size, alignment)

#### **Phase C: Integration** (1 hour)
- [ ] Wire up component to `main.cpp` (setup() + loop())
- [ ] Run integration tests (INT-* scenarios)
- [ ] Verify no memory leaks (heap monitoring)
- [ ] Profile performance (meet latency targets)

#### **Phase D: Documentation** (30 minutes)
- [ ] Update `05-implementation/implementation-log.md`
- [ ] Update `05-implementation/traceability-matrix.md`
- [ ] Add inline code comments (Doxygen format)
- [ ] Update README with build/test instructions

#### **Phase E: Quality Gates** (30 minutes)
- [ ] Run full test suite (`pio test -e native`)
- [ ] Run static analysis (`pio check`)
- [ ] Verify code coverage ≥80%
- [ ] Push to GitHub → CI passes
- [ ] Create PR for review (if team environment)

---

## 📊 Traceability Matrix Structure

Create `05-implementation/traceability-matrix.md`:

```markdown
# Phase 05: Implementation Traceability Matrix

| Requirement | Architecture | Design | Source Files | Test Files | Status |
|-------------|--------------|--------|--------------|------------|--------|
| REQ-F-004 (#6) | ARC-C-005 (#21) | DES-C-005 (#49) | src/timing/TimingManager.cpp | test/test_timing/test_timestamp_query.cpp | ✅ |
| | | DES-I-001 (#52) | src/timing/TimingManager.h | test/test_timing/test_rtc_health.cpp | ✅ |
| | | DES-I-002 (#56) | src/interfaces/ITimingProvider.h | test/test_timing/test_time_sync.cpp | ✅ |
| | | DES-D-006 (#70) | src/timing/TimingManagerState.h | test/test_timing/test_timing_state.cpp | ✅ |
```

**Validation**: Use script `scripts/validate-traceability.py` (already exists) to ensure 100% coverage.

---

## 🎯 Success Criteria for Phase 05

### Completion Criteria (All must be met)

- ✅ **All 7 components implemented** (DES-C-001 through DES-C-007)
- ✅ **All 74 unit tests passing** (100% pass rate)
- ✅ **All 26 integration tests passing** (100% pass rate)
- ✅ **Code coverage ≥80%** (line coverage)
- ✅ **All performance targets met** (latency, memory, CPU)
- ✅ **CI pipeline green** (build + test + static analysis)
- ✅ **Traceability complete** (100% REQ → DES → SRC → TEST)
- ✅ **Zero compiler warnings** (`-Wall -Wextra`)
- ✅ **Zero static analysis issues** (clang-tidy, cppcheck)
- ✅ **Documentation updated** (implementation log, traceability matrix)

### Quality Metrics Targets

| Metric | Target | Validation |
|--------|--------|------------|
| Unit Test Pass Rate | 100% | `pio test -e native` |
| Integration Test Pass Rate | 100% | `pio test -e native` |
| Code Coverage | ≥80% | Coverage report |
| Static Analysis Issues | 0 | `pio check` |
| Compiler Warnings | 0 | Build logs |
| Performance Tests | 9/9 pass | PERF-001 through PERF-009 |
| Memory Usage | <420 KB | PERF-007 |
| CPU Utilization | <60% | PERF-008 |

---

## 📅 Implementation Schedule

### Proposed Timeline (8 working days @ 8 hours/day)

**Day 1**: Setup + Wave 1 (Timing Manager)
- Morning: CI/CD setup, project structure
- Afternoon: DES-C-005 (Timing Manager) - 8h

**Day 2-3**: Wave 2 (Audio + BPM)
- Day 2: DES-C-001 (Audio Detection) - 12h (continue to Day 3)
- Day 3: Complete DES-C-001 + DES-C-002 (BPM Calculation) - 8h

**Day 4**: Wave 3 (Config + Output) Part 1
- Morning: DES-C-006 (Configuration Manager) - 6h
- Afternoon: Start DES-C-004 (Output Controller) - 2h

**Day 5**: Wave 3 Part 2
- DES-C-004 (Output Controller) - 8h complete

**Day 6-7**: Wave 4 (Network Services)
- Day 6: DES-C-003 (Web Server) - 14h (continue to Day 7)
- Day 7: Complete DES-C-003 + DES-C-007 (MQTT) - 6h

**Day 8**: Integration + Quality Assurance
- Morning: Integration tests (26 scenarios)
- Afternoon: Performance validation, documentation

---

## 🚀 Next Steps - START HERE

### Immediate Actions (Day 1 Morning)

1. **Set up CI/CD pipeline** (1 hour)
   - Create `.github/workflows/ci.yml`
   - Configure PlatformIO test environments
   - Verify GitHub Actions execution

2. **Create project structure** (30 minutes)
   - Create `src/` component folders
   - Create `test/` test folders
   - Create `test/mocks/` mock interfaces

3. **Start Wave 1: DES-C-005 (Timing Manager)** (6.5 hours)
   - Implement DES-I-001 (Timestamp Query) - TDD cycle
   - Implement DES-I-002 (RTC Health Status) - TDD cycle
   - Implement DES-I-003 (Time Synchronization) - TDD cycle
   - Implement DES-D-006 (TimingManagerState) - Data model

4. **Daily standup** (end of day)
   - Update `05-implementation/implementation-log.md`
   - Commit with traceability tags
   - Push to GitHub → Verify CI green

---

## 📚 Reference Documents

- **TDD Plan**: `04-design/tdd-plan-phase-05.md` (GitHub Issue #51)
- **Component Designs**: GitHub Issues #44-50 (DES-C-001 through DES-C-007)
- **Interface Specs**: GitHub Issues #52-64 (DES-I-001 through DES-I-013)
- **Data Models**: GitHub Issues #65-71 (DES-D-001 through DES-D-007)
- **Architecture Summary**: `03-architecture/ARCHITECTURE-SUMMARY.md`
- **Requirements**: GitHub Issues #1-14 (REQ-* IDs)

---

**Status**: ✅ Phase 05 kickoff complete - Ready to start implementation  
**Next Action**: Set up CI/CD pipeline (Day 1, Task 1)  
**Estimated Duration**: 8 working days (64 hours)  
**End Goal**: Fully tested, standards-compliant ESP32 firmware ready for Phase 06 Integration
