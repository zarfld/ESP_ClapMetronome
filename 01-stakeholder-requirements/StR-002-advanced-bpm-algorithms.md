# Stakeholder Requirements: Advanced BPM Detection Algorithms

**Document ID**: StR-002  
**Issue**: TBD (to be created)  
**Phase**: 01 - Stakeholder Requirements  
**Status**: Draft  
**Date**: 2025-11-23  
**Standards**: ISO/IEC/IEEE 29148:2018 (Stakeholder Requirements)

## 1. Introduction

### 1.1 Purpose
Define stakeholder needs for enhancing the BPM detection system with advanced algorithms to improve tempo accuracy, stability, and responsiveness to tempo changes in real-world drumming scenarios.

### 1.2 Context
Current implementation (Phase 28-29 fixes) provides:
- ✅ IOI normalization with subdivision detection
- ✅ CV-based stability measurement (12% threshold)
- ✅ Tempo locking with deviation counting
- ✅ Half/double tempo correction

**Identified gaps** from analysis of industry best practices (docs/beat_detection/*.md):
- ❌ Histogram-based tempo clustering (more robust against outliers)
- ❌ Phase-Locked Loop (PLL) tracking (smooth tempo transitions)
- ❌ Weighted voting (down-weight hi-hat subdivisions)

### 1.3 Scope
This document covers stakeholder requirements for three enhancement areas:
1. **Histogram-based tempo detection** (REQ-F-015)
2. **PLL tempo tracking** (REQ-F-016)
3. **Subdivision weighting** (REQ-F-017)

## 2. Stakeholder Identification

| Stakeholder | Role | Interest |
|------------|------|----------|
| Musicians/Drummers | Primary Users | Accurate tempo detection during complex rhythms |
| Practice Session Users | End Users | Stable metronome that doesn't jump during fills |
| Live Performance Users | End Users | Quick tempo lock, smooth tempo changes |
| Development Team | Implementers | Clear requirements, testable acceptance criteria |

## 3. Business Context

### 3.1 Problem Statement
**Current System Limitations:**
1. **Tempo jumps during drum fills**: Current averaging can be biased by outlier intervals
2. **Hi-hat dominance**: Fast subdivisions (16ths) weighted equally with quarter notes
3. **Abrupt tempo transitions**: No smooth tracking through tempo changes
4. **Multi-tempo ambiguity**: No mechanism to identify strongest tempo candidate

### 3.2 Opportunity
Industry-standard algorithms (documented in attached research) provide:
- **Histogram clustering**: Robust tempo estimation even with outliers
- **PLL tracking**: Musical "groove lock" that rides through messy sections
- **Weighted voting**: Prioritizes musically significant beats over subdivisions

### 3.3 Success Criteria
- Tempo remains stable during drum fills (4+ beat fills)
- Tempo locks within 8-12 beats (current: 8+ beats)
- Smooth tempo transitions (no abrupt jumps >10 BPM)
- Hi-hat 16ths don't bias tempo calculation
- Memory overhead < 1KB additional RAM
- CPU overhead < 10% additional processing time

## 4. Stakeholder Requirements

### StR-002.1: Robust Tempo Detection (Histogram)
**Priority**: High  
**Stakeholder**: Musicians, Live Performance Users

**Need**: The system shall detect tempo robustly even when drum fills create outlier intervals that would bias simple averaging.

**Rationale**: 
- Current averaging treats all normalized intervals equally
- Drum fills with fast rolls or pauses create outliers
- Histogram voting identifies dominant tempo pattern

**Acceptance Criteria**:
- Tempo remains within ±3 BPM during 4+ beat fills
- System identifies correct tempo when 70%+ of beats match pattern
- Outliers (fills, pauses) don't cause >10 BPM jumps

**Traces to**: Current system's averaging in `calculateAverageInterval()`

---

### StR-002.2: Smooth Tempo Tracking (PLL)
**Priority**: High  
**Stakeholder**: Musicians, Practice Session Users

**Need**: The system shall track tempo changes smoothly without abrupt jumps, maintaining musical "groove lock" through imperfect timing.

**Rationale**:
- Current locking is binary: locked or unlocked
- Real tempo changes are gradual (ritardando, accelerando)
- PLL provides phase and frequency tracking like human perception

**Acceptance Criteria**:
- Tempo changes transition smoothly (max 2 BPM per beat)
- System maintains phase lock through single missed/extra beats
- No tempo jumps >5 BPM between consecutive calculations
- Gradual tempo changes (±10 BPM over 8 beats) tracked accurately

**Traces to**: Current locking logic in `calculateBPM()`

---

### StR-002.3: Subdivision Down-Weighting
**Priority**: Medium  
**Stakeholder**: Musicians (hi-hat players), Practice Session Users

**Need**: The system shall prioritize quarter-note timing over fast subdivisions (8ths, 16ths) when calculating tempo.

**Rationale**:
- Hi-hat 16th notes create many short intervals
- Current normalization treats all intervals equally after normalization
- Musically, quarter notes define tempo; subdivisions are ornamentation

**Acceptance Criteria**:
- Hi-hat 16th note patterns (4+ consecutive) don't shift tempo >2 BPM
- Quarter note intervals have 2-4× weight of 16th notes
- System still detects tempo from 16ths-only when no quarter notes present
- Weight formula: `w = min(1.0, IOI / 0.5s)` or similar

**Traces to**: Current equal-weight handling in `calculateAverageInterval()`

---

### StR-002.4: Memory Efficiency
**Priority**: High  
**Stakeholder**: Development Team, System Architects

**Need**: Advanced algorithms shall not exceed 1KB additional RAM allocation on ESP32 platform.

**Rationale**:
- Current system: ~544 bytes for BPM state + 512 bytes shadow tracker
- ESP32 RAM: 320KB total, current usage 7.7% (25KB)
- Must leave headroom for future features

**Acceptance Criteria**:
- Histogram implementation: ≤512 bytes (128 bins × 4 bytes)
- PLL state: ≤64 bytes (phase, period, error accumulators)
- Total additional RAM: ≤1KB
- Flash size increase: ≤8KB

**Traces to**: Memory budget in `BPMCalculationState.h`

---

### StR-002.5: CPU Performance
**Priority**: High  
**Stakeholder**: Development Team, Real-time Performance

**Need**: Advanced algorithms shall not increase processing time by more than 10% per beat.

**Rationale**:
- Current processing: <1ms per beat on ESP32 @ 240MHz
- Audio detection runs at 40kHz sample rate (tight timing)
- Must maintain real-time responsiveness

**Acceptance Criteria**:
- Histogram update: <500µs per beat
- PLL calculation: <200µs per beat
- Total BPM calculation: <2ms per beat
- No blocking operations during beat processing

**Traces to**: Performance requirements in system design

---

### StR-002.6: Backward Compatibility
**Priority**: High  
**Stakeholder**: Current Users, Development Team

**Need**: Advanced algorithms shall maintain or improve current accuracy on existing test cases.

**Rationale**:
- Current system passes all Phase 05 TDD tests
- Users rely on existing behavior
- Improvements must not break working features

**Acceptance Criteria**:
- All existing unit tests pass without modification
- Current CV-based stability (12% threshold) preserved as fallback
- Locked tempo behavior unchanged (3s timeout, deviation counting)
- Serial telemetry format backward compatible

**Traces to**: Existing test suite in `test/native/`

## 5. Constraints

### 5.1 Technical Constraints
- **Platform**: ESP32-D0WD-V3, 240MHz, 320KB RAM, 1.3MB Flash
- **Language**: C++11 (PlatformIO Arduino framework)
- **Real-time**: Audio processing at 40kHz, BPM updates at beat rate (1-4 Hz)
- **Precision**: 64-bit microsecond timestamps (no floating-point timing)

### 5.2 Design Constraints
- **Modularity**: Algorithms must be toggleable (compile-time flags)
- **Testability**: Must support native unit testing (no ESP32 hardware required)
- **Standards**: Follow ISO/IEC/IEEE 12207:2017 implementation process
- **XP Practices**: TDD (Red-Green-Refactor), continuous integration

### 5.3 Regulatory Constraints
- None (consumer music device)

## 6. Assumptions and Dependencies

### 6.1 Assumptions
- Current IOI normalization works correctly (Phase 05 validated)
- Beat detection provides accurate timestamps (Phase 28 fix deployed)
- ESP32 clock stable (no significant drift during session)

### 6.2 Dependencies
- **Phase 05**: Current BPM implementation tested and stable
- **Phase 28**: Audio detection window initialization fixed
- **Phase 29**: CV threshold relaxed to 12%
- **Research**: Algorithm descriptions in `docs/beat_detection/*.md`

## 7. Risks

| Risk | Impact | Mitigation |
|------|--------|------------|
| Increased complexity breaks existing features | High | Comprehensive regression testing, feature flags |
| CPU overhead causes audio glitches | High | Performance profiling, optimize critical paths |
| Memory allocation exceeds budget | Medium | Static allocation only, validate at compile time |
| PLL instability on tempo changes | Medium | Extensive testing with real drumming patterns |
| Histogram bins don't cover edge cases | Low | Configurable bin size and range |

## 8. Verification Approach

### 8.1 Testing Strategy
1. **Unit Tests**: Each algorithm component (histogram, PLL, weighting)
2. **Integration Tests**: Combined algorithm behavior
3. **Performance Tests**: CPU/memory profiling on ESP32
4. **Real-world Tests**: Recorded drum patterns with known BPM

### 8.2 Acceptance Testing
- Drum fill test: 16-beat pattern with 4-beat fill
- Hi-hat test: 16th notes + quarter notes mix
- Tempo change test: Gradual acceleration/deceleration
- Edge cases: Very slow (40 BPM), very fast (180 BPM)

## 9. Traceability

### Traces From
- Current implementation analysis (Phase 28-29)
- Industry best practices (docs/beat_detection/1.md, 2.md, 3.md)
- User feedback (phantom beats, tempo locking issues)

### Traces To
- **Phase 02**: Functional requirements (REQ-F-015, REQ-F-016, REQ-F-017)
- **Phase 03**: Architecture decisions (ADR-BPM-003, ADR-BPM-004)
- **Phase 04**: Detailed design (DES-C-003: Histogram, DES-C-004: PLL)

## 10. Approval

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Product Owner | TBD | 2025-11-23 | Pending |
| Lead Developer | AI Agent | 2025-11-23 | Draft |
| Stakeholder Rep | TBD | 2025-11-23 | Pending |

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 0.1 | 2025-11-23 | AI Agent | Initial draft based on gap analysis |

---

## References

1. ISO/IEC/IEEE 29148:2018 - Systems and software engineering - Life cycle processes - Requirements engineering
2. ESP_ClapMetronome Implementation (Phase 05, 28, 29)
3. Industry best practices: `docs/beat_detection/1.md`, `2.md`, `3.md`
4. Current implementation: `src/bpm/BPMCalculation.cpp`, `BPMCalculationState.h`
