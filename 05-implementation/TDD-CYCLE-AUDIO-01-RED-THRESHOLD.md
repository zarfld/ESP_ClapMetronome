# TDD Cycle AUDIO-01: Adaptive Threshold - RED Phase

**Date**: 2025-11-20  
**Component**: DES-C-001 Audio Detection Engine  
**Cycle**: AUDIO-01 (Adaptive Threshold Calculation)  
**Phase**: RED (Create Failing Tests)  
**Status**: 🔴 RED - Tests written, awaiting GREEN implementation

## Objective

Implement adaptive threshold calculation for beat/clap detection. Threshold dynamically adjusts based on recent audio samples to accommodate varying microphone levels and room acoustics.

**Acceptance Criteria**: AC-AUDIO-001  
**Formula**: `threshold = 0.8 × (max - min) + min`

## Test Plan

### Category 1: Threshold Calculation (4 tests)

These tests verify the core adaptive threshold formula works correctly with various audio sample patterns.

#### Test 1: ThresholdCalculation_QuietEnvironment
**Given**: Audio samples with low amplitude (100-200 range)  
**When**: calculateThreshold() called  
**Then**: Threshold = 0.8 × (200-100) + 100 = 180

**Test Code**:
```cpp
TEST_F(AdaptiveThresholdTest, ThresholdCalculation_QuietEnvironment) {
    // Simulate quiet room: samples between 100-200
    for (int i = 0; i < 100; i++) {
        detector->processSample(100 + (i % 101)); // 100 to 200
    }
    
    float threshold = detector->getThreshold();
    
    // Expected: 0.8 × (200-100) + 100 = 180
    EXPECT_NEAR(threshold, 180.0f, 5.0f);
}
```

#### Test 2: ThresholdCalculation_LoudEnvironment
**Given**: Audio samples with high amplitude (500-1000 range)  
**When**: calculateThreshold() called  
**Then**: Threshold = 0.8 × (1000-500) + 500 = 900

**Test Code**:
```cpp
TEST_F(AdaptiveThresholdTest, ThresholdCalculation_LoudEnvironment) {
    // Simulate loud room: samples between 500-1000
    for (int i = 0; i < 100; i++) {
        detector->processSample(500 + (i % 501)); // 500 to 1000
    }
    
    float threshold = detector->getThreshold();
    
    // Expected: 0.8 × (1000-500) + 500 = 900
    EXPECT_NEAR(threshold, 900.0f, 10.0f);
}
```

#### Test 3: ThresholdCalculation_UpdatesWithNewSamples
**Given**: Initial quiet samples, then loud samples introduced  
**When**: New louder samples processed  
**Then**: Threshold increases to match new amplitude range

**Test Code**:
```cpp
TEST_F(AdaptiveThresholdTest, ThresholdCalculation_UpdatesWithNewSamples) {
    // Start with quiet samples
    for (int i = 0; i < 50; i++) {
        detector->processSample(100 + (i % 51)); // 100-150
    }
    float initial_threshold = detector->getThreshold();
    
    // Introduce louder samples
    for (int i = 0; i < 50; i++) {
        detector->processSample(500 + (i % 501)); // 500-1000
    }
    float updated_threshold = detector->getThreshold();
    
    // Threshold should increase significantly
    EXPECT_GT(updated_threshold, initial_threshold + 100.0f);
}
```

#### Test 4: ThresholdCalculation_HandlesConstantLevel
**Given**: All samples at same amplitude (no variation)  
**When**: calculateThreshold() called  
**Then**: Threshold = sample value (no change)

**Test Code**:
```cpp
TEST_F(AdaptiveThresholdTest, ThresholdCalculation_HandlesConstantLevel) {
    // All samples at 500 (no variation)
    for (int i = 0; i < 100; i++) {
        detector->processSample(500);
    }
    
    float threshold = detector->getThreshold();
    
    // Expected: 0.8 × (500-500) + 500 = 500
    EXPECT_NEAR(threshold, 500.0f, 1.0f);
}
```

### Category 2: Window Size and Timing (3 tests)

These tests verify threshold adapts over correct time window and handles edge cases.

#### Test 5: ThresholdWindow_Uses100Samples
**Given**: More than 100 samples processed  
**When**: Threshold calculated  
**Then**: Only last 100 samples used (oldest discarded)

**Test Code**:
```cpp
TEST_F(AdaptiveThresholdTest, ThresholdWindow_Uses100Samples) {
    // Fill with 50 quiet samples
    for (int i = 0; i < 50; i++) {
        detector->processSample(100);
    }
    
    // Fill with 100 loud samples (should push out quiet ones)
    for (int i = 0; i < 100; i++) {
        detector->processSample(1000);
    }
    
    float threshold = detector->getThreshold();
    
    // Should reflect loud samples only, not quiet ones
    // Expected: 0.8 × (1000-1000) + 1000 = 1000 (constant)
    EXPECT_NEAR(threshold, 1000.0f, 10.0f);
}
```

#### Test 6: ThresholdWindow_InitialSamplesUsed
**Given**: Less than 100 samples available  
**When**: Threshold calculated  
**Then**: Uses all available samples (doesn't wait for 100)

**Test Code**:
```cpp
TEST_F(AdaptiveThresholdTest, ThresholdWindow_InitialSamplesUsed) {
    // Only 10 samples
    for (int i = 0; i < 10; i++) {
        detector->processSample(100 + i * 10); // 100 to 190
    }
    
    float threshold = detector->getThreshold();
    
    // Should calculate from 10 samples
    // Expected: 0.8 × (190-100) + 100 = 172
    EXPECT_NEAR(threshold, 172.0f, 5.0f);
}
```

#### Test 7: ThresholdWindow_RealTimeUpdate
**Given**: Continuous sample stream  
**When**: Each sample processed  
**Then**: Threshold updates in real-time (<1ms)

**Test Code**:
```cpp
TEST_F(AdaptiveThresholdTest, ThresholdWindow_RealTimeUpdate) {
    auto start = micros();
    
    // Process 100 samples
    for (int i = 0; i < 100; i++) {
        detector->processSample(500 + i);
    }
    
    auto duration = micros() - start;
    
    // Should complete in <100ms (1ms per sample target)
    EXPECT_LT(duration, 100000); // 100ms in microseconds
}
```

### Category 3: Edge Cases (3 tests)

#### Test 8: EdgeCase_ZeroAmplitudeSamples
**Given**: All samples are zero  
**When**: Threshold calculated  
**Then**: Threshold = 0 (no false positives)

**Test Code**:
```cpp
TEST_F(AdaptiveThresholdTest, EdgeCase_ZeroAmplitudeSamples) {
    // All zero samples (silence)
    for (int i = 0; i < 100; i++) {
        detector->processSample(0);
    }
    
    float threshold = detector->getThreshold();
    
    EXPECT_NEAR(threshold, 0.0f, 0.1f);
}
```

#### Test 9: EdgeCase_MaximumAmplitudeSamples
**Given**: Samples at ADC maximum (4095 for 12-bit)  
**When**: Threshold calculated  
**Then**: Threshold = 4095 (handles max range)

**Test Code**:
```cpp
TEST_F(AdaptiveThresholdTest, EdgeCase_MaximumAmplitudeSamples) {
    // All samples at ADC max (12-bit = 4095)
    for (int i = 0; i < 100; i++) {
        detector->processSample(4095);
    }
    
    float threshold = detector->getThreshold();
    
    EXPECT_NEAR(threshold, 4095.0f, 1.0f);
}
```

#### Test 10: EdgeCase_NegativeToPositiveRange
**Given**: Samples range from negative to positive (DC-centered)  
**When**: Threshold calculated  
**Then**: Uses absolute values correctly

**Test Code**:
```cpp
TEST_F(AdaptiveThresholdTest, EdgeCase_NegativeToPositiveRange) {
    // Simulate DC-centered audio: -500 to +500
    for (int i = 0; i < 100; i++) {
        detector->processSample(-500 + (i % 1001)); // -500 to 500
    }
    
    float threshold = detector->getThreshold();
    
    // Expected: 0.8 × (500-(-500)) + (-500) = 0.8 × 1000 - 500 = 300
    EXPECT_NEAR(threshold, 300.0f, 10.0f);
}
```

## Test Fixture

```cpp
/**
 * Test fixture for adaptive threshold tests
 * Provides mock timing and controlled sample injection
 */
class AdaptiveThresholdTest : public ::testing::Test {
protected:
    MockTimingProvider* timing_;
    AudioDetection* detector;
    
    void SetUp() override {
        // Create mock timing provider
        timing_ = new MockTimingProvider();
        timing_->setTimestamp(0);
        
        // Create audio detector with mock timing
        detector = new AudioDetection(timing_);
        detector->init();
    }
    
    void TearDown() override {
        delete detector;
        delete timing_;
    }
    
    // Helper: Advance time by microseconds
    void advanceTime(uint64_t us) {
        timing_->advanceTime(us);
    }
    
    // Helper: Simulate sample at specific amplitude
    void injectSample(uint16_t amplitude) {
        detector->processSample(amplitude);
        advanceTime(125); // 8kHz = 125µs per sample
    }
};
```

## Expected Implementation APIs

Based on tests, AudioDetection needs:

```cpp
class AudioDetection {
public:
    /**
     * Process single audio sample
     * Updates threshold calculation
     * 
     * @param sample 12-bit ADC value (0-4095)
     */
    void processSample(uint16_t sample);
    
    /**
     * Get current adaptive threshold
     * Formula: 0.8 × (max - min) + min over last 100 samples
     * 
     * @return Current threshold value
     */
    float getThreshold() const;
    
private:
    uint16_t sample_buffer_[100];  // Rolling window
    uint8_t buffer_index_;         // Current write position
    uint8_t sample_count_;         // Samples in buffer (0-100)
};
```

## Files to Create

1. **test/test_audio/test_adaptive_threshold.cpp** (this test suite)
2. **test/test_audio/CMakeLists.txt** (build configuration)
3. **test/mocks/MockTimingProvider.h** (if not exists from OUT cycles)

## Success Criteria

**RED Phase Complete** when:
- ✅ 10 tests written and compile successfully
- ✅ All 10 tests FAIL with expected errors
- ✅ Test fixture compiles and runs
- ✅ Compilation errors documented

**Expected Failures**:
- `AudioDetection::processSample()` not implemented
- `AudioDetection::getThreshold()` not implemented
- Missing `sample_buffer_` member variable
- Missing threshold calculation logic

## Traceability

**Implements**:
- AC-AUDIO-001: Adaptive threshold calculation
- DES-I-004: Beat Event Interface (partial - threshold is prereq)
- REQ-F-001: Audio clap/kick detection (foundation)

**Design Reference**:
- DES-C-001: Audio Detection Engine (#45)
- DES-D-002: Audio Detection State (threshold state)

**Next Cycle**: AUDIO-02 (State Machine - Rising Edge Detection)

---

**Status**: 🔴 RED Phase Ready  
**Test Count**: 10 comprehensive tests  
**Expected Failures**: processSample(), getThreshold() not implemented  
**Next**: GREEN phase implementation
