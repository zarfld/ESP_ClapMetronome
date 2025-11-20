# Wave 2.2: BPM Calculation Engine - Implementation Kickoff

**Component**: DES-C-002 - BPM Calculation Engine  
**GitHub Issue**: #46  
**Phase**: 05 - Implementation  
**Standard**: ISO/IEC/IEEE 12207:2017  
**XP Practice**: Test-Driven Development (TDD)  
**Start Date**: 2025-11-20  
**Status**: 🚀 **STARTED** - RED Phase  

---

## 🎯 Component Overview

**Purpose**: Calculate beats per minute (BPM) from beat event timestamps using circular buffer averaging with tempo validation.

**Key Specifications** (from Phase 04):
- **Algorithm**: Circular buffer BPM calculation with tempo validation
- **Interfaces**: DES-I-006 (BPM Update), DES-I-007 (Tap Addition)
- **Data Models**: DES-D-003 (Tap Circular Buffer, 64 × 8B = 524B)
- **Performance**: <5ms calculation, <2% CPU, 572B RAM
- **Algorithms**:
  - BPM: Average interval conversion `60,000,000 µs / avg_interval`
  - Stability: Coefficient of Variation (CV < 5%)
  - Half/double detection: 5 consecutive intervals ~2× or ~0.5× average

---

## 📋 Acceptance Criteria (from TDD Plan)

| ID | Criterion | Test Method | Pass Condition |
|----|-----------|-------------|----------------|
| AC-BPM-001 | Single tap returns 0 | Unit test | getBPM() = 0 |
| AC-BPM-002 | 4 taps at 120 BPM | Unit test | getBPM() = 120.0 ± 0.5 |
| AC-BPM-003 | 64 taps at 140 BPM | Unit test | getBPM() = 140.0 ± 0.5 |
| AC-BPM-004 | Half-tempo detection | Unit test | 5 intervals ~2× avg → BPM /= 2 |
| AC-BPM-005 | Double-tempo detection | Unit test | 5 intervals ~0.5× avg → BPM *= 2 |
| AC-BPM-006 | Stability flag | Unit test | CV < 5% → stable = true |
| AC-BPM-007 | Circular buffer wrap | Unit test | 65th tap overwrites oldest |
| AC-BPM-008 | Integration with Audio | Integration test | Real beats → accurate BPM |
| AC-BPM-009 | Calculation latency | Performance test | <5ms |
| AC-BPM-010 | Memory footprint | Performance test | <600B RAM |
| AC-BPM-011 | CPU usage | Performance test | <2% average |
| AC-BPM-012 | Clear/reset | Unit test | clear() resets to initial state |
| AC-BPM-013 | Invalid interval handling | Unit test | Intervals <100ms or >2000ms ignored |
| AC-BPM-014 | BPM update callbacks | Unit test | Callbacks fired on BPM change |

---

## 🔄 TDD Implementation Plan

### TDD Cycle 1: Basic Tap Addition (AC-BPM-001, AC-BPM-002)
**Status**: 🔴 RED - Starting now  
**Tests**: 
- Single tap returns BPM = 0
- 4 taps at 120 BPM returns 120.0 ± 0.5

### TDD Cycle 2: Circular Buffer (AC-BPM-003, AC-BPM-007)
**Status**: ⏳ Pending  
**Tests**:
- 64 taps fill buffer
- 65th tap wraps around

### TDD Cycle 3: Stability Detection (AC-BPM-006)
**Status**: ⏳ Pending  
**Tests**:
- CV < 5% sets stable = true
- CV ≥ 5% sets stable = false

### TDD Cycle 4: Half/Double Tempo Correction (AC-BPM-004, AC-BPM-005)
**Status**: ⏳ Pending  
**Tests**:
- 5 consecutive 2× intervals halves BPM
- 5 consecutive 0.5× intervals doubles BPM

### TDD Cycle 5: Invalid Interval Filtering (AC-BPM-013)
**Status**: ⏳ Pending  
**Tests**:
- Intervals <100ms rejected
- Intervals >2000ms rejected

### TDD Cycle 6: Clear/Reset (AC-BPM-012)
**Status**: ⏳ Pending  
**Tests**:
- clear() resets tap count to 0
- clear() resets BPM to 0

### TDD Cycle 7: Callback Notifications (AC-BPM-014)
**Status**: ⏳ Pending  
**Tests**:
- onBPMUpdate callback fired
- Callback receives correct BPM value

### Integration Test: Audio → BPM (AC-BPM-008)
**Status**: ⏳ Pending (after Wave 2.2 complete)  

### Performance Tests: Latency, Memory, CPU (AC-BPM-009, AC-BPM-010, AC-BPM-011)
**Status**: ⏳ Pending (after functional tests complete)  

---

## 📊 Estimated Effort

- **TDD Cycles**: 7 cycles × 30 min = 3.5 hours
- **Integration Test**: 1 hour
- **Performance Tests**: 1 hour
- **Documentation**: 30 min
- **Total**: ~6 hours

---

## 📁 Files to Create

### Source Files
1. `src/bpm/BPMCalculationState.h` - DES-D-003 data model
2. `src/bpm/BPMCalculation.h` - Component header
3. `src/bpm/BPMCalculation.cpp` - Implementation

### Test Files
1. `test/test_bpm/test_basic_taps.cpp` - AC-BPM-001, 002
2. `test/test_bpm/test_circular_buffer.cpp` - AC-BPM-003, 007
3. `test/test_bpm/test_stability.cpp` - AC-BPM-006
4. `test/test_bpm/test_tempo_correction.cpp` - AC-BPM-004, 005
5. `test/test_bpm/test_invalid_intervals.cpp` - AC-BPM-013
6. `test/test_bpm/test_clear_reset.cpp` - AC-BPM-012
7. `test/test_bpm/test_callbacks.cpp` - AC-BPM-014

---

## 🔗 Traceability

**Requirements**: REQ-F-002 (#3) - Calculate BPM from tap timestamps  
**Architecture**: ARC-C-002 (#22) - BPM Calculation (tap buffer)  
**Design**: DES-C-002 (#46) - BPM Calculation Engine  
**Interfaces**: 
- DES-I-006: BPM Update Interface
- DES-I-007: Tap Addition Interface
**Data Models**: DES-D-003 - Tap Circular Buffer  

---

## 🚀 Next Steps

1. ✅ Create directory structure (`src/bpm/`, `test/test_bpm/`)
2. ✅ Create kickoff document (this file)
3. 🔴 Start TDD Cycle 1: Write failing test for basic tap addition
4. 🟢 Implement minimal code to pass test
5. 🔵 Refactor for quality

---

**Status**: Ready to begin TDD Cycle 1  
**Next Action**: Create `test/test_bpm/test_basic_taps.cpp` with RED test
