# TDD Cycle OUT-01: RED Phase Completion

**Date**: 2025-01-18  
**Wave**: 3.2 Output Controller  
**Cycle**: OUT-01 (MIDI Beat Clock Format)  
**Phase**: RED (Tests written, implementation incomplete)  
**Status**: ✅ RED phase complete - compilation failures expected

---

## Objective

Implement RTP-MIDI Beat Clock protocol (0xF8/0xFA/0xFC messages) via network transport (UDP/WiFi), replacing the initially incorrect MIDI note-on/off design.

---

## Design Corrections Applied

### Critical Correction 1: MIDI Protocol
- **Original (v0.1.0)**: MIDI note-on/off (0x90/0x80) with note numbers and velocities
- **Corrected (v0.2.0)**: MIDI Beat Clock (0xF8 Timing Clock, 0xFA START, 0xFC STOP)
- **Reason**: User requirement was "send MIDI beat clock," not MIDI notes

### Critical Correction 2: Transport Layer
- **Original (v0.1.0)**: DIN MIDI serial UART (31.25 kbaud)
- **Corrected (v0.2.0)**: RTP-MIDI network transport (UDP over WiFi, ports 5004/5005)
- **Reason**: Project uses RTP-MIDI (no DIN MIDI hardware port)

---

## Test File Created

**File**: `test/test_output/test_midi_beat_clock.cpp`

**Total Tests**: 15 tests covering:
- MIDI Beat Clock message format (0xF8, 0xFA, 0xFC)
- 24 PPQN clock rate configuration
- Clock interval calculation (60 seconds / BPM / 24)
- Synchronization workflow (START → clocks → STOP)
- RTP-MIDI network configuration (port 5004)
- State machine transitions (STOPPED → RUNNING → STOPPED)

---

## RED Phase Results

### Build Command
```powershell
cd d:\Repos\ESP_ClapMetronome\test\test_output
cmake -S . -B build -A x64
cmake --build build --config Debug
```

### Compilation Errors (Expected)

#### Missing Method: `getOutputState()`
```
error C2039: "getOutputState" ist kein Member von "OutputController".
```
**Tests affected**: 12 tests
**Fix required**: Add `OutputState getOutputState() const` method to header and implementation

#### Missing Configuration Fields
```
error C2039: "midi_enabled" ist kein Member von "OutputConfig".
```
**Tests affected**: 1 test
**Fix required**: Add fields to `OutputConfig` struct:
- `bool midi_enabled`
- `bool midi_send_start_stop`
- `uint8_t midi_ppqn` (already exists in corrected header)
- `uint16_t rtp_port` (already exists in corrected header)

#### Missing Helper Function: `micros()`
```
error C3861:  "micros": Bezeichner wurde nicht gefunden.
```
**Tests affected**: Test fixture helper
**Fix required**: Add Arduino-compatible `micros()` function or mock for native builds

#### Old Test File Errors
```
error C2065: "MIDIPacket": nichtdeklarierter Bezeichner
```
**File**: `test_midi_output.cpp` (old MIDI note tests)
**Action**: This file is outdated and will be removed/replaced; not blocking Beat Clock development

---

## Implementation TODO (GREEN Phase)

### 1. Add `getOutputState()` Method
```cpp
// In OutputController.h
struct OutputState {
    bool     midi_enabled;
    bool     relay_enabled;
    bool     syncing;              // true if clock stream active
    uint32_t clocks_sent;          // Total 0xF8 messages since START
    uint32_t last_clock_time_ms;   // millis() of last clock
    float    current_bpm;          // Active BPM tempo
    OutputMode mode;               // Current output mode
};

OutputState getOutputState() const;

// In OutputController.cpp
OutputState OutputController::getOutputState() const {
    OutputState state;
    state.midi_enabled = config_.midi_enabled;
    state.relay_enabled = config_.relay_enabled;
    state.syncing = syncing_;
    state.clocks_sent = clocks_sent_;
    state.last_clock_time_ms = last_clock_us_ / 1000;
    state.current_bpm = current_bpm_;
    state.mode = config_.mode;
    return state;
}
```

### 2. Add Missing OutputConfig Fields
Already present in corrected header (`rtp_port`, `midi_ppqn`), but need initialization:
```cpp
OutputConfig() :
    midi_enabled(true),
    midi_send_start_stop(true),
    midi_ppqn(24),
    rtp_port(5004),
    relay_enabled(false),
    relay_pulse_ms(50),
    relay_watchdog_ms(100),
    relay_debounce_ms(10),
    mode(OutputMode::MIDI_ONLY)
{}
```

### 3. Implement sendMIDIClock()
```cpp
bool OutputController::sendMIDIClock() {
    if (config_.mode == OutputMode::DISABLED || config_.mode == OutputMode::RELAY_ONLY) {
        return false;
    }
    sendMIDIRealTime(static_cast<uint8_t>(MIDIClockMessage::TIMING_CLOCK));
    clocks_sent_++;
    midi_messages_sent_++;
    return true;
}
```

### 4. Implement sendMIDIStart()
```cpp
bool OutputController::sendMIDIStart() {
    if (config_.mode == OutputMode::DISABLED || config_.mode == OutputMode::RELAY_ONLY) {
        return false;
    }
    sendMIDIRealTime(static_cast<uint8_t>(MIDIClockMessage::START));
    clocks_sent_ = 0;  // Reset clock counter on START
    midi_messages_sent_++;
    return true;
}
```

### 5. Implement sendMIDIStop()
```cpp
bool OutputController::sendMIDIStop() {
    if (config_.mode == OutputMode::DISABLED || config_.mode == OutputMode::RELAY_ONLY) {
        return false;
    }
    sendMIDIRealTime(static_cast<uint8_t>(MIDIClockMessage::STOP));
    midi_messages_sent_++;
    return true;
}
```

### 6. Implement setBPM()
```cpp
void OutputController::setBPM(float bpm) {
    current_bpm_ = bpm;
    updateOutputInterval();
}

void OutputController::updateOutputInterval() {
    // MIDI Beat Clock: 24 clocks per quarter note
    // Interval = (60 seconds / BPM) / 24 clocks
    // Convert to microseconds: interval_us = (60 * 1,000,000) / BPM / 24
    //                                       = 2,500,000 / BPM
    clock_interval_us_ = static_cast<uint32_t>(2500000.0f / current_bpm_);
    
    // Clamp to valid range (40-300 BPM)
    if (clock_interval_us_ < 8333) clock_interval_us_ = 8333;    // 300 BPM
    if (clock_interval_us_ > 62500) clock_interval_us_ = 62500;  // 40 BPM
}
```

### 7. Implement startSync() / stopSync()
```cpp
void OutputController::startSync() {
    if (config_.midi_send_start_stop && config_.mode != OutputMode::RELAY_ONLY) {
        sendMIDIStart();
    }
    syncing_ = true;
    last_clock_us_ = micros();
}

void OutputController::stopSync() {
    if (config_.midi_send_start_stop && config_.mode != OutputMode::RELAY_ONLY) {
        sendMIDIStop();
    }
    syncing_ = false;
}
```

### 8. Implement sendMIDIRealTime() (Stub)
```cpp
void OutputController::sendMIDIRealTime(uint8_t message) {
    // RED: Stub implementation for TDD
    // Will construct RTP-MIDI packet and send via UDP socket
    // (Full implementation in later cycle)
    
    // For now, just log the message (test verification will be mocked)
    #ifdef NATIVE_BUILD
        // Mock capture for tests
    #else
        Serial.printf("MIDI RT: 0x%02X\n", message);
    #endif
}
```

### 9. Add Time Helper Functions (Native Build)
```cpp
// In a test helper file or mock
#ifdef NATIVE_BUILD
    static uint64_t mock_micros_value = 0;
    uint64_t micros() { return mock_micros_value; }
    void set_mock_micros(uint64_t value) { mock_micros_value = value; }
#endif
```

---

## Acceptance Criteria Tested

| AC ID | Description | Test Count | Status |
|---|---|---|---|
| AC-OUT-001 | MIDI timing clock format (0xF8) | 1 | ❌ Blocked (missing getOutputState) |
| AC-OUT-002 | MIDI START message (0xFA) | 2 | ❌ Blocked (missing getOutputState) |
| AC-OUT-003 | RTP-MIDI network transport | 2 | ❌ Blocked (missing getOutputState) |
| AC-OUT-004 | 24 PPQN clock rate | 1 | ❌ Blocked (missing getConfig) |
| AC-OUT-008 | Network latency handling | 0 | ⏳ Deferred to cycle OUT-06 |
| AC-OUT-010 | BPM synchronization (interval) | 3 | ❌ Blocked (missing setBPM) |
| AC-OUT-011 | MIDI STOP message (0xFC) | 2 | ❌ Blocked (missing getOutputState) |
| AC-OUT-013 | State machine transitions | 4 | ❌ Blocked (missing getOutputState) |

**Total**: 15 tests, **0 passing** (expected for RED phase)

---

## Next Steps (GREEN Phase)

1. **Add missing API methods** to `OutputController.h` and `.cpp`:
   - `getOutputState()`
   - `getConfig()`
2. **Implement MIDI Beat Clock methods**:
   - `sendMIDIClock()` → 0xF8
   - `sendMIDIStart()` → 0xFA
   - `sendMIDIStop()` → 0xFC
   - `sendMIDIRealTime(uint8_t)` (stub for now)
3. **Implement BPM and timing**:
   - `setBPM(float)` and `updateOutputInterval()`
4. **Implement sync control**:
   - `startSync()` and `stopSync()`
5. **Add state tracking**:
   - `clocks_sent_` counter
   - `syncing_` boolean
6. **Add time helpers** for native builds (`micros()` mock)
7. **Rebuild and run tests** → expect 15/15 passing (GREEN phase)

---

## Files Changed

### Created
- `test/test_output/test_midi_beat_clock.cpp` (456 lines, 15 tests)

### Modified
- `test/test_output/CMakeLists.txt` (added test_midi_beat_clock target)

### To Modify (GREEN Phase)
- `src/output/OutputController.h` (add getOutputState(), getConfig())
- `src/output/OutputController.cpp` (implement 8 methods)
- `test/mocks/time_mock.h` (add micros() for native builds) - TO BE CREATED

---

## Traceability

**Design**: DES-C-004 Output Controller (#50)  
**Architecture**: ARC-C-004 Output Subsystem (#24)  
**Requirements**: REQ-F-008 MIDI Output (#11)  
**TDD Plan**: `04-design/tdd-plan-phase-05.md` (Wave 3.2, Cycle OUT-01)

---

## Estimated Time to GREEN

**Implementation**: 2 hours  
**Testing/Debugging**: 30 minutes  
**Total**: 2.5 hours

---

**Status**: ✅ RED phase complete, ready for GREEN phase implementation
