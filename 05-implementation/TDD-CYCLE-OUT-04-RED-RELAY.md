# TDD Cycle OUT-04: Relay Output Implementation - RED Phase

**Date**: 2025-11-20  
**Cycle**: OUT-04 (Wave 3.2: Output Controller)  
**Phase**: RED (Write failing tests)  
**Standards**: ISO/IEC/IEEE 12207:2017, XP Test-First Programming  

## 🎯 Objective

Implement **GPIO-based relay pulse output** for triggering external equipment (drum machines, synthesizers) synchronized to BPM.

## 📋 Requirements

### Acceptance Criteria
- **AC-OUT-005**: Relay pulse duration configurable 10-500ms (default 50ms)
- **AC-OUT-006**: Relay watchdog - force OFF if stuck >100ms
- **AC-OUT-012**: Relay GPIO safety - LOW when disabled

### Functional Requirements
- Pulse output on beat detection or timer trigger
- Configurable pulse duration via OutputConfig
- Watchdog timer to prevent stuck-ON relay
- Debounce period (minimum OFF time)
- Safe default state (GPIO LOW)
- State tracking for testing/monitoring

## 🧪 Test Plan (RED Phase)

### Test Suite: test_relay_output

**Estimated Tests**: 8-10 tests

#### 1. RelayInitialization_DefaultLowState
**Given**: OutputController created  
**When**: Relay GPIO queried  
**Then**: GPIO state is LOW (safe default)  
**Verifies**: AC-OUT-012

#### 2. RelayPulse_ConfiguredDuration
**Given**: Relay pulse_ms configured to 50ms  
**When**: `pulseRelay()` called  
**Then**: GPIO HIGH immediately  
**And**: After 50ms, GPIO returns to LOW  
**Verifies**: AC-OUT-005

#### 3. RelayPulse_CustomDuration
**Given**: Relay pulse_ms configured to 100ms  
**When**: `pulseRelay()` called  
**Then**: GPIO HIGH for 100ms, then LOW  
**Verifies**: AC-OUT-005

#### 4. RelayWatchdog_ForcesOffAt100ms
**Given**: Relay turned ON manually (simulated failure)  
**When**: 101ms elapsed without turning OFF  
**Then**: Watchdog forces GPIO LOW  
**And**: State transitions to WATCHDOG  
**Verifies**: AC-OUT-006

#### 5. RelayDebounce_MiniumumOffTime
**Given**: Relay pulse completed  
**When**: Another pulse requested within debounce period (10ms)  
**Then**: Pulse rejected (returns false)  
**And**: GPIO remains LOW  

#### 6. RelayState_DisabledMode
**Given**: OutputMode = MIDI_ONLY (relay disabled)  
**When**: `pulseRelay()` called  
**Then**: Returns false  
**And**: GPIO remains LOW  
**Verifies**: AC-OUT-012

#### 7. RelayMultiplePulses_Sequential
**Given**: Relay configured for 30ms pulses  
**When**: 5 pulses requested sequentially with proper spacing  
**Then**: All 5 pulses complete successfully  
**And**: GPIO HIGH for 30ms each time  

#### 8. RelayStats_TrackPulseCount
**Given**: Relay pulses sent  
**When**: Statistics queried  
**Then**: pulse_count increments correctly  
**And**: last_pulse_timestamp recorded  

#### 9. RelayGPIOSafety_OnDestruction
**Given**: OutputController with active relay  
**When**: Destructor called  
**Then**: GPIO forced LOW before cleanup  
**Verifies**: AC-OUT-012

#### 10. RelayOverride_ManualControl
**Given**: Relay in automatic mode  
**When**: Manual GPIO control used (`setRelayGPIO(true)`)  
**Then**: GPIO state changes immediately  
**And**: Watchdog still active  

## 🏗️ Interface Design

### Relay Configuration (Already in OutputConfig)
```cpp
struct OutputConfig {
    uint16_t relay_pulse_ms;        // Pulse duration (10-500ms, default 50ms)
    uint16_t relay_watchdog_ms;     // Watchdog timeout (default 100ms)
    uint16_t relay_debounce_ms;     // Minimum OFF time (default 10ms)
    // ... existing fields
};
```

### Relay Statistics Structure
```cpp
struct RelayStats {
    uint32_t pulse_count;           // Total pulses since start
    uint32_t watchdog_triggers;     // Times watchdog fired
    uint32_t debounce_rejects;      // Pulses rejected by debounce
    uint64_t last_pulse_us;         // Timestamp of last pulse
    bool currently_on;              // Current GPIO state
};
```

### New/Updated Methods
```cpp
// Already declared in OutputController.h
bool pulseRelay();                  // Trigger relay pulse
void setRelayGPIO(bool high);       // Manual GPIO control
bool getRelayGPIO() const;          // Query GPIO state

// New methods to add
RelayStats getRelayStats() const;   // Query statistics
void resetRelayStats();             // Clear counters
void processRelayWatchdog();        // Check watchdog (call in loop/timer)
```

## 🔧 Implementation Strategy

### Phase 1: Basic Pulse Control (Tests 1, 2, 3)
- Initialize GPIO to LOW
- Implement `pulseRelay()` to set HIGH and record timestamp
- Add `processRelayWatchdog()` to turn OFF after duration
- Track pulse start time

### Phase 2: Watchdog and Safety (Tests 4, 6, 9)
- Check elapsed time in `processRelayWatchdog()`
- Force OFF if exceeds watchdog_ms
- Transition to WATCHDOG state
- Ensure destructor sets GPIO LOW

### Phase 3: Debounce and Mode Control (Tests 5, 7)
- Track last pulse end time
- Reject pulses within debounce period
- Check OutputMode before allowing pulse
- Validate sequential pulses

### Phase 4: Statistics and Manual Control (Tests 8, 10)
- Increment pulse_count on each pulse
- Track watchdog_triggers and debounce_rejects
- Record last_pulse_us timestamp
- Allow manual GPIO override (with watchdog still active)

## 🧰 Mock/Test Infrastructure

### Time Advancement
Already available from previous cycles:
```cpp
void advance_time_us(uint64_t delta);   // Advance mock time
uint64_t micros();                      // Current mock time
void reset_mock_time();                 // Reset to 0
```

### GPIO State Tracking
GPIO state is stored in `relay_on_` member, so no additional mocking needed for native tests. Actual GPIO calls will be `#ifdef NATIVE_BUILD` guarded.

## 📊 Expected RED Phase Results

After creating tests, expect:
- **0-2/10 tests passing** initially
- Tests fail with expected messages:
  - "Relay not initialized to LOW"
  - "Pulse duration incorrect"
  - "Watchdog did not fire"
  - "Debounce not enforced"
  - "Statistics not tracked"

## ✅ RED Phase Completion Criteria

- ✅ All 10 tests written and compiling
- ✅ Test fixture with setup/teardown
- ✅ Expected failures documented
- ✅ Clear path to GREEN implementation

## 🎯 Next Steps (GREEN Phase)

1. Implement `pulseRelay()` - set HIGH, record timestamp
2. Implement `processRelayWatchdog()` - check timeout, set LOW
3. Add debounce logic - track last pulse end
4. Implement statistics tracking
5. Add destructor GPIO safety
6. Verify all 10 tests pass

---

**Status**: Ready to begin RED phase  
**Estimated Duration**: 1 hour (RED), 1-2 hours (GREEN)  
**Dependencies**: OUT-01, OUT-02, OUT-03 complete ✅
