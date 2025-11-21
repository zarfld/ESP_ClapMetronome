# Traceability Linkage Action Plan

**Date**: 2025-11-21  
**Status**: 🚨 **CRITICAL** - CI Failing (22.22% coverage, needs 90%)  
**Issue**: GitHub Actions validation failing - no traceability.json + missing GitHub Issues

---

## 🎯 Current Problem

GitHub Actions CI is failing with:
```
❌ Requirements overall coverage 22.22% < 90.00%
❌ ADR linkage coverage 0.00% < 70.00%
❌ Scenario linkage coverage 0.00% < 60.00%
❌ Test linkage coverage 0.00% < 40.00%
Error: Process completed with exit code 5.
```

**Root Causes**:
1. **No `build/traceability.json`** - File doesn't exist (spec-generation job failed/skipped)
2. **No GitHub Issues** - Requirements/ADRs/Tests not tracked as issues
3. **No issue linkage** - Existing docs don't reference GitHub issue numbers
4. **Missing features** - WebServer, MIDI, RTP MIDI, HTTP API, MQTT not implemented

---

## 📊 Current State Assessment

### Existing Documentation

| Phase | Files | Issue Linkage | Status |
|-------|-------|---------------|--------|
| **01-stakeholder-requirements** | 1 file (just created) | No issues yet | ⚠️ NEW |
| **02-requirements/functional** | 1 file (MQTT) | No `#N` refs | ❌ NO LINKAGE |
| **03-architecture/decisions** | ~8 ADR files | Some `#N` refs in old docs | ⚠️ PARTIAL |
| **03-architecture/components** | ~7 ARC-C files | No `#N` refs | ❌ NO LINKAGE |
| **04-design** | Multiple design docs | No `#N` refs | ❌ NO LINKAGE |
| **05-implementation** | Code in `src/` | No `Implements: #N` comments | ❌ NO LINKAGE |
| **test/** | 403 tests passing | No `Verifies: #N` comments | ❌ NO LINKAGE |

### Test Results (Hardware Working ✅)

```
✅ 403/403 tests passing (100%)
✅ Native: 366 tests
✅ ESP32 Baseline: 9 tests
✅ DS3231 RTC: 9 tests
✅ MAX9814 Mic: 11 tests
✅ Combined Hardware: 8 tests
```

**BUT**: Tests don't reference requirement issues!

---

## 🛠️ Solution Strategy

### Approach A: GitHub Issues Workflow (Standards-Compliant) ⭐ RECOMMENDED

**Advantages**:
- ✅ Follows ISO/IEC/IEEE 29148 traceability requirements
- ✅ Enables automated validation
- ✅ Better collaboration (team can track/review/approve)
- ✅ Integrates with GitHub Projects, CI/CD

**Steps**:
1. **Create Stakeholder Requirement Issues** (StR-001, etc.)
   - Use GitHub Issue templates
   - Labels: `type:stakeholder-requirement`, `phase:01`, `priority:p0`
   
2. **Create Functional Requirement Issues** (REQ-F-001 to REQ-F-010)
   - Template: `type:requirement:functional`, `phase:02`
   - Add `Traces to: #1` (StR-001 issue)
   
3. **Create Architecture Decision Issues** (ADR-001 to ADR-010)
   - Template: `type:architecture-decision`, `phase:03`
   - Add `Satisfies: #2, #3` (REQ-F issues)
   
4. **Create Test Case Issues** (TEST-001 to TEST-050)
   - Template: `type:test-case`, `phase:07`
   - Add `Verifies: #2` (REQ-F issue)
   
5. **Add Issue References to Code**
   ```cpp
   /**
    * Audio detection module
    * Implements: #2 (REQ-F-001: Tempo Detection)
    * Architecture: #15 (ADR-001: Use FFT for frequency analysis)
    * Verified by: #50 (TEST-AUDIO-001)
    */
   class AudioDetection {
   ```

6. **Add Issue References to Tests**
   ```cpp
   /**
    * Test beat detection accuracy
    * Verifies: #2 (REQ-F-001: Tempo Detection)
    * Acceptance Criteria: Detect beats within 20ms latency
    */
   void test_beat_detection_latency() {
   ```

7. **Generate traceability.json**
   ```bash
   python scripts/build-traceability.py
   python scripts/validate-trace-coverage.py --min-req 90
   ```

**Effort**: ~8-10 hours (create 50-100 issues with proper linkage)

---

### Approach B: Document-Only (Quick Fix, Not Standards-Compliant) ❌ NOT RECOMMENDED

**Disadvantages**:
- ❌ Doesn't satisfy ISO/IEC/IEEE 29148 requirements
- ❌ Can't validate traceability automatically
- ❌ Manual maintenance nightmare (stale docs)
- ❌ No CI integration

**Would require**:
- Create fake issues or use doc IDs
- Modify validation scripts to accept doc refs
- Lose automation benefits

---

## 📋 Recommended Immediate Actions

### Priority 1: Create Core GitHub Issues (TODAY)

**Stakeholder Requirements** (1 issue):
- [ ] #1: StR-001: Clap-Based Metronome System (✅ DONE - doc created)

**Functional Requirements** (10 issues):
- [ ] #2: REQ-F-001: Tempo Detection from Acoustic Signals
- [ ] #3: REQ-F-002: BPM Calculation and Smoothing
- [ ] #4: REQ-F-003: Lock Status Detection (LOCKED/UNLOCKING/UNLOCKED)
- [ ] #5: REQ-F-004: LED Beat Output
- [ ] #6: REQ-F-005: Web Server HTTP API (`/api/config`, `/api/status`)
- [ ] #7: REQ-F-006: Web UI Dashboard (http://device-ip/)
- [ ] #8: REQ-F-007: MQTT Telemetry Publishing (✅ DOC EXISTS)
- [ ] #9: REQ-F-008: MIDI Beat Clock Output (24 PPQ)
- [ ] #10: REQ-F-009: RTP MIDI Support (network MIDI)
- [ ] #11: REQ-F-010: Configuration Management (persist settings)

**Architecture Decisions** (5 issues):
- [ ] #12: ADR-001: Use State Machine for Beat Detection
- [ ] #13: ADR-002: Use DS3231 RTC for Timestamps
- [ ] #14: ADR-003: Use ESPAsyncWebServer for HTTP
- [ ] #15: ADR-004: Use 256dpi/MQTT Library
- [ ] #16: ADR-005: Use RTP MIDI for Network Synchronization

**Test Cases** (5 representative issues):
- [ ] #17: TEST-AUDIO-001: Beat Detection Latency (<20ms)
- [ ] #18: TEST-BPM-001: BPM Calculation Accuracy (±0.5 BPM)
- [ ] #19: TEST-WEB-001: HTTP API Endpoints (/api/config, /api/status)
- [ ] #20: TEST-MQTT-001: Telemetry Publishing (5s interval)
- [ ] #21: TEST-MIDI-001: Beat Clock Accuracy (24 PPQ, <1ms jitter)

---

### Priority 2: Add Traceability Links to Existing Code (TOMORROW)

**Files to Update** (~30 files):

1. **src/audio/AudioDetection.cpp** (280 lines)
   ```cpp
   /**
    * Implements: #2 (REQ-F-001: Tempo Detection)
    * Architecture: #12 (ADR-001: State Machine)
    * Verified by: #17 (TEST-AUDIO-001)
    */
   ```

2. **src/bpm/BPMCalculator.cpp** (180 lines)
   ```cpp
   /**
    * Implements: #3 (REQ-F-002: BPM Calculation)
    * Verified by: #18 (TEST-BPM-001)
    */
   ```

3. **src/timing/TimingManager.cpp** (276 lines)
   ```cpp
   /**
    * Implements: #13 (ADR-002: DS3231 RTC)
    * Verified by: Hardware Test Suite
    */
   ```

4. **test/test_audio/test_audio.cpp** (~2000 lines)
   ```cpp
   // Verifies: #2 (REQ-F-001: Tempo Detection)
   // Verifies: #17 (TEST-AUDIO-001: Beat Detection Latency)
   void test_beat_detection() {
   ```

**Bulk Update Script** (create `scripts/add-issue-refs-to-code.py`):
- Scan `src/**/*.cpp` for class/function patterns
- Scan `test/**/*.cpp` for test functions
- Insert `Implements: #N` / `Verifies: #N` comments
- Preserve existing code structure

---

### Priority 3: Implement Missing Features (NEXT WEEK)

**Feature Status**:

| Feature | Status | Effort | Priority |
|---------|--------|--------|----------|
| **Audio Detection** | ✅ COMPLETE | - | - |
| **BPM Calculation** | ✅ COMPLETE | - | - |
| **LED Output** | ✅ COMPLETE | - | - |
| **Web Server** | ❌ MISSING | 4h | P1 |
| **HTTP API** | ❌ MISSING | 2h | P1 |
| **Web UI** | ❌ MISSING | 6h | P2 |
| **MQTT Client** | ❌ MISSING | 3h | P2 |
| **MIDI Output** | ❌ MISSING | 4h | P2 |
| **RTP MIDI** | ❌ MISSING | 4h | P3 |

**Implementation Order**:
1. **Web Server + HTTP API** (6h)
   - ESPAsyncWebServer setup
   - `/api/config` GET/POST endpoints
   - `/api/status` GET endpoint (BPM, lock status, audio levels)
   
2. **Web UI** (6h)
   - Static HTML/CSS/JS served from SPIFFS
   - Real-time BPM display (WebSocket or polling)
   - Configuration form
   
3. **MQTT Client** (3h)
   - 256dpi/MQTT library integration
   - Telemetry publishing every 5s
   - LWT (Last Will Testament) support
   
4. **MIDI Beat Clock** (4h)
   - Serial MIDI output (GPIO pin)
   - 24 PPQ (pulses per quarter note)
   - Song position pointer (SPP)
   
5. **RTP MIDI** (4h)
   - Apple MIDI (UDP port 5004)
   - Network MIDI synchronization
   - Zero-conf discovery (optional)

---

## 🎯 Success Criteria

### CI Validation Targets

```
✅ Requirements overall coverage ≥ 90.00%
✅ ADR linkage coverage ≥ 70.00%
✅ Scenario linkage coverage ≥ 60.00%
✅ Test linkage coverage ≥ 40.00%
```

### Deliverables

1. **GitHub Issues Created**: 20+ issues (StR, REQ-F, ADR, TEST)
2. **Code Annotated**: All `src/**/*.cpp` have `Implements: #N` comments
3. **Tests Annotated**: All `test/**/*.cpp` have `Verifies: #N` comments
4. **Traceability Matrix**: `build/traceability.json` generated
5. **CI Passing**: All GitHub Actions workflows green ✅

---

## 📞 Next Steps

### Immediate (Today)
1. **Create GitHub Issues**:
   ```bash
   # Use GitHub CLI or web interface
   gh issue create --title "StR-001: Clap-Based Metronome System" \
     --label "type:stakeholder-requirement,phase:01,priority:p0" \
     --body-file "01-stakeholder-requirements/StR-001-clap-metronome-system.md"
   ```

2. **Run Traceability Builder** (after issues created):
   ```bash
   python scripts/build-traceability.py
   python scripts/validate-trace-coverage.py --min-req 90
   ```

### Short-Term (This Week)
1. Add issue references to existing code (30 files)
2. Implement Web Server + HTTP API (REQ-F-005)
3. Implement MQTT Client (REQ-F-007)

### Medium-Term (Next Week)
1. Implement Web UI (REQ-F-006)
2. Implement MIDI Beat Clock (REQ-F-008)
3. Implement RTP MIDI (REQ-F-009)

---

## 📚 References

- **Standards**: ISO/IEC/IEEE 29148:2018 (Requirements Engineering)
- **CI Workflow**: `.github/workflows/ci-standards-compliance.yml`
- **Validation Script**: `scripts/validate-trace-coverage.py`
- **Traceability Builder**: `scripts/build-traceability.py` (✅ CREATED)
- **Issue Templates**: `.github/ISSUE_TEMPLATE/*.yml`
- **Documentation**: `docs/copilot-usage.md`, `docs/lifecycle-guide.md`

---

**Status**: 📋 **PLAN READY** - Awaiting GitHub Issue Creation  
**Estimated Total Effort**: 20-30 hours (issues + code annotation + features)  
**Critical Path**: Create issues → Annotate code → Implement features → CI validation

**Next Action**: Create GitHub issues for StR-001, REQ-F-001 to REQ-F-010, ADR-001 to ADR-005
