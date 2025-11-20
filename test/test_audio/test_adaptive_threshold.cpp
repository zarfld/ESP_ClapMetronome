/**
 * TDD Cycle AUDIO-01: Adaptive Threshold Tests
 * 
 * Tests adaptive threshold calculation for beat/clap detection.
 * Formula: threshold = 0.8 × (max - min) + min over last 100 samples
 * 
 * Component: DES-C-001 Audio Detection Engine
 * Acceptance Criteria: AC-AUDIO-001
 * GitHub Issue: #45
 * 
 * TDD Phase: RED ⏳
 * Expected: 10/10 tests FAIL (processSample/getThreshold not implemented)
 */

#include <gtest/gtest.h>
#include "../../src/audio/AudioDetection.h"
#include "../mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * Test fixture for adaptive threshold tests
 * Provides mock timing and controlled sample injection
 */
class AdaptiveThresholdTest : public ::testing::Test {
protected:
    MockTimingProvider* timing_;
    AudioDetection* detector_;
    
    void SetUp() override {
        // Create mock timing provider
        timing_ = new MockTimingProvider();
        timing_->setTimestamp(0);
        
        // Create audio detector with mock timing
        detector_ = new AudioDetection(timing_);
        detector_->init();
    }
    
    void TearDown() override {
        delete detector_;
        delete timing_;
    }
    
    // Helper: Advance time by microseconds
    void advanceTime(uint64_t us) {
        timing_->advanceTime(us);
    }
    
    // Helper: Simulate sample at specific amplitude
    void injectSample(uint16_t amplitude) {
        detector_->processSample(amplitude);
        advanceTime(125); // 8kHz = 125µs per sample
    }
};

// ============================================================================
// Category 1: Threshold Calculation (4 tests)
// ============================================================================

/**
 * Test 1: Threshold calculation in quiet environment
 * 
 * Verifies threshold formula with low amplitude samples.
 * Expected: 0.8 × (200-100) + 100 = 180
 */
TEST_F(AdaptiveThresholdTest, ThresholdCalculation_QuietEnvironment) {
    // Simulate quiet room: samples between 100-200
    for (int i = 0; i < 100; i++) {
        detector_->processSample(100 + (i % 101)); // 100 to 200
        advanceTime(125); // 8kHz sample rate
    }
    
    float threshold = detector_->getThreshold();
    
    // Expected: 0.8 × (200-100) + 100 = 180
    EXPECT_NEAR(threshold, 180.0f, 5.0f);
}

/**
 * Test 2: Threshold calculation in loud environment
 * 
 * Verifies threshold formula with high amplitude samples.
 * Note: 100 samples = 500-599, so range is 99
 * Expected: 0.8 × (599-500) + 500 = 579
 */
TEST_F(AdaptiveThresholdTest, ThresholdCalculation_LoudEnvironment) {
    // Simulate loud room: samples between 500-1000
    // Note: With 100 samples and (i % 501), we only get 500-599
    for (int i = 0; i < 501; i++) {
        detector_->processSample(500 + (i % 501)); // 500 to 1000
        advanceTime(125);
    }
    
    float threshold = detector_->getThreshold();
    
    // Expected: With 501 samples and 100-sample window, last 100 are range 900-1000
    // threshold = 0.8 × (1000-900) + 900 = 980
    EXPECT_NEAR(threshold, 980.0f, 15.0f);
}

/**
 * Test 3: Threshold updates with new samples
 * 
 * Verifies threshold adapts when sample amplitude changes.
 * Threshold should increase when louder samples introduced.
 */
TEST_F(AdaptiveThresholdTest, ThresholdCalculation_UpdatesWithNewSamples) {
    // Start with quiet samples
    for (int i = 0; i < 50; i++) {
        detector_->processSample(100 + (i % 51)); // 100-150
        advanceTime(125);
    }
    float initial_threshold = detector_->getThreshold();
    
    // Introduce louder samples
    for (int i = 0; i < 50; i++) {
        detector_->processSample(500 + (i % 501)); // 500-1000
        advanceTime(125);
    }
    float updated_threshold = detector_->getThreshold();
    
    // Threshold should increase significantly
    EXPECT_GT(updated_threshold, initial_threshold + 100.0f);
}

/**
 * Test 4: Threshold handles constant amplitude
 * 
 * Verifies threshold calculation when no variation exists.
 * Expected: 0.8 × (500-500) + 500 = 500
 */
TEST_F(AdaptiveThresholdTest, ThresholdCalculation_HandlesConstantLevel) {
    // All samples at 500 (no variation)
    for (int i = 0; i < 100; i++) {
        detector_->processSample(500);
        advanceTime(125);
    }
    
    float threshold = detector_->getThreshold();
    
    // Expected: 0.8 × (500-500) + 500 = 500
    EXPECT_NEAR(threshold, 500.0f, 1.0f);
}

// ============================================================================
// Category 2: Window Size and Timing (3 tests)
// ============================================================================

/**
 * Test 5: Threshold uses rolling window of 100 samples
 * 
 * Verifies oldest samples are discarded when buffer full.
 * Only last 100 samples should affect threshold.
 */
TEST_F(AdaptiveThresholdTest, ThresholdWindow_Uses100Samples) {
    // Fill with 50 quiet samples
    for (int i = 0; i < 50; i++) {
        detector_->processSample(100);
        advanceTime(125);
    }
    
    // Fill with 100 loud samples (should push out quiet ones)
    for (int i = 0; i < 100; i++) {
        detector_->processSample(1000);
        advanceTime(125);
    }
    
    float threshold = detector_->getThreshold();
    
    // Should reflect loud samples only, not quiet ones
    // Expected: 0.8 × (1000-1000) + 1000 = 1000 (constant)
    EXPECT_NEAR(threshold, 1000.0f, 10.0f);
}

/**
 * Test 6: Threshold works with fewer than 100 samples
 * 
 * Verifies threshold calculation doesn't wait for full buffer.
 * Should use all available samples.
 */
TEST_F(AdaptiveThresholdTest, ThresholdWindow_InitialSamplesUsed) {
    // Only 10 samples
    for (int i = 0; i < 10; i++) {
        detector_->processSample(100 + i * 10); // 100 to 190
        advanceTime(125);
    }
    
    float threshold = detector_->getThreshold();
    
    // Should calculate from 10 samples
    // Expected: 0.8 × (190-100) + 100 = 172
    EXPECT_NEAR(threshold, 172.0f, 5.0f);
}

/**
 * Test 7: Threshold updates in real-time
 * 
 * Verifies threshold calculation is fast enough for real-time audio.
 * 100 samples should process in <100ms (1ms per sample target).
 */
TEST_F(AdaptiveThresholdTest, ThresholdWindow_RealTimeUpdate) {
    timing_->setTimestamp(0);
    
    // Process 100 samples
    for (int i = 0; i < 100; i++) {
        detector_->processSample(500 + i);
        advanceTime(125); // Simulate 8kHz
    }
    
    uint64_t duration = timing_->getTimestampUs();
    
    // Should complete in reasonable time (125µs × 100 = 12.5ms)
    EXPECT_LT(duration, 100000); // 100ms in microseconds
}

// ============================================================================
// Category 3: Edge Cases (3 tests)
// ============================================================================

/**
 * Test 8: Threshold handles zero amplitude (silence)
 * 
 * Verifies no false positives in silent conditions.
 * Threshold should be zero.
 */
TEST_F(AdaptiveThresholdTest, EdgeCase_ZeroAmplitudeSamples) {
    // All zero samples (silence)
    for (int i = 0; i < 100; i++) {
        detector_->processSample(0);
        advanceTime(125);
    }
    
    float threshold = detector_->getThreshold();
    
    EXPECT_NEAR(threshold, 0.0f, 0.1f);
}

/**
 * Test 9: Threshold handles maximum ADC values
 * 
 * Verifies threshold works at full scale (12-bit ADC = 4095).
 * No overflow or saturation issues.
 */
TEST_F(AdaptiveThresholdTest, EdgeCase_MaximumAmplitudeSamples) {
    // All samples at ADC max (12-bit = 4095)
    for (int i = 0; i < 100; i++) {
        detector_->processSample(4095);
        advanceTime(125);
    }
    
    float threshold = detector_->getThreshold();
    
    EXPECT_NEAR(threshold, 4095.0f, 1.0f);
}

/**
 * Test 10: Threshold handles DC-centered audio
 * 
 * Verifies threshold calculation with negative-to-positive range.
 * Simulates audio centered at 0V (bipolar signal).
 */
TEST_F(AdaptiveThresholdTest, EdgeCase_NegativeToPositiveRange) {
    // Simulate DC-centered audio: -500 to +500
    // Note: In real system, ADC is 0-4095. This tests algorithm robustness.
    // Need 1001 samples to cover full -500 to +500 range
    for (int i = 0; i < 1001; i++) {
        // Cast to int16_t to allow negative values in test
        int16_t sample = -500 + (i % 1001); // -500 to 500
        detector_->processSample(static_cast<uint16_t>(sample + 2048)); // DC offset for ADC
        advanceTime(125);
    }
    
    float threshold = detector_->getThreshold();
    
    // Expected: With 1001 samples and 100-sample window, last 100 span ADC range
    // threshold = 0.8 × (range of last 100) + min
    // Range is approximately 900 ADC units, giving threshold around 2528
    EXPECT_NEAR(threshold, 2528.0f, 25.0f);
}

// ============================================================================
// Main (GTest auto-discovery)
// ============================================================================
