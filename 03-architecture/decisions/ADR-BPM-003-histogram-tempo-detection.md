# ADR-BPM-003: Histogram-Based Tempo Detection Architecture

**Status**: Proposed  
**Date**: 2025-11-23  
**Decision Maker**: Development Team  
**Issue**: TBD (to be created)

## Context

Current BPM calculation uses simple averaging of normalized intervals with rhythm quantization. While this works for steady patterns, it has limitations:

1. **Outlier sensitivity**: Drum fills with fast rolls or pauses bias the average
2. **No multi-candidate handling**: Single average doesn't represent confidence distribution
3. **Binary decisions**: No gradual confidence assessment

Industry best practices (docs/beat_detection/1.md) suggest histogram-based clustering for robust tempo detection.

## Decision

We will implement a **histogram-based tempo detector** as a pluggable component that can replace or augment the current averaging approach.

### Architecture Components

```
┌─────────────────────────────────────────────────────┐
│         BPMCalculation (Main Controller)            │
├─────────────────────────────────────────────────────┤
│  - addTap()                                         │
│  - calculateBPM()                                   │
└────────────┬───────────────────────────┬────────────┘
             │                           │
             ▼                           ▼
  ┌──────────────────────┐    ┌──────────────────────┐
  │ IntervalNormalizer   │    │ SubdivisionWeighting │
  │ (Existing)           │    │ (New - REQ-F-017)    │
  ├──────────────────────┤    ├──────────────────────┤
  │ - normalize()        │    │ - calculateWeight()  │
  └──────────┬───────────┘    └──────────┬───────────┘
             │                           │
             └──────────┬────────────────┘
                        ▼
             ┌──────────────────────┐
             │ HistogramDetector    │
             │ (New - REQ-F-015)    │
             ├──────────────────────┤
             │ - addInterval()      │
             │ - getTempo()         │
             │ - getConfidence()    │
             │ - getHistogram()     │
             └──────────┬───────────┘
                        │
                        ▼
             ┌──────────────────────┐
             │   PLLTracker         │
             │ (New - REQ-F-016)    │
             ├──────────────────────┤
             │ - addBeat()          │
             │ - getTempo()         │
             │ - isLocked()         │
             └──────────────────────┘
```

### Data Flow

```
Beat Detected (timestamp)
    ↓
addTap(timestamp_us)
    ↓
Calculate interval = current - previous
    ↓
Normalize interval → quarter-note basis
    ↓
Calculate weight (subdivision-based)
    ↓
Convert to BPM candidate = 60M / interval
    ↓
Vote into histogram[bpm_bin] += weight
    ↓
Find peak bin → estimated_tempo
    ↓
Calculate confidence = peak_votes / total_votes
    ↓
Feed to PLL tracker → smooth tracking
    ↓
Output: current_bpm, is_locked
```

### Component Responsibilities

#### HistogramDetector
- **Responsibility**: Accumulate weighted votes, identify peak tempo
- **State**: 140 bins (60-200 BPM), total votes, peak bin
- **Memory**: 576 bytes (140×4 + 16)
- **API**: addInterval(), getTempo(), getConfidence()

#### SubdivisionWeighting
- **Responsibility**: Calculate interval significance weights
- **State**: Reference interval (configurable)
- **Memory**: 8 bytes
- **API**: calculateWeight(interval_us)

#### PLLTracker
- **Responsibility**: Smooth tempo tracking with phase lock
- **State**: Period, phase, gains (alpha, beta), lock status
- **Memory**: 64 bytes
- **API**: addBeat(), getTempo(), isLocked()

## Consequences

### Positive
- ✅ **Robust tempo detection**: Outliers don't bias result
- ✅ **Confidence metric**: Know when tempo estimate is reliable
- ✅ **Smooth tracking**: PLL prevents abrupt jumps
- ✅ **Modular design**: Each component testable independently
- ✅ **Configurable**: Compile-time flags for each feature

### Negative
- ❌ **Increased complexity**: More components to test and maintain
- ❌ **Memory overhead**: +640 bytes total (histogram 576 + PLL 64)
- ❌ **CPU overhead**: +700µs per beat (histogram 500 + PLL 200)
- ❌ **Learning curve**: Team must understand PLL and histogram concepts

### Neutral
- ⚖️ **Fallback option**: Keep existing averaging as compile-time option
- ⚖️ **Incremental adoption**: Can enable features independently
- ⚖️ **Backward compatibility**: Maintain existing API

## Implementation Strategy

### Phase 1: Histogram (REQ-F-015)
1. Create `HistogramDetector` class in `src/bpm/histogram/`
2. Unit tests for histogram voting and peak detection
3. Integration into `BPMCalculation::calculateBPM()`
4. Feature flag: `ENABLE_HISTOGRAM_DETECTOR`

### Phase 2: Weighting (REQ-F-017)
1. Create `SubdivisionWeighting` class in `src/bpm/weighting/`
2. Unit tests for weight calculation
3. Integration into histogram voting
4. Feature flag: `ENABLE_SUBDIVISION_WEIGHTING`

### Phase 3: PLL (REQ-F-016)
1. Create `PLLTracker` class in `src/bpm/pll/`
2. Unit tests for phase/tempo tracking
3. Integration after histogram output
4. Feature flag: `ENABLE_PLL_TRACKER`

### Phase 4: Validation
1. A/B testing with real drum patterns
2. Performance profiling on ESP32
3. Regression testing on existing test suite
4. User acceptance testing

## Alternatives Considered

### Alt 1: Keep Simple Averaging
- **Pros**: Zero overhead, proven to work on steady rhythms
- **Cons**: Doesn't solve fill stability problem
- **Decision**: Rejected - doesn't address identified gaps

### Alt 2: Exponential Moving Average (EMA)
- **Pros**: Simple smoothing, minimal overhead
- **Cons**: Still sensitive to outliers, no confidence metric
- **Decision**: Rejected - not as robust as histogram

### Alt 3: Kalman Filter
- **Pros**: Optimal estimator with confidence bounds
- **Cons**: Complex math, high CPU cost, overkill for this problem
- **Decision**: Rejected - PLL simpler and sufficient

### Alt 4: Autocorrelation / Tempogram
- **Pros**: Academic gold standard, handles complex patterns
- **Cons**: Very high CPU (FFT), large memory, unsuitable for embedded
- **Decision**: Rejected - not feasible on ESP32

## Validation Criteria

- ✅ All existing unit tests pass
- ✅ Histogram accuracy >95% on synthetic test patterns
- ✅ PLL locks within 5 beats on steady rhythms
- ✅ Memory overhead <1KB
- ✅ CPU overhead <10% (700µs / 10ms budget)
- ✅ Tempo stable through 4-beat drum fills

## References

- REQ-F-015: Histogram Tempo Detection
- REQ-F-016: PLL Tempo Tracking
- REQ-F-017: Subdivision Weighting
- Industry best practices: docs/beat_detection/1.md, 2.md, 3.md
- Current implementation: src/bpm/BPMCalculation.cpp

---

**Approved By**: TBD  
**Date**: TBD
