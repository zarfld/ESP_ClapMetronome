# Functional Requirement: Subdivision Down-Weighting

**Requirement ID**: REQ-F-017  
**Issue**: TBD (to be created)  
**Phase**: 02 - Requirements  
**Priority**: Medium  
**Status**: Draft  
**Date**: 2025-11-23  
**Standards**: ISO/IEC/IEEE 29148:2018 (System Requirements)

## 1. Requirement Statement

The BPM calculation system **shall** apply weighting to beat intervals based on their musical significance, giving higher weight to quarter-note timing (primary tempo indicators) and lower weight to fast subdivisions (8ths, 16ths) to prevent hi-hat patterns from biasing tempo calculations.

## 2. Traceability

### Traces From
- **StR-002.3**: Subdivision Down-Weighting
- Current implementation: Equal-weight averaging in `calculateAverageInterval()` lines 189-297

### Traces To
- **ADR-BPM-005**: Architecture decision for weighting strategy
- **DES-C-005**: Subdivision Weighting component design
- **TEST-BPM-017-xx**: Test cases for weighting functionality

## 3. Functional Behavior

### 3.1 Concept Overview

Musically, tempo is defined by the primary beat (quarter notes in 4/4 time). Subdivisions (8th notes, 16th notes) are ornamentation that should not dominate tempo calculation. Weighting gives more influence to longer intervals.

### 3.2 Inputs
- **interval_us** (uint64_t): Normalized inter-onset interval (quarter-note basis)
- **interval_type** (enum): QUARTER, EIGHTH, SIXTEENTH, etc. (derived from normalization)

### 3.3 Processing

**Weight Calculation:**
```
// Reference interval: 500ms (120 BPM quarter note)
REFERENCE_INTERVAL_US = 500000

weight = min(1.0, interval_us / REFERENCE_INTERVAL_US)

// Alternative: Exponential down-weighting
weight = exp(-abs(log2(interval_us / REFERENCE_INTERVAL_US)))
```

**Examples:**
- 500ms (quarter @ 120 BPM): `w = min(1.0, 500000/500000) = 1.0` (full weight)
- 250ms (eighth @ 120 BPM): `w = min(1.0, 250000/500000) = 0.5` (half weight)
- 125ms (sixteenth @ 120 BPM): `w = min(1.0, 125000/500000) = 0.25` (quarter weight)
- 1000ms (half @ 120 BPM): `w = min(1.0, 1000000/500000) = 1.0` (capped at full)

### 3.4 Integration Points

**With Histogram (REQ-F-015):**
```cpp
// When voting into histogram
float weight = calculateWeight(normalized_interval_us);
histogram[bpm_bin] += weight;  // Weighted vote
```

**With Averaging (Current):**
```cpp
// Weighted average calculation
float weighted_sum = 0.0f;
float weight_total = 0.0f;

for (auto interval : normalized_intervals) {
    float w = calculateWeight(interval);
    weighted_sum += (interval * w);
    weight_total += w;
}

average_interval = weighted_sum / weight_total;
```

### 3.5 Outputs
- **weight** (float): 0.0 to 1.0, indicating interval significance

## 4. Acceptance Criteria

### AC-017-001: Quarter Note Priority
**Given** Beat pattern: 4 quarter notes (500ms) + 8 16th notes (125ms)  
**When** Weighted average calculated  
**Then** Result within ±5% of quarter note timing (not biased by 16ths)

**Test**: Unit test with mixed subdivisions
```cpp
// Pattern: Q Q Q Q (500ms each), then 16th×8 (125ms each)
// Unweighted average: (4*500 + 8*125) / 12 = 250ms (wrong!)
// Weighted average: (4*500*1.0 + 8*125*0.25) / (4*1.0 + 8*0.25) = ~417ms
// Expected BPM: ~144 (closer to 120 than 240)
EXPECT_NEAR(weighted_bpm, 144.0f, 5.0f);
```

### AC-017-002: Hi-Hat Pattern Robustness
**Given** 4 snare backbeats (500ms) + 16 hi-hat 16ths (125ms)  
**When** Tempo calculated with weighting  
**Then** Tempo within ±3 BPM of snare tempo (120 BPM)

**Test**: Real drum pattern simulation
```cpp
// Snare every 2 beats: 1000ms intervals
// Hi-hat every 0.5 beats: 250ms intervals
// Expected: Snare dominates tempo → 120 BPM
EXPECT_NEAR(weighted_tempo, 120.0f, 3.0f);
```

### AC-017-003: All-Subdivision Handling
**Given** Pattern with ONLY 16th notes (no quarter notes)  
**When** Weighted tempo calculated  
**Then** System still detects tempo (weights don't suppress all votes)

**Test**: Edge case with only fast subdivisions
```cpp
// Only 16th notes at 120 BPM base (125ms intervals)
// Even with low weights, should detect 120 BPM (after normalization)
EXPECT_NEAR(tempo, 120.0f, 5.0f);
```

### AC-017-004: Weight Formula Consistency
**Given** Various interval lengths (125ms, 250ms, 500ms, 1000ms)  
**When** Weights calculated  
**Then** Weight increases monotonically with interval length (up to cap)

**Test**: Weight function verification
```cpp
float w1 = calculateWeight(125000);   // 16th
float w2 = calculateWeight(250000);   // 8th
float w3 = calculateWeight(500000);   // Quarter
float w4 = calculateWeight(1000000);  // Half

EXPECT_LT(w1, w2);  // 16th < 8th
EXPECT_LT(w2, w3);  // 8th < quarter
EXPECT_LE(w3, 1.0f);  // Capped at 1.0
EXPECT_LE(w4, 1.0f);  // Capped at 1.0
```

### AC-017-005: Performance
**Given** Weight calculation for single interval  
**When** Executed on ESP32  
**Then** Processing time < 10µs

**Test**: Performance micro-benchmark
```cpp
auto start = micros();
float weight = calculateWeight(interval);
auto elapsed = micros() - start;
EXPECT_LT(elapsed, 10);
```

### AC-017-006: Backward Compatibility
**Given** Existing test cases with uniform quarter notes  
**When** Weighting enabled  
**Then** Results unchanged (weights all equal → no effect)

**Test**: Regression with uniform patterns
```cpp
// All intervals = 500ms → all weights = 1.0
// Weighted average == unweighted average
EXPECT_EQ(weighted_bpm, unweighted_bpm);
```

## 5. Non-Functional Requirements

### NFR-017-001: Simplicity
- Weight calculation: Single formula, no branching
- No lookup tables or complex logic
- Easy to tune (single reference interval constant)

### NFR-017-002: Computational Efficiency
- O(1) per interval (no loops)
- Use integer division where possible
- Avoid expensive math (sqrt, exp) in critical path

### NFR-017-003: Tunability
- Reference interval configurable (default 500ms = 120 BPM)
- Weight cap configurable (default 1.0)
- Formula replaceable (linear vs exponential)

## 6. Interface Specification

### 6.1 Public API
```cpp
class SubdivisionWeighting {
public:
    void init(uint64_t reference_interval_us = 500000);
    
    // Calculate weight for given interval
    float calculateWeight(uint64_t normalized_interval_us) const;
    
    // Update reference interval (e.g., based on current tempo)
    void setReferenceInterval(uint64_t reference_us);
    
private:
    uint64_t reference_interval_us_;
    static constexpr float MAX_WEIGHT = 1.0f;
};
```

### 6.2 Integration Point
Apply in histogram voting (REQ-F-015):
```cpp
// In HistogramTempoDetector::addInterval()
float weight = weighting_.calculateWeight(normalized_interval_us);
histogram_[bpm_bin] += weight;  // Weighted vote
```

Or in weighted averaging (current fallback):
```cpp
// In calculateAverageInterval()
float weight = weighting_.calculateWeight(interval);
weighted_sum += (interval * weight);
weight_total += weight;
```

## 7. Design Constraints

### 7.1 Configuration Constants
```cpp
// Reference interval: 500ms (120 BPM quarter note)
static constexpr uint64_t DEFAULT_REFERENCE_INTERVAL_US = 500000;

// Alternative: Make tempo-adaptive
// reference_interval_us = 60000000 / current_locked_bpm
```

### 7.2 Weight Formula Options

**Option A: Linear Ratio (Simple, Fast)**
```cpp
weight = min(1.0f, (float)interval_us / (float)reference_us);
```
- Pros: Simple, no math library, predictable
- Cons: Sharp cutoff at reference interval

**Option B: Exponential Decay (Smooth)**
```cpp
weight = exp(-abs(log2((float)interval_us / (float)reference_us)));
```
- Pros: Smooth falloff, theoretically sound
- Cons: Requires math library, more CPU

**Recommendation**: Start with Option A (linear), upgrade to B if needed.

### 7.3 Feature Flag
```cpp
#define ENABLE_SUBDIVISION_WEIGHTING 1  // Compile-time toggle
```

## 8. Validation Approach

### 8.1 Unit Testing (Native)
- Test weight calculation with various intervals
- Test weighted average vs unweighted
- Test edge cases (zero weight, max weight, very long intervals)

### 8.2 Integration Testing (ESP32)
- Test with synthetic drum patterns (mixed subdivisions)
- Test hi-hat + snare combinations
- Test with real recorded audio patterns

### 8.3 A/B Comparison
- Record tempo accuracy with and without weighting
- Compare on patterns with heavy hi-hat presence
- Measure false positives (tempo jumps) reduction

## 9. Dependencies

### 9.1 Prerequisites
- IOI normalization functional (Phase 05)
- Subdivision detection working (ratios 0.5×, 0.25×, etc.)

### 9.2 Related Requirements
- **REQ-F-015**: Histogram Tempo Detection (uses weights for voting)
- **REQ-F-002**: BPM Calculation (may use weighted averaging)

## 10. Risks and Mitigation

| Risk | Impact | Mitigation |
|------|--------|------------|
| Over-suppression of 16ths → missed tempo | Medium | Ensure minimum weight (e.g., 0.1) to keep all votes counted |
| Reference interval wrong for different tempos | Low | Make tempo-adaptive or use multiple references |
| Formula doesn't improve accuracy | Medium | A/B test with real data, keep unweighted as fallback |

## 11. Success Metrics

- **Accuracy**: Tempo within ±2 BPM on mixed subdivision patterns
- **Robustness**: Hi-hat 16ths don't shift tempo >3 BPM from backbeat
- **Performance**: Weight calculation <10µs per interval
- **Simplicity**: <20 lines of code for weight function

## 12. Tuning Guidelines

### 12.1 Reference Interval Selection
- **Fixed (500ms)**: Works for 80-160 BPM range
- **Tempo-adaptive**: `reference = 60000000 / current_bpm` (tracks tempo)
- **Hybrid**: Fixed during tracking, adaptive when locked

### 12.2 Weight Formula Tuning
Test with real drum patterns:
1. Record accuracy with linear formula
2. If issues, try exponential decay
3. Adjust reference interval if needed
4. Compare against unweighted baseline

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2025-11-23 | AI Agent | Initial functional requirements |

## References

1. ISO/IEC/IEEE 29148:2018 - Requirements engineering
2. StR-002: Advanced BPM Algorithms stakeholder requirements
3. Industry best practices: `docs/beat_detection/1.md`
4. Music theory: Rhythmic hierarchy and beat salience
