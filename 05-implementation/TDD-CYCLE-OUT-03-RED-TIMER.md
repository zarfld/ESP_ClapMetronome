# TDD Cycle OUT-03: Timer-based Clock Sending - RED Phase

**Date**: 2025-11-20  
**Cycle**: OUT-03 (Wave 3.2: Output Controller)  
**Phase**: RED (Write failing tests)  
**Standards**: ISO/IEC/IEEE 12207:2017, XP Test-First Programming  

## 🎯 Objective

Implement **hardware timer-driven MIDI clock sending** to achieve precise 24 PPQN timing at configurable BPM without relying on `loop()` polling.

## 📋 Requirements

### Acceptance Criteria (from TDD Plan)
- **AC-OUT-007**: Output timing jitter <1ms over 1000 outputs
- **AC-OUT-008**: ISR execution time <10µs (updated: <10ms for network latency)
- **AC-OUT-009**: CPU usage <3% average
- **AC-OUT-010**: BPM synchronization - output interval matches BPM

### Functional Requirements
- Timer interrupts at 24 PPQN intervals (e.g., 140 BPM = 56ms per beat = 2.33ms per tick)
- Safe ISR implementation (minimal work, flag-based signaling)
- Dynamic BPM updates reconfigure timer interval
- Start/stop control enables/disables timer
- Jitter measurement and reporting

## 🧪 Test Plan (RED Phase)

### Test Suite: test_timer_clock

**Estimated Tests**: 10 tests

#### 1. TimerInitialization_CalculatesCorrectInterval
**Given**: OutputController configured with 140 BPM, 24 PPQN  
**When**: `startSync()` called  
**Then**: Timer interval = (60,000,000 µs / 140 BPM / 24 PPQN) = 17,857 µs  

#### 2. TimerISR_TriggersAt24PPQN
**Given**: Timer running at 120 BPM  
**When**: Advance time by 20.833ms (one tick at 120 BPM)  
**Then**: Clock pulse counter incremented  

#### 3. ClockSending_FromISR
**Given**: Timer interrupt triggered  
**When**: ISR executes  
**Then**: MIDI Clock (0xF8) sent via network  
**And**: Last clock timestamp recorded  

#### 4. BPMChange_UpdatesTimerInterval
**Given**: Timer running at 120 BPM (interval 20.833ms)  
**When**: `setBPM(140)` called  
**Then**: Timer reconfigured to 17.857ms interval  
**And**: No clock pulses dropped during transition  

#### 5. StartSync_EnablesTimer
**Given**: Timer disabled, BPM = 130  
**When**: `startSync()` called  
**Then**: Timer enabled, interrupts start firing  
**And**: MIDI Start (0xFA) sent  
**And**: Clock counter reset to 0  

#### 6. StopSync_DisablesTimer
**Given**: Timer running  
**When**: `stopSync()` called  
**Then**: Timer disabled, no more interrupts  
**And**: MIDI Stop (0xFC) sent  
**And**: Clock counter holds final value  

#### 7. JitterMeasurement_UnderThreshold
**Given**: Timer running for 100 clock pulses  
**When**: Jitter calculated (std deviation of intervals)  
**Then**: Jitter <1ms (AC-OUT-007)  
**And**: Statistics available via `getTimerStats()`  

#### 8. ISRExecutionTime_Under10us
**Given**: Timer ISR triggered  
**When**: ISR executes (flag set + counter increment)  
**Then**: Execution time <10µs (measured via timestamp delta)  
**Note**: Full network send happens in `loop()`, not ISR  

#### 9. CPUUsage_Under3Percent
**Given**: Timer running at 140 BPM for 10 seconds  
**When**: CPU profiling enabled  
**Then**: Timer overhead <3% (AC-OUT-009)  
**And**: Main loop remains responsive  

#### 10. ClockCounter_Wraps24PPQN
**Given**: Timer running, clock_counter = 23  
**When**: Next clock pulse (24th in beat)  
**Then**: Clock counter wraps to 0  
**And**: Beat counter increments  

## 🏗️ Interface Design

### Timer Configuration Structure
```cpp
struct TimerConfig {
    uint32_t interval_us;       // Calculated from BPM and PPQN
    bool enabled;               // Timer active state
    uint16_t bpm;               // Current tempo
    uint8_t ppqn;               // Pulses per quarter note (default 24)
};
```

### Timer Statistics Structure
```cpp
struct TimerStats {
    uint32_t total_interrupts;  // Total ISR triggers
    uint32_t clocks_sent;       // Successful clock sends
    uint32_t missed_clocks;     // Dropped due to overrun
    float jitter_ms;            // Timing jitter (std dev)
    uint32_t avg_isr_time_us;   // Average ISR execution time
    uint32_t max_isr_time_us;   // Maximum ISR execution time
};
```

### New Methods (OutputController)
```cpp
// Timer control
bool startTimerClock();         // Enable timer interrupts
bool stopTimerClock();          // Disable timer interrupts
bool setBPM(uint16_t bpm);      // Update tempo (reconfigure timer)

// Timer configuration
TimerConfig getTimerConfig() const;
bool setTimerInterval(uint32_t interval_us);

// Statistics and diagnostics
TimerStats getTimerStats() const;
void resetTimerStats();

// ISR handler (called from static ISR)
void handleTimerInterrupt();    // Non-static, called by static wrapper
```

### Static ISR Wrapper
```cpp
// Static ISR (required for ESP32 hw_timer_t)
static OutputController* timer_instance;  // Singleton for ISR access
static void IRAM_ATTR onTimerISR();       // Static wrapper calls handleTimerInterrupt()
```

## 🔧 Implementation Strategy

### Phase 1: Timer Configuration (Tests 1, 5, 6)
- Calculate interval from BPM/PPQN
- Initialize ESP32 `hw_timer_t` (or mock for native tests)
- Enable/disable timer on start/stop

### Phase 2: ISR and Clock Sending (Tests 2, 3, 8)
- Implement static ISR wrapper
- Set flag in ISR (minimal work)
- Process flag in `loop()` to send MIDI clock
- Measure ISR execution time

### Phase 3: Dynamic BPM Updates (Test 4)
- Reconfigure timer interval atomically
- Ensure no dropped clocks during transition
- Validate interval calculation

### Phase 4: Jitter and Performance (Tests 7, 9, 10)
- Track actual vs expected intervals
- Calculate jitter statistics
- Measure CPU overhead
- Verify counter wraparound

## 🧰 Mock/Test Infrastructure

### Time Mocking
```cpp
// Extend existing time_mock.h
void trigger_timer_isr();           // Manually trigger ISR for testing
void advance_to_next_tick();        // Advance time to next timer interval
uint32_t get_isr_call_count();      // Count ISR invocations
```

### Hardware Timer Mock
```cpp
#ifdef NATIVE_BUILD
class MockHardwareTimer {
public:
    void begin(uint32_t interval_us);
    void enable();
    void disable();
    void setInterval(uint32_t interval_us);
    
    // Test control
    void trigger();                 // Simulate ISR fire
    bool isEnabled() const;
    uint32_t getInterval() const;
};
#endif
```

## 📊 Expected RED Phase Results

After creating tests, expect:
- **0/10 tests passing** (all RED)
- Compilation succeeds (interfaces defined)
- Tests fail with expected messages:
  - "Timer not initialized"
  - "Interval calculation incorrect"
  - "ISR not triggered"
  - "Clock not sent"

## ✅ RED Phase Completion Criteria

- ✅ All 10 tests written and compiling
- ✅ Test fixture created with setup/teardown
- ✅ Mock infrastructure in place
- ✅ Expected failures documented
- ✅ GREEN phase implementation plan ready

## 🎯 Next Steps (GREEN Phase)

1. Implement `startTimerClock()` / `stopTimerClock()`
2. Add ESP32 `hw_timer_t` initialization (with mock for native)
3. Implement static ISR wrapper + `handleTimerInterrupt()`
4. Add flag-based clock sending (ISR sets flag, loop sends)
5. Implement `setBPM()` with interval recalculation
6. Add jitter tracking and statistics
7. Verify all 10 tests pass
8. Run performance validation

---

**Status**: Ready to begin RED phase implementation  
**Estimated Duration**: 1 hour (RED), 2-3 hours (GREEN)  
**Dependencies**: Cycles OUT-01 (MIDI) and OUT-02 (Network) complete ✅
