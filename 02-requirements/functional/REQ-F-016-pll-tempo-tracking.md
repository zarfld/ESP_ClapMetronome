# Functional Requirement: PLL Tempo Tracking

**Requirement ID**: REQ-F-016  
**Issue**: TBD (to be created)  
**Phase**: 02 - Requirements  
**Priority**: High  
**Status**: Draft  
**Date**: 2025-11-23  
**Standards**: ISO/IEC/IEEE 29148:2018 (System Requirements)

## 1. Requirement Statement

The BPM calculation system **shall** implement Phase-Locked Loop (PLL) tempo tracking to maintain smooth, continuous tempo estimation that locks to the rhythmic groove and gracefully handles timing imperfections, missed beats, and gradual tempo changes.

## 2. Traceability

### Traces From
- **StR-002.2**: Smooth Tempo Tracking (PLL)
- Current implementation: Tempo locking logic in `calculateBPM()` lines 243-289

### Traces To
- **ADR-BPM-004**: Architecture decision for PLL implementation
- **DES-C-004**: PLL Tempo Tracker component design
- **TEST-BPM-016-xx**: Test cases for PLL functionality

## 3. Functional Behavior

### 3.1 Concept Overview

A Phase-Locked Loop tracks both:
- **Period (T)**: Seconds per beat (inverse of tempo)
- **Phase (φ)**: Predicted timestamp of next beat

On each beat, the system compares actual timing to prediction, adjusting both phase and period to maintain lock.

### 3.2 Inputs
- **beat_timestamp_us** (uint64_t): Actual beat occurrence time
- **current_tempo_estimate** (float): From histogram or averaging (BPM)

### 3.3 Processing

**Initialization:**
```
T = 60,000,000 / initial_tempo_bpm  // Period in microseconds
phi = beat_timestamp_us + T         // Predict first beat
alpha = 0.2                         // Phase correction gain
beta = 0.01                         // Tempo correction gain
```

**On Each Beat:**
```
// 1. Calculate timing error
error_us = beat_timestamp_us - phi

// 2. Correct phase (faster response)
phi = phi + (alpha * error_us)

// 3. Correct period (slower adaptation for stability)
T = T + (beta * error_us)

// 4. Clamp period to valid range
T = clamp(T, 60000000/MAX_BPM, 60000000/MIN_BPM)

// 5. Update tempo
tracked_tempo_bpm = 60,000,000 / T

// 6. Predict next beat
phi = phi + T
```

### 3.4 Outputs
- **tracked_tempo_bpm** (float): Smoothly tracked tempo (60-200 BPM)
- **phase_error_ms** (float): Timing error in milliseconds (for telemetry)
- **is_locked** (bool): True when error consistently < threshold

### 3.5 Lock Detection
```
IF abs(error_us) < LOCK_THRESHOLD_US (e.g., 50ms) THEN
    lock_count++
    IF lock_count >= LOCK_COUNT_THRESHOLD (e.g., 3) THEN
        is_locked = true
    END IF
ELSE
    lock_count = 0
    is_locked = false
END IF
```

## 4. Acceptance Criteria

### AC-016-001: Basic Tracking
**Given** Steady beats at 120 BPM (500ms intervals)  
**When** PLL processes 10 beats  
**Then** Tracked tempo converges to 120 BPM ± 0.5 BPM

**Test**: Unit test with synthetic steady rhythm
```cpp
// Feed 10 beats at exactly 500ms intervals
for (int i = 0; i < 10; i++) {
    pll.addBeat(i * 500000);  // 500ms intervals
}
EXPECT_NEAR(pll.getTempo(), 120.0f, 0.5f);
```

### AC-016-002: Smooth Tempo Changes
**Given** Gradual acceleration from 100 to 120 BPM over 16 beats  
**When** PLL tracks tempo  
**Then** Tempo changes smoothly, no jumps > 2 BPM between beats

**Test**: Integration test with gradual acceleration
```cpp
// Generate accelerating beat pattern
// Expected: smooth increase without abrupt jumps
for (int i = 1; i < timestamps.size(); i++) {
    float tempo_change = abs(tempos[i] - tempos[i-1]);
    EXPECT_LE(tempo_change, 2.0f);
}
```

### AC-016-003: Missed Beat Tolerance
**Given** PLL locked to 120 BPM, one beat missed (double interval)  
**When** Next beat arrives  
**Then** PLL maintains lock, error corrected within 2 beats

**Test**: Resilience test with single dropout
```cpp
// Pattern: 500,500,500, 1000 (missed beat), 500,500
pll.addBeat(0);
pll.addBeat(500000);
pll.addBeat(1000000);
pll.addBeat(2000000);  // Missed one beat
pll.addBeat(2500000);
pll.addBeat(3000000);
// After 2 more beats, should be back on track
EXPECT_TRUE(pll.isLocked());
EXPECT_NEAR(pll.getTempo(), 120.0f, 1.0f);
```

### AC-016-004: Phase Lock Detection
**Given** PLL tracking steady 120 BPM for 5+ beats  
**When** Timing error < 50ms for 3 consecutive beats  
**Then** `isLocked()` returns true

**Test**: Lock state verification
```cpp
// Feed 8 steady beats
for (int i = 0; i < 8; i++) {
    pll.addBeat(i * 500000);
}
EXPECT_TRUE(pll.isLocked());
```

### AC-016-005: Performance
**Given** PLL calculation on ESP32  
**When** Single beat processed  
**Then** Processing time < 200µs

**Test**: Performance profiling
```cpp
auto start = micros();
pll.addBeat(timestamp);
auto elapsed = micros() - start;
EXPECT_LT(elapsed, 200);
```

### AC-016-006: Memory Budget
**Given** PLL state structure  
**When** Compiled for ESP32  
**Then** Memory usage ≤ 64 bytes

**Test**: Static memory analysis
```cpp
EXPECT_LE(sizeof(PLLState), 64);
```

### AC-016-007: Integration with Histogram
**Given** Histogram provides tempo estimate + confidence  
**When** Confidence > 70% and tempo differs from PLL by >5 BPM  
**Then** PLL gradually adapts to histogram tempo (not sudden jump)

**Test**: Histogram + PLL integration
```cpp
// PLL at 120 BPM, histogram suggests 110 BPM (high confidence)
// Expect: Gradual shift over 4-6 beats, not instant jump
```

## 5. Non-Functional Requirements

### NFR-016-001: Convergence Speed
- Initial lock: 3-5 beats from cold start
- Re-lock after disruption: 2-3 beats
- Tempo change tracking: 8-12 beats for 10 BPM shift

### NFR-016-002: Stability
- Phase jitter: ±10ms standard deviation in steady state
- Tempo variance: ±0.2 BPM standard deviation over 32 beats
- No oscillation/hunting behavior

### NFR-016-003: Numerical Precision
- Use 64-bit integers for timestamps (microsecond precision)
- Use float for period/phase (sufficient for audio timing)
- Avoid accumulation errors (reset phase on overflow protection)

## 6. Interface Specification

### 6.1 Public API
```cpp
class PLLTempoTracker {
public:
    void init(float initial_tempo_bpm, uint64_t start_time_us);
    void reset();
    
    // Process new beat
    void addBeat(uint64_t beat_timestamp_us);
    
    // Update from external tempo estimate (e.g., histogram)
    void updateTempoEstimate(float external_tempo_bpm, float confidence);
    
    // Get current tracking state
    float getTempo() const;
    float getPhaseError() const;  // Milliseconds
    bool isLocked() const;
    uint64_t getPredictedNextBeat() const;
    
private:
    struct {
        uint64_t period_us;        // T: microseconds per beat
        uint64_t phase_us;         // φ: predicted next beat time
        float alpha;               // Phase correction gain (0.2)
        float beta;                // Tempo correction gain (0.01)
        uint8_t lock_count;        // Consecutive beats within threshold
        bool is_locked;            // Lock status
        int32_t last_error_ms;     // For telemetry
    } state_;
};
```

### 6.2 Integration Point
Integrate into `BPMCalculation.cpp` after histogram/averaging:
```cpp
// Get tempo estimate from histogram or averaging
float estimated_tempo = histogram_detector_.getTempo();

// Feed to PLL tracker
pll_tracker_.updateTempoEstimate(estimated_tempo, confidence);

// Use PLL output as final tempo
state_.current_bpm = pll_tracker_.getTempo();
state_.is_locked = pll_tracker_.isLocked();
```

## 7. Design Constraints

### 7.1 Configuration Constants
```cpp
static constexpr float PLL_ALPHA = 0.2f;        // Phase correction gain
static constexpr float PLL_BETA = 0.01f;        // Tempo correction gain
static constexpr uint64_t LOCK_THRESHOLD_US = 50000;  // 50ms timing tolerance
static constexpr uint8_t LOCK_COUNT_THRESHOLD = 3;    // Beats to confirm lock
static constexpr float MAX_TEMPO_CHANGE_PER_BEAT = 2.0f;  // BPM, safety clamp
```

### 7.2 Tuning Parameters
- **alpha (0.2)**: Higher = faster phase correction, more jitter
- **beta (0.01)**: Higher = faster tempo adaptation, less stability
- Typical ratio: alpha = 20× beta

### 7.3 Feature Flag
```cpp
#define ENABLE_PLL_TRACKER 1  // Compile-time toggle
```

## 8. Validation Approach

### 8.1 Unit Testing (Native)
- Test basic tracking with steady tempo
- Test convergence speed (cold start → lock)
- Test missed beat recovery
- Test gradual tempo changes
- Test edge cases (very slow/fast, timing outliers)

### 8.2 Integration Testing (ESP32)
- Test with real drum patterns (varying timing)
- Test histogram → PLL feedback loop
- Test lock detection with real audio
- Test phase prediction accuracy

### 8.3 Performance Testing
- Profile CPU cycles per beat on ESP32
- Measure memory footprint
- Stress test with rapid tempo changes

## 9. Dependencies

### 9.1 Prerequisites
- Accurate beat timestamps (Phase 28 audio detection fix)
- Tempo estimate source (histogram or averaging)
- Microsecond-precision timing (`ITimingProvider`)

### 9.2 Related Requirements
- **REQ-F-015**: Histogram Tempo Detection (provides PLL input)
- **REQ-F-002**: Existing BPM calculation (PLL replaces locking logic)

## 10. Risks and Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| PLL diverges on tempo changes | High | Add upper bound on tempo change per beat, reset on large error |
| Instability from noise | Medium | Tune alpha/beta conservatively, require 3 beats to lock |
| Numerical overflow in phase | Low | Use uint64_t, reset phase periodically |
| Too slow to lock | Medium | Adjustable alpha/beta, A/B test with users |

## 11. Success Metrics

- **Lock time**: <5 beats from cold start (95th percentile)
- **Phase accuracy**: ±10ms standard deviation when locked
- **Tempo accuracy**: ±0.5 BPM when tracking steady rhythm
- **Resilience**: Maintains lock through single missed beat (90% of cases)
- **Performance**: <200µs per beat on ESP32

## 12. Open Issues

1. **Gain tuning**: Should alpha/beta be runtime-configurable or compile-time constants?
2. **Reset policy**: When should PLL hard-reset (vs. gradual adaptation)?
3. **Confidence integration**: Should PLL gains scale with histogram confidence?
4. **Multi-mode**: Run PLL + old locking in parallel for comparison?

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2025-11-23 | AI Agent | Initial functional requirements |

## References

1. ISO/IEC/IEEE 29148:2018 - Requirements engineering
2. StR-002: Advanced BPM Algorithms stakeholder requirements
3. Industry best practices: `docs/beat_detection/1.md`, `2.md`, `3.md`
4. PLL theory: Digital Signal Processing fundamentals
