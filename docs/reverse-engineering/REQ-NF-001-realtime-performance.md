# REQ-NF-001: Real-Time Performance Requirements

**Type**: Non-Functional Requirement  
**Priority**: P0 (Critical)  
**Category**: Performance, Reliability  
**Status**: Draft - Reverse Engineered  
**Source**: `ESP_ClapMetronome.ino` lines 134-146, timing-critical sections

---

## Description

System shall provide real-time audio processing with sub-100ms detection latency, maintain accurate timing for metronome beats, and prevent system crashes through watchdog timer management.

---

## Non-Functional Requirements

### REQ-NF-001.1: Clap Detection Latency

System shall detect claps with minimal latency:

- **Target**: <50ms from acoustic event to detection
- **Measurement**: Time from analog spike to threshold trigger
- **Impact**: Affects musician's perception of system responsiveness
- **Critical path**: Analog sampling → threshold comparison → tap recording

### REQ-NF-001.2: Timing Accuracy

System shall maintain accurate beat timing:

- **BPM precision**: ±1 BPM or ±2ms per beat (whichever is larger)
- **Jitter**: <10ms beat-to-beat variation
- **Reference**: `millis()` and `micros()` system timers
- **Update rate**: BPM recalculated on each valid clap

### REQ-NF-001.3: Watchdog Timer Management

System shall prevent crashes and hangs:

**ESP8266**:
- Disable default watchdog: `ESP.wdtDisable()`
- Enable extended watchdog: `ESP.wdtEnable(40000)` (40 seconds)
- Feed interval: Implicit via loop execution

**ESP32**:
- Initialize task watchdog: `esp_task_wdt_init(40000, true)`
- Panic on timeout: System restart enabled
- Thread monitoring: Current task added to watchdog

### REQ-NF-001.4: Loop Execution Timing

System shall maintain responsive main loop:

- **Target loop time**: <100ms per iteration
- **Sampling overhead**: 32 analog reads per loop
- **Processing**: Threshold calculation, BPM updates, MQTT publishes
- **Non-blocking**: No `delay()` calls in main loop

### REQ-NF-001.5: Debouncing and Noise Rejection

System shall prevent false triggers:

- **Debounce delay**: 50ms minimum between state changes
- **Rise time validation**: <6ms from min to trigger
- **Falling edge confirm**: Require signal drops below `threshold - 10`
- **State machine**: `tapped` flag prevents re-trigger until reset

---

## Performance Metrics

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| Clap detection latency | <50ms | `TimeTrig - TimeMin` |
| Beat timing accuracy | ±2ms | Compare `tdelay` to actual intervals |
| Main loop duration | <100ms | `StopSample - StartSample` |
| Watchdog timeout | 40s | Configuration value |
| Debounce window | 50ms | `lastDebounceTime` delta |

---

## Acceptance Criteria

```gherkin
Scenario: Low-latency clap detection
  Given system is sampling at maximum rate
  When acoustic clap occurs
  Then detection occurs within 50ms
  And tap timestamp recorded with microsecond precision

Scenario: Accurate beat timing
  Given system tracking 120 BPM (500ms per beat)
  When 100 beats played
  Then average beat interval is 500ms ±2ms
  And jitter is <10ms standard deviation

Scenario: Watchdog prevents hang (ESP32)
  Given main loop blocks for >40 seconds
  When watchdog timeout expires
  Then system automatically restarts
  And panic message logged

Scenario: Debouncing prevents double-triggers
  Given clap detected and tapped=true
  When signal oscillates near threshold
  Then no additional triggers occur
  Until signal drops below threshold-10
  And tapped flag resets
```

---

## Implementation Details

**Code Location**: `ESP_ClapMetronome.ino`

```cpp
/**
 * Implements: #N (REQ-NF-001: Real-Time Performance Requirements)
 * 
 * Watchdog setup: lines 134-146
 * Timing measurement: lines 230-233, 366-367
 * Debouncing: lines 66, 434-438
 * Rise time check: lines 260-261
 */

// ESP32 watchdog (lines 140-143)
#elif defined(ESP32)
#define WDT_TIMEOUT 3
  esp_task_wdt_init(40000, true);
  esp_task_wdt_add(NULL);
#endif

// Timing measurement (lines 230-233)
int StartSample = last_pulse;
// ... sampling loop ...
int StopSample = lastread;
float SampleDuration = (StopSample-StartSample)/1000;

// Rise time validation (lines 260-261)
float risetime = now - TimeMin[i_fAvg];
if (risetime<riseTlim){ // riseTlim = 6ms
```

---

## Constraints and Assumptions

**Timing constraints**:
- Arduino `millis()`: 1ms resolution (sufficient for BPM)
- Arduino `micros()`: 1μs resolution (sufficient for rise time)
- Loop execution: Must complete before watchdog timeout
- No blocking operations allowed in main loop

**Hardware dependencies**:
- ESP8266: 80/160 MHz CPU, limited by WiFi interrupt overhead
- ESP32: Dual-core 240 MHz, better real-time capabilities
- ADC sampling: ~100μs per `analogRead()` call

---

## Validation Required

⚠️ **Reverse-engineered - Needs validation:**

- [ ] Measure actual clap detection latency with oscilloscope
- [ ] Profile main loop execution time under various conditions
- [ ] Test watchdog functionality (intentional hang test)
- [ ] Validate timing accuracy over extended periods (hours)
- [ ] Check WiFi interrupt impact on timing
- [ ] Verify debounce window prevents all double-triggers
- [ ] Test with various clap volumes and resonance

---

## Traces To

- **Parent**: #N (StR-001: Clap-Based Metronome System)
- **Impacts**: All functional requirements (critical system property)
- **Verified by**: TEST-NF-001 (Performance benchmarks)
- **Architecture**: ADR-ARCH-001 (Real-Time Architecture Pattern)

---

## Create GitHub Issue

```bash
gh issue create \
  --label "type:requirement:non-functional,phase:02-requirements,priority:p0,category:performance" \
  --title "REQ-NF-001: Real-Time Performance Requirements" \
  --body-file REQ-NF-001-realtime-performance.md
```
