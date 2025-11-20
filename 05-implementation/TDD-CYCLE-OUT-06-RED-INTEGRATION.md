# TDD Cycle OUT-06: BPM Engine Integration - RED Phase

**Date**: 2025-11-20
**Cycle**: OUT-06 (BPM Engine Integration)
**Phase**: RED - Test Creation
**Objective**: Integrate OutputController with BPM Calculation Engine

---

## Executive Summary

Create integration tests connecting BPM Calculation Engine to OutputController. Verify BPM updates propagate correctly, detection events trigger outputs, and full system operates end-to-end from clap detection through BPM calculation to synchronized outputs.

**Integration Points**:
1. BPM updates from engine → OutputController.updateBPM()
2. Detection events → Relay pulse triggers
3. Stability detection → Output synchronization
4. Full pipeline: Clap → BPM → Output

---

## Acceptance Criteria

### AC-OUT-014: BPM Update Integration ✓
**Given**: BPM calculation engine provides BPM updates via callback  
**When**: BPM value changes  
**Then**: OutputController receives update and adjusts timer interval  
**Verification**: Timer interval recalculated according to BPM

### AC-OUT-015: Detection Event Integration ✓
**Given**: Detection system identifies beat events  
**When**: Beat detected  
**Then**: Relay pulse triggered (if relay enabled)  
**Verification**: Relay pulse occurs on beat

### AC-OUT-016: Stability-Based Synchronization ✓
**Given**: BPM calculation provides stability flag  
**When**: BPM becomes stable (CV < 5%, ≥4 taps)  
**Then**: MIDI clock synchronization starts automatically  
**Verification**: MIDI clocks sent at calculated interval

### AC-OUT-017: End-to-End Integration ✓
**Given**: Complete system (detection → BPM → output)  
**When**: Series of taps provided  
**Then**: System calculates BPM and outputs synchronized clock  
**Verification**: Full pipeline operates correctly

---

## Test Plan (12 Tests)

### Category 1: BPM Update Integration (3 tests)

#### Test 1: BPMUpdate_UpdatesTimerInterval
**Objective**: Verify BPM updates propagate to timer
**Setup**:
- Create BPMCalculation engine
- Create OutputController
- Connect via callback
**Action**:
- Add 4 taps at 120 BPM to engine
- Engine calculates BPM and fires callback
**Verify**:
- OutputController.updateBPM() called with 120 BPM
- Timer interval = 20833µs (60,000,000 / 120 / 24)
- Timer stats reflect new interval

#### Test 2: BPMUpdate_MultipleBPMChanges
**Objective**: Verify dynamic BPM changes work correctly
**Setup**: Connected BPM engine and OutputController
**Action**:
- Start at 120 BPM (4 taps)
- Change to 100 BPM (4 more taps)
- Change to 140 BPM (4 more taps)
**Verify**:
- Each BPM change updates timer interval
- 120 BPM → 20833µs
- 100 BPM → 25000µs
- 140 BPM → 17857µs

#### Test 3: BPMUpdate_IgnoresInvalidBPM
**Objective**: Verify out-of-range BPM values rejected
**Setup**: Connected system
**Action**:
- Attempt BPM updates: 30 BPM (too low), 250 BPM (too high)
**Verify**:
- Updates rejected (not applied)
- Timer interval unchanged
- Valid range: 40-240 BPM

### Category 2: Detection Event Integration (3 tests)

#### Test 4: DetectionEvent_TriggersRelayPulse
**Objective**: Verify beat detection triggers relay
**Setup**:
- OutputController in RELAY_ONLY mode
- Relay enabled
**Action**:
- Simulate beat detection event
- Call relay pulse trigger
**Verify**:
- Relay activates for configured duration (50ms default)
- Relay stats increment pulse count

#### Test 5: DetectionEvent_MIDIAndRelayCoordinated
**Objective**: Verify both outputs can be triggered together
**Setup**:
- OutputController in BOTH mode
- System syncing at 120 BPM
**Action**:
- Trigger beat detection
**Verify**:
- Relay pulse occurs
- MIDI clocks continue uninterrupted
- No jitter introduced

#### Test 6: DetectionEvent_RespectDebounce
**Objective**: Verify relay debounce prevents rapid re-trigger
**Setup**: OutputController with 10ms debounce
**Action**:
- Trigger relay pulse
- Attempt immediate re-trigger (5ms later)
- Wait debounce period
- Trigger again (15ms later)
**Verify**:
- First pulse: succeeds
- Second pulse: rejected (within debounce)
- Third pulse: succeeds (after debounce)

### Category 3: Stability-Based Synchronization (3 tests)

#### Test 7: StableDetection_StartsSync
**Objective**: Verify sync starts when BPM stable
**Setup**:
- BPM engine connected to OutputController
- MIDI_ONLY mode
**Action**:
- Add 4 taps at 120 BPM (CV < 5%, stable)
**Verify**:
- MIDI sync starts automatically
- State: STOPPED → RUNNING
- MIDI START message sent
- Clocks sent at 20833µs intervals

#### Test 8: UnstableBPM_NoAutoSync
**Objective**: Verify unstable BPM doesn't start sync
**Setup**: Connected system
**Action**:
- Add 3 taps (insufficient for stability)
**Verify**:
- Sync does NOT auto-start
- State remains STOPPED
- No MIDI messages sent

#### Test 9: StabilityLoss_MaintainsSync
**Objective**: Verify sync continues if stability lost temporarily
**Setup**:
- System syncing at stable 120 BPM
**Action**:
- Add erratic tap (causes CV > 5%)
**Verify**:
- Sync continues
- BPM adjusts to new average
- No STOP message sent

### Category 4: End-to-End Integration (3 tests)

#### Test 10: EndToEnd_ClapToBPMToOutput
**Objective**: Verify full pipeline from detection to output
**Setup**:
- Create complete system (detection → BPM → output)
- Connect all callbacks
**Action**:
- Simulate 4 clap detections at 120 BPM
**Verify**:
- BPM calculated: ~120 BPM
- Relay pulses on each detection
- MIDI sync starts after stability
- Timer interval correct (20833µs)

#### Test 11: EndToEnd_BPMChange_OutputAdjusts
**Objective**: Verify output adjusts to BPM changes
**Setup**: System running at 120 BPM
**Action**:
- Change tempo: 4 taps at 140 BPM
**Verify**:
- BPM updates to ~140 BPM
- Timer interval adjusts: 20833µs → 17857µs
- MIDI clocks maintain synchronization
- No glitches or dropped clocks

#### Test 12: EndToEnd_StopAndRestart
**Objective**: Verify clean stop/restart cycle
**Setup**: System syncing at 120 BPM
**Action**:
- Stop sync (manual or timeout)
- Clear taps
- Start new sequence at 100 BPM
**Verify**:
- STOP message sent
- State: RUNNING → STOPPED
- New sequence starts fresh
- BPM recalculated correctly
- START message sent

---

## Implementation Requirements

### New Integration Layer

#### BPMOutputBridge Class
**Purpose**: Coordinate BPM engine and OutputController

**Responsibilities**:
- Register as BPM update callback listener
- Forward BPM updates to OutputController
- Coordinate detection events with relay pulses
- Manage stability-based auto-sync logic

**Interface**:
```cpp
class BPMOutputBridge {
public:
    BPMOutputBridge(BPMCalculation* bpm, OutputController* output);
    
    void init();
    
    // Callback from BPM engine
    void onBPMUpdate(const BPMUpdateEvent& event);
    
    // Detection event handler
    void onBeatDetected(uint64_t timestamp_us);
    
    // Configuration
    void setAutoSyncEnabled(bool enabled);
    bool isAutoSyncEnabled() const;
    
private:
    BPMCalculation* bpm_engine_;
    OutputController* output_controller_;
    bool auto_sync_enabled_;
    bool is_syncing_;
};
```

### OutputController Enhancements

**New Methods** (if needed):
```cpp
// Already have:
void updateBPM(uint16_t bpm);           // Dynamic BPM updates ✓
bool startSync(uint16_t bpm);           // Start with BPM ✓
void pulseRelay();                      // Trigger relay pulse ✓

// May need:
void setBPMSource(BPMCalculation* bpm); // Connect to BPM engine
void setAutoSync(bool enabled);         // Enable stability-based auto-sync
```

### BPMCalculation Enhancements

**Callback Registration**:
```cpp
// Already have:
void onBPMUpdate(std::function<void(const BPMUpdateEvent&)> callback); ✓

// Event structure:
struct BPMUpdateEvent {
    float bpm;
    bool is_stable;
    uint64_t timestamp_us;
    uint8_t tap_count;
}; ✓
```

---

## Test Infrastructure

### Mock/Stub Requirements

1. **MockTimingProvider**: Already exists for BPM tests ✓
2. **Real OutputController**: Use actual implementation
3. **Real BPMCalculation**: Use actual implementation
4. **Test Fixture**: BPMOutputIntegrationTest

### Test Fixture Structure

```cpp
class BPMOutputIntegrationTest : public ::testing::Test {
protected:
    // Components
    MockTimingProvider* timing_;
    BPMCalculation* bpm_engine_;
    OutputController* output_controller_;
    BPMOutputBridge* bridge_;  // Integration layer
    
    // Test helpers
    void addTapsAtBPM(uint8_t count, uint16_t bpm);
    void advanceTime(uint64_t us);
    
    void SetUp() override {
        timing_ = new MockTimingProvider();
        bpm_engine_ = new BPMCalculation(timing_);
        output_controller_ = new OutputController();
        
        // Configure output for tests
        OutputConfig config = output_controller_->getConfig();
        config.mode = OutputMode::BOTH;
        config.midi_send_start_stop = true;
        output_controller_->setConfig(config);
        
        // Create integration bridge
        bridge_ = new BPMOutputBridge(bpm_engine_, output_controller_);
        bridge_->init();
    }
    
    void TearDown() override {
        delete bridge_;
        delete output_controller_;
        delete bpm_engine_;
        delete timing_;
    }
};
```

---

## Success Criteria

### Compilation (RED Phase)
- ❌ Tests compile but fail (expected for RED phase)
- ❌ Missing BPMOutputBridge implementation
- ❌ Integration points not connected

### Test Execution (GREEN Phase - Target)
- ✅ 12/12 integration tests passing
- ✅ BPM updates propagate correctly
- ✅ Detection events trigger outputs
- ✅ Stability-based sync works
- ✅ End-to-end pipeline functional
- ✅ Total: **71 tests passing** (59 previous + 12 new)

---

## Performance Targets

| Metric | Target | Verification |
|--------|--------|--------------|
| BPM Update Latency | <5ms | Measure callback to timer update |
| Detection to Relay | <10ms | Trigger to pulse activation |
| Sync Start Latency | <20ms | Stability to first clock |
| Memory Overhead | <1KB | Bridge object size |
| CPU Impact | <1% | Additional processing |

---

## Traceability

**Requirements**:
- AC-OUT-014: BPM Update Integration (Tests 1-3)
- AC-OUT-015: Detection Event Integration (Tests 4-6)
- AC-OUT-016: Stability-Based Synchronization (Tests 7-9)
- AC-OUT-017: End-to-End Integration (Tests 10-12)

**Previous Cycles**:
- OUT-01: MIDI Beat Clock (16 tests) ✅
- OUT-02: RTP-MIDI Network (13 tests) ✅
- OUT-03: Timer-based Clock (10 tests) ✅
- OUT-04: Relay Output (10 tests) ✅
- OUT-05: Performance Validation (10 tests) ✅

**Architecture**:
- ARC-C-002: BPM Calculation Engine (existing)
- ARC-C-004: Output Controller (existing)
- **NEW**: BPM-Output Integration Layer

---

## File Structure

### New Files (to be created)
```
src/integration/
  BPMOutputBridge.h          # Integration layer header
  BPMOutputBridge.cpp        # Integration implementation

test/test_integration/
  CMakeLists.txt             # Build configuration
  test_bpm_output.cpp        # Integration tests (12 tests)
```

### Modified Files (if needed)
```
src/output/OutputController.h   # May add setBPMSource()
src/output/OutputController.cpp # May add BPM source tracking
```

---

## Standards Compliance

**ISO/IEC/IEEE 12207:2017**: Integration Process
- Component integration planning
- Interface compatibility verification
- Integration testing procedures

**IEEE 1012-2016**: Integration Testing
- Interface testing
- Component interaction validation
- System-level verification

**XP Practices**:
- Test-Driven Development (TDD)
- Continuous Integration
- Simple Design (minimal coupling)
- Collective Code Ownership

---

## Next Steps

1. **Create Test File**: `test/test_integration/test_bpm_output.cpp`
2. **Implement Tests**: 12 integration tests (RED phase)
3. **Verify Compilation Failures**: Expected missing symbols
4. **Create Bridge**: `src/integration/BPMOutputBridge` (GREEN phase)
5. **Run Tests**: Achieve 12/12 passing
6. **Document Success**: GREEN phase completion

---

**Status**: 🔴 RED Phase - Test Creation
**Date**: 2025-11-20
**Next**: Create test file with 12 integration tests
**Target**: 71 total tests (59 + 12)
