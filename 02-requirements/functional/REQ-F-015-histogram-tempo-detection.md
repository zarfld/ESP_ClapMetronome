# Functional Requirement: Histogram-Based Tempo Detection

**Requirement ID**: REQ-F-015  
**Issue**: TBD (to be created)  
**Phase**: 02 - Requirements  
**Priority**: High  
**Status**: Draft  
**Date**: 2025-11-23  
**Standards**: ISO/IEC/IEEE 29148:2018 (System Requirements)

## 1. Requirement Statement

The BPM calculation system **shall** implement histogram-based tempo clustering to robustly identify the dominant tempo pattern from beat intervals, even in the presence of outliers from drum fills, pauses, or timing irregularities.

## 2. Traceability

### Traces From
- **StR-002.1**: Robust Tempo Detection (Histogram)
- Current implementation: `calculateAverageInterval()` in `BPMCalculation.cpp` lines 189-297

### Traces To
- **ADR-BPM-003**: Architecture decision for histogram implementation
- **DES-C-003**: Histogram Tempo Detector component design
- **TEST-BPM-015-xx**: Test cases for histogram functionality

## 3. Functional Behavior

### 3.1 Inputs
- Normalized beat intervals (uint64_t microseconds, quarter-note basis)
- Beat count (uint8_t, 2-64 beats)
- Current tempo estimate (float BPM, optional)

### 3.2 Processing

**Step 1: Interval Normalization**
- Each raw IOI normalized to quarter-note basis (existing logic)
- Subdivisions (½, ¼, ⅛) and multiples (2×) folded to base tempo
- Triplets (⅔, ⅓) normalized with 15% tolerance

**Step 2: BPM Conversion**
```
For each normalized_interval:
    bpm_candidate = 60,000,000 / normalized_interval
    IF bpm_candidate < 60 BPM THEN
        bpm_candidate = bpm_candidate * 2
    ELSE IF bpm_candidate > 200 BPM THEN
        bpm_candidate = bpm_candidate / 2
    END IF
```

**Step 3: Histogram Voting**
- Create histogram: 140 bins covering 60-200 BPM (1 BPM resolution)
- Each BPM candidate votes into nearest bin
- Apply weight: `w = min(1.0, interval / 500000µs)` (see REQ-F-017)
- Accumulate: `histogram[bin] += weight`

**Step 4: Peak Detection**
```
max_bin = 0
max_votes = 0.0
FOR bin IN 60..200 BPM:
    IF histogram[bin] > max_votes THEN
        max_votes = histogram[bin]
        max_bin = bin
    END IF
END FOR
tempo_estimate = max_bin
```

**Step 5: Confidence Calculation**
```
total_votes = SUM(histogram[all bins])
confidence = max_votes / total_votes * 100%
```

### 3.3 Outputs
- **tempo_bpm** (float): Peak histogram bin (60-200 BPM)
- **confidence** (float): Peak votes / total votes (0-100%)
- **histogram_state** (internal): Persisted for next iteration

## 4. Acceptance Criteria

### AC-015-001: Histogram Construction
**Given** 10+ normalized beat intervals  
**When** histogram is built  
**Then** each interval votes into correct BPM bin (±1 BPM tolerance)

**Test**: Unit test with synthetic intervals at 120 BPM
```cpp
// 10 intervals at exactly 500ms (120 BPM)
EXPECT_HISTOGRAM_PEAK(120, confidence > 95%);
```

### AC-015-002: Outlier Resistance
**Given** 12 intervals: 10 at 120 BPM, 2 outliers at 60 BPM  
**When** histogram built  
**Then** peak identifies 120 BPM (outliers don't dominate)

**Test**: Unit test with 83% inliers, 17% outliers
```cpp
// Expect: 120 BPM peak with ~70% confidence
EXPECT_EQ(tempo, 120.0f);
EXPECT_GT(confidence, 65.0f);
```

### AC-015-003: Fill Stability
**Given** Real drum pattern: 8 beats at 90 BPM + 4-beat fill with 16ths  
**When** histogram calculated after each beat  
**Then** tempo stays within ±3 BPM of 90 during fill

**Test**: Integration test with recorded fill pattern
```cpp
// Pattern: 500,500,500,500,500,500,500,500 (8 beats)
//          125,125,125,125 (fill with 16ths, normalized to 500)
EXPECT_NEAR(tempo, 90.0f, 3.0f);  // Throughout sequence
```

### AC-015-004: Bin Resolution
**Given** Intervals clustered around 120.5 BPM  
**When** histogram built with 1 BPM bins  
**Then** votes distributed between bins 120 and 121, peak identifies nearest

**Test**: Fractional BPM handling
```cpp
// Intervals averaging 120.5 BPM
EXPECT_TRUE(tempo == 120.0f || tempo == 121.0f);
```

### AC-015-005: Memory Budget
**Given** Histogram implementation compiled  
**When** memory usage analyzed  
**Then** histogram state ≤ 560 bytes (140 bins × 4 bytes float)

**Test**: Static memory analysis
```cpp
EXPECT_LE(sizeof(HistogramState), 560);
```

### AC-015-006: Performance
**Given** 32 beat intervals in buffer  
**When** histogram calculation executed on ESP32  
**Then** total processing time < 500µs per beat

**Test**: Performance profiling on hardware
```cpp
auto start = micros();
histogram.calculate();
auto elapsed = micros() - start;
EXPECT_LT(elapsed, 500);
```

### AC-015-007: Backward Compatibility
**Given** Existing test cases from Phase 05  
**When** histogram replaces averaging  
**Then** all existing tests pass (stable inputs unaffected)

**Test**: Regression suite
```cpp
// Run all TDD-CYCLE-BPM-* tests with histogram enabled
EXPECT_ALL_PASS();
```

## 5. Non-Functional Requirements

### NFR-015-001: Real-time Performance
- Histogram update: O(n) where n = beat count ≤ 32
- Peak detection: O(k) where k = 140 bins
- Total complexity: O(n + k) ≈ 172 operations
- Target: < 500µs on ESP32 @ 240MHz

### NFR-015-002: Memory Efficiency
- Static allocation only (no malloc/new)
- Histogram: 140 bins × 4 bytes = 560 bytes
- Metadata: 16 bytes (bin_count, max_bin, confidence, etc.)
- Total: 576 bytes ≤ 1KB budget

### NFR-015-003: Numerical Stability
- Use float for histogram bins (sufficient precision for vote counts)
- Use uint64_t for interval arithmetic (no precision loss)
- Avoid division in inner loop (pre-compute bin width)

## 6. Interface Specification

### 6.1 Public API
```cpp
class HistogramTempoDetector {
public:
    void init();
    void reset();
    
    // Add normalized interval, returns true if histogram updated
    bool addInterval(uint64_t normalized_interval_us);
    
    // Get current tempo estimate
    float getTempo() const;
    float getConfidence() const;
    
    // Get histogram data (for telemetry/debugging)
    const float* getHistogram() const;
    uint8_t getBinCount() const;
};
```

### 6.2 Integration Point
Replace `calculateAverageInterval()` in `BPMCalculation.cpp`:
```cpp
// OLD:
state_.average_interval_us = calculateAverageInterval();

// NEW:
histogram_detector_.addInterval(normalized_interval);
float histogram_bpm = histogram_detector_.getTempo();
float confidence = histogram_detector_.getConfidence();
```

## 7. Design Constraints

### 7.1 Configuration Constants
```cpp
static constexpr uint8_t HISTOGRAM_MIN_BPM = 60;
static constexpr uint8_t HISTOGRAM_MAX_BPM = 200;
static constexpr uint8_t HISTOGRAM_BIN_COUNT = 140;  // 1 BPM resolution
static constexpr float HISTOGRAM_MIN_CONFIDENCE = 0.50f;  // 50% threshold
```

### 7.2 Feature Flag
```cpp
#define ENABLE_HISTOGRAM_DETECTOR 1  // Compile-time toggle
```

## 8. Validation Approach

### 8.1 Unit Testing (Native)
- Test histogram construction with synthetic intervals
- Test outlier rejection (10% outliers)
- Test confidence calculation
- Test edge cases (min/max BPM, empty histogram)

### 8.2 Integration Testing (ESP32)
- Test with real drum patterns (recorded audio → beat timestamps)
- Test fill sequences (8 steady + 4 fill beats)
- Test tempo changes (gradual acceleration/deceleration)

### 8.3 Performance Testing
- Profile CPU usage with ESP32 cycle counter
- Measure memory allocation (static analysis)
- Stress test with maximum beat buffer (64 beats)

## 9. Dependencies

### 9.1 Prerequisites
- IOI normalization working correctly (Phase 05 validated)
- Beat detection providing accurate timestamps (Phase 28 fix)
- Tap circular buffer functional (`BPMCalculationState`)

### 9.2 Related Requirements
- **REQ-F-016**: PLL Tempo Tracking (histogram output feeds PLL)
- **REQ-F-017**: Subdivision Weighting (weights applied during voting)
- **REQ-F-002**: Existing BPM calculation (histogram replaces averaging)

## 10. Risks and Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| Histogram doesn't improve accuracy | Medium | A/B testing with real patterns, keep averaging as fallback |
| CPU overhead causes audio glitches | High | Optimize inner loop, profile on hardware, add performance gate |
| Bin resolution too coarse for fine tempo | Low | Configurable bin size, interpolation for sub-bin precision |
| Edge case: all votes in one bin | Low | Add minimum spread check, fall back to averaging |

## 11. Success Metrics

- **Accuracy**: Tempo within ±2 BPM on synthetic test patterns (>95% of beats)
- **Robustness**: Handles 20% outlier rate without tempo jump
- **Performance**: <500µs per beat on ESP32
- **Memory**: ≤576 bytes allocated
- **Regression**: Zero failures on existing test suite

## 12. Open Issues

1. **Gaussian blur**: Should histogram bins be blurred (±1-2 BPM) for smoother peak?
2. **Confidence threshold**: Is 50% the right minimum confidence to publish tempo?
3. **Bin interpolation**: Should peak be interpolated from neighboring bins for sub-BPM precision?
4. **Integration**: Replace averaging completely or run in parallel (A/B mode)?

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2025-11-23 | AI Agent | Initial functional requirements |

## References

1. ISO/IEC/IEEE 29148:2018 - Requirements engineering
2. StR-002: Advanced BPM Algorithms stakeholder requirements
3. Industry best practices: `docs/beat_detection/1.md`
4. Current implementation: `src/bpm/BPMCalculation.cpp`
