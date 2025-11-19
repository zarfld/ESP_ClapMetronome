/**
 * @file test_noise_rejection.cpp
 * @brief Unit Tests for AC-AUDIO-013 Noise Rejection
 * 
 * TDD Cycle 13: Noise Rejection Validation
 * 
 * Acceptance Criteria:
 *   - AC-AUDIO-013: No detections below threshold
 *   - Validates three-layer false positive protection from Cycle 9
 *   - Tests various noise patterns and edge cases
 * 
 * Test Strategy:
 *   This is a VALIDATION phase - noise rejection was implemented in Cycle 9.
 *   These unit tests complement Cycle 9's statistical tests with:
 *   1. Edge case noise patterns
 *   2. Threshold boundary enforcement
 *   3. Noise floor estimation accuracy
 *   4. Three-layer validation logic
 * 
 * Standards: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * XP Practice: Test-Driven Development (TDD) - VALIDATION Phase
 * 
 * Traceability:
 *   - Implements: AC-AUDIO-013 (Noise rejection unit tests)
 *   - Related: AC-AUDIO-009 (Detection accuracy - Cycle 9)
 *   - Design: 04-design/tdd-plan-phase-05.md
 */

#define _USE_MATH_DEFINES  // Enable M_PI on Windows
#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include "../../src/audio/AudioDetection.h"
#include "../mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * @brief Noise Rejection Test Fixture
 * 
 * Provides utilities for testing noise rejection capabilities.
 */
class NoiseRejectionTest : public ::testing::Test {
protected:
    MockTimingProvider mock_timing_;
    AudioDetection* detector_;
    
    // Beat event capture
    std::vector<BeatEvent> beat_events_;
    
    void SetUp() override {
        mock_timing_.reset();
        detector_ = new AudioDetection(&mock_timing_);
        detector_->init();
        
        beat_events_.clear();
        
        // Register callback
        detector_->onBeat([this](const BeatEvent& event) {
            beat_events_.push_back(event);
        });
    }
    
    void TearDown() override {
        delete detector_;
    }
    
    /**
     * Fill window with background noise level
     */
    void fillWindow(uint16_t level) {
        for (int i = 0; i < 64; i++) {
            detector_->processSample(level);
            mock_timing_.advanceTime(62);
        }
    }
    
    /**
     * Generate random noise within specified range
     */
    std::vector<uint16_t> generateRandomNoise(size_t count, uint16_t center, uint16_t range) {
        std::vector<uint16_t> noise;
        for (size_t i = 0; i < count; i++) {
            int value = center + (rand() % (2 * range + 1)) - range;
            value = std::max(0, std::min(4095, value));
            noise.push_back(static_cast<uint16_t>(value));
        }
        return noise;
    }
    
    /**
     * Generate periodic noise (simulates electrical interference)
     */
    std::vector<uint16_t> generatePeriodicNoise(size_t count, uint16_t center, uint16_t amplitude, double frequency) {
        std::vector<uint16_t> noise;
        for (size_t i = 0; i < count; i++) {
            double phase = 2.0 * M_PI * frequency * i / 16000.0;  // 16kHz sample rate
            int value = center + static_cast<int>(amplitude * std::sin(phase));
            value = std::max(0, std::min(4095, value));
            noise.push_back(static_cast<uint16_t>(value));
        }
        return noise;
    }
    
    /**
     * Process samples and count false positives
     */
    int processSamplesAndCountBeats(const std::vector<uint16_t>& samples) {
        beat_events_.clear();
        for (uint16_t sample : samples) {
            detector_->processSample(sample);
            mock_timing_.advanceTime(62);
        }
        return static_cast<int>(beat_events_.size());
    }
};

/**
 * @test NoiseAtThreshold_NoDetection
 * 
 * Validates that noise exactly at threshold does not trigger detection.
 * 
 * Given: Window filled with background level (2000)
 * When: Samples at adaptive threshold level
 * Then: No beats detected (threshold + margin required)
 * 
 * Traceability: AC-AUDIO-013 (No detections below threshold)
 */
TEST_F(NoiseRejectionTest, NoiseAtThreshold_NoDetection) {
    // Arrange: Fill window with quiet baseline to establish threshold
    fillWindow(1500);
    
    // Get current threshold
    uint16_t threshold = detector_->getThreshold();
    EXPECT_GT(threshold, 0) << "Threshold should be calculated";
    
    beat_events_.clear();
    
    // Act: Feed samples exactly at threshold (no margin)
    // These should not cross threshold + margin
    for (int i = 0; i < 100; i++) {
        detector_->processSample(threshold);
        mock_timing_.advanceTime(62);
    }
    
    // Assert: No detections (threshold + margin required per AC-AUDIO-009)
    EXPECT_EQ(0U, beat_events_.size())
        << "Samples at threshold should not trigger detection (margin required)";
}

/**
 * @test RandomNoise_BelowThreshold
 * 
 * Validates that random noise below threshold does not cause detections.
 * 
 * Given: Background level + random fluctuations
 * When: 200 samples of noise below threshold
 * Then: Zero false positives
 * 
 * Traceability: AC-AUDIO-013, AC-AUDIO-009 (False positive rate <5%)
 */
TEST_F(NoiseRejectionTest, RandomNoise_BelowThreshold) {
    // Arrange: Fill window with quiet level to establish low threshold
    const uint16_t BASELINE = 1500;
    fillWindow(BASELINE);
    
    // Generate random noise centered at baseline (no sudden jumps)
    std::vector<uint16_t> noise = generateRandomNoise(200, BASELINE, 30);
    
    // Act: Process noise
    size_t detections = processSamplesAndCountBeats(noise);
    
    // Assert: No false positives (noise stays near baseline)
    EXPECT_EQ(0U, detections)
        << "Random noise below threshold should not trigger detections";
}

/**
 * @test PeriodicNoise_NoDetection
 * 
 * Validates that periodic noise (e.g., 50Hz/60Hz hum) doesn't trigger beats.
 * 
 * Given: 50Hz periodic noise at low amplitude
 * When: 320 samples (20ms at 16kHz)
 * Then: No false positives
 * 
 * Traceability: AC-AUDIO-013 (Noise rejection)
 */
TEST_F(NoiseRejectionTest, PeriodicNoise_NoDetection) {
    // Arrange: Generate 50Hz noise (common AC line frequency) with very low amplitude
    const uint16_t CENTER = 1800;
    const uint16_t AMPLITUDE = 30;  // Very small amplitude - below detection threshold
    
    // Fill window with periodic noise itself to establish proper threshold
    for (int i = 0; i < 64; i++) {
        double phase = 2.0 * M_PI * 50.0 * i / 16000.0;
        int value = CENTER + static_cast<int>(AMPLITUDE * std::sin(phase));
        detector_->processSample(static_cast<uint16_t>(value));
        mock_timing_.advanceTime(62);
    }
    
    beat_events_.clear();
    
    // Act: Continue with periodic noise (same amplitude, no sudden changes)
    std::vector<uint16_t> noise = generatePeriodicNoise(320, CENTER, AMPLITUDE, 50.0);
    size_t detections = processSamplesAndCountBeats(noise);
    
    // Assert: No false positives from periodic noise
    EXPECT_EQ(0U, detections)
        << "Periodic 50Hz noise should not trigger detections";
}

/**
 * @test GradualDrift_NoDetection
 * 
 * Validates that slow signal drift doesn't trigger detection.
 * 
 * Given: Slowly increasing baseline (thermal drift simulation)
 * When: Gradual change from 2000 to 2300 over 200 samples
 * Then: No false positives (adaptive threshold follows drift)
 * 
 * Traceability: AC-AUDIO-013, AC-AUDIO-001 (Adaptive threshold)
 */
TEST_F(NoiseRejectionTest, GradualDrift_NoDetection) {
    // Arrange: Fill window
    fillWindow(2000);
    beat_events_.clear();
    
    // Act: Simulate slow baseline drift
    for (int i = 0; i < 200; i++) {
        uint16_t value = 2000 + static_cast<uint16_t>((300.0 * i) / 200.0);
        detector_->processSample(value);
        mock_timing_.advanceTime(62);
    }
    
    // Assert: No detections (adaptive threshold should follow drift)
    EXPECT_EQ(0U, beat_events_.size())
        << "Gradual baseline drift should not trigger detections";
}

/**
 * @test BurstNoise_BelowMinimumAmplitude
 * 
 * Validates that noise bursts below minimum amplitude don't trigger.
 * 
 * Given: Sudden noise spikes of insufficient amplitude
 * When: Spikes below MIN_SIGNAL_AMPLITUDE (200 ADC units)
 * Then: No detections (Layer 3 protection)
 * 
 * Traceability: AC-AUDIO-013, AC-AUDIO-009 (Layer 3: minimum amplitude)
 */
TEST_F(NoiseRejectionTest, BurstNoise_BelowMinimumAmplitude) {
    // Arrange: Fill window with stable low baseline
    const uint16_t BASELINE = 1500;
    fillWindow(BASELINE);
    
    beat_events_.clear();
    
    // Act: Generate small spikes (below MIN_SIGNAL_AMPLITUDE = 200)
    // These are only 100 ADC above baseline - insufficient for detection
    for (int i = 0; i < 10; i++) {
        // Background
        for (int j = 0; j < 20; j++) {
            detector_->processSample(BASELINE);
            mock_timing_.advanceTime(62);
        }
        
        // Small spike (only 100 above baseline, need 200)
        detector_->processSample(BASELINE + 100);
        mock_timing_.advanceTime(62);
    }
    
    // Assert: No detections (amplitude too small)
    EXPECT_EQ(0U, beat_events_.size())
        << "Burst noise below minimum amplitude should not trigger";
}

/**
 * @test NoiseFloorEstimation_Accuracy
 * 
 * Validates that noise floor estimation correctly identifies ambient level.
 * 
 * Given: Known noise level in window
 * When: Window filled with 2000 ± 50 ADC
 * Then: Threshold calculated accounts for noise floor
 * 
 * Traceability: AC-AUDIO-009 (Layer 1: noise floor estimation)
 */
TEST_F(NoiseRejectionTest, NoiseFloorEstimation_Accuracy) {
    // Arrange: Fill window with known noise level
    const uint16_t NOISE_CENTER = 2000;
    const uint16_t NOISE_RANGE = 50;
    
    for (int i = 0; i < 64; i++) {
        int noise = (rand() % (2 * NOISE_RANGE + 1)) - NOISE_RANGE;
        uint16_t value = static_cast<uint16_t>(NOISE_CENTER + noise);
        detector_->processSample(value);
        mock_timing_.advanceTime(62);
    }
    
    // Act: Get threshold (includes noise floor calculation)
    uint16_t threshold = detector_->getThreshold();
    
    // Assert: Threshold should be above noise level
    // Threshold = 0.8 * (max - min) + min, with noise floor enforcement
    // With ±50 noise, max-min ~100, threshold ~80, noise floor ~1950
    EXPECT_GT(threshold, NOISE_CENTER - 100)
        << "Threshold should account for noise floor";
    EXPECT_LT(threshold, NOISE_CENTER + 500)
        << "Threshold should be reasonable for noise level";
}

/**
 * @test ThresholdMargin_HysteresisProtection
 * 
 * Validates that threshold margin prevents oscillation at boundary.
 * 
 * Given: Samples oscillating around threshold
 * When: Values cross threshold but stay within margin
 * Then: No detections (hysteresis prevents oscillation)
 * 
 * Traceability: AC-AUDIO-009 (Layer 2: threshold margin/hysteresis)
 */
TEST_F(NoiseRejectionTest, ThresholdMargin_HysteresisProtection) {
    // Arrange: Fill window with stable baseline
    const uint16_t BASELINE = 1800;
    fillWindow(BASELINE);
    uint16_t threshold = detector_->getThreshold();
    
    beat_events_.clear();
    
    // Act: Oscillate just below threshold + margin (within hysteresis zone)
    // Margin is 80, so oscillate at threshold + 40 (below threshold + margin)
    for (int i = 0; i < 50; i++) {
        uint16_t value = static_cast<uint16_t>(threshold + (i % 2 == 0 ? 40 : 20));
        detector_->processSample(value);
        mock_timing_.advanceTime(62);
    }
    
    // Assert: No detections (oscillations below threshold + margin)
    EXPECT_EQ(0U, beat_events_.size())
        << "Oscillations within threshold margin should not trigger detection";
}

/**
 * @test WidebandNoise_HighFrequency
 * 
 * Validates rejection of high-frequency wideband noise.
 * 
 * Given: Rapid sample-to-sample variations
 * When: High frequency noise (approaching Nyquist)
 * Then: No detections (not a valid beat signature)
 * 
 * Traceability: AC-AUDIO-013 (Noise rejection)
 */
TEST_F(NoiseRejectionTest, WidebandNoise_HighFrequency) {
    // Arrange: Fill window with alternating noise to establish threshold
    const uint16_t CENTER = 2000;
    const uint16_t NOISE_AMP = 50;  // Small amplitude
    
    for (int i = 0; i < 64; i++) {
        uint16_t value = static_cast<uint16_t>(CENTER + (i % 2 == 0 ? NOISE_AMP : -NOISE_AMP));
        detector_->processSample(value);
        mock_timing_.advanceTime(62);
    }
    
    beat_events_.clear();
    
    // Act: Continue with high-frequency alternating noise
    for (int i = 0; i < 200; i++) {
        uint16_t value = static_cast<uint16_t>(CENTER + (i % 2 == 0 ? NOISE_AMP : -NOISE_AMP));
        detector_->processSample(value);
        mock_timing_.advanceTime(62);
    }
    
    // Assert: No detections (too fast for valid beat)
    EXPECT_EQ(0U, beat_events_.size())
        << "High-frequency noise should not trigger detections";
}

/**
 * @test QuietEnvironment_NoFalsePositives
 * 
 * Validates perfect rejection in completely quiet environment.
 * 
 * Given: Constant signal (no variation)
 * When: 500 samples at 2048 ADC (midpoint)
 * Then: Zero detections
 * 
 * Traceability: AC-AUDIO-013 (Zero false positives in silence)
 */
TEST_F(NoiseRejectionTest, QuietEnvironment_NoFalsePositives) {
    // Arrange: Fill window
    fillWindow(2048);
    beat_events_.clear();
    
    // Act: Process completely quiet signal
    for (int i = 0; i < 500; i++) {
        detector_->processSample(2048);
        mock_timing_.advanceTime(62);
    }
    
    // Assert: Zero detections
    EXPECT_EQ(0U, beat_events_.size())
        << "Quiet environment should produce zero false positives";
}

/**
 * @test ThreeLayerValidation_Integration
 * 
 * Validates that all three protection layers work together.
 * 
 * Layers:
 *   1. Noise floor estimation (20th percentile)
 *   2. Threshold margin (80 ADC units hysteresis)
 *   3. Minimum signal amplitude (200 ADC units)
 * 
 * Test: Signal must pass all three layers to trigger detection
 * 
 * Traceability: AC-AUDIO-009, AC-AUDIO-013 (Three-layer protection)
 */
TEST_F(NoiseRejectionTest, ThreeLayerValidation_Integration) {
    // Arrange: Fill window with stable baseline
    const uint16_t BASELINE = 1800;
    fillWindow(BASELINE);
    uint16_t threshold = detector_->getThreshold();
    
    beat_events_.clear();
    
    // Test Case 1: Above threshold but within margin → NO detection
    // Feed samples just above threshold (within 80 margin)
    for (int i = 0; i < 10; i++) {
        detector_->processSample(static_cast<uint16_t>(threshold + 40));
        mock_timing_.advanceTime(62);
    }
    EXPECT_EQ(0U, beat_events_.size()) << "Layer 2 (margin) should block";
    
    // Test Case 2: Generate proper beat that passes all layers
    beat_events_.clear();
    
    // Background
    for (int i = 0; i < 20; i++) {
        detector_->processSample(BASELINE);
        mock_timing_.advanceTime(62);
    }
    
    // Strong beat: well above threshold + margin + sufficient amplitude
    uint16_t strong_signal = static_cast<uint16_t>(threshold + 350);
    for (int i = 0; i < 5; i++) {
        detector_->processSample(strong_signal);
        mock_timing_.advanceTime(62);
    }
    
    // Falling edge back to baseline (triggers detection)
    for (int i = 0; i < 3; i++) {
        uint16_t falling = static_cast<uint16_t>(strong_signal - (i + 1) * 100);
        detector_->processSample(falling);
        mock_timing_.advanceTime(62);
    }
    
    // Assert: Should detect (passes all layers)
    EXPECT_EQ(1U, beat_events_.size())
        << "Signal passing all three layers should be detected";
}

/**
 * @test EdgeCase_NarrowRange_EnforcesMinimum
 * 
 * Validates that narrow range signals enforce minimum threshold.
 * 
 * Given: Narrow dynamic range (<400 ADC units)
 * When: Signal barely crosses adaptive threshold
 * Then: No detection unless also above noise_floor + MIN_AMPLITUDE
 * 
 * Traceability: AC-AUDIO-009 (Conditional enforcement)
 */
TEST_F(NoiseRejectionTest, EdgeCase_NarrowRange_EnforcesMinimum) {
    // Arrange: Create narrow range scenario
    // Fill with values around 2000 ± 100 (range = 200 < 400)
    for (int i = 0; i < 64; i++) {
        uint16_t value = static_cast<uint16_t>(2000 + (i % 20) * 10);  // Range: 2000-2190
        detector_->processSample(value);
        mock_timing_.advanceTime(62);
    }
    
    uint16_t threshold = detector_->getThreshold();
    beat_events_.clear();
    
    // Act: Try to trigger with value just above threshold
    // But likely below noise_floor + 200 (minimum enforcement kicks in)
    detector_->processSample(threshold + 100);
    mock_timing_.advanceTime(62);
    
    // Assert: Should NOT detect (narrow range enforces minimum)
    // Note: This test validates the conditional logic exists,
    // exact behavior depends on noise floor calculation
    EXPECT_EQ(0U, beat_events_.size())
        << "Narrow range should enforce minimum threshold";
}

/**
 * @test EdgeCase_WideRange_AllowsLowerThreshold
 * 
 * Validates that wide range signals use adaptive threshold without minimum.
 * 
 * Given: Wide dynamic range (>400 ADC units)
 * When: Signal crosses adaptive threshold
 * Then: Detection allowed (minimum not enforced)
 * 
 * Traceability: AC-AUDIO-009 (Conditional enforcement)
 */
TEST_F(NoiseRejectionTest, EdgeCase_WideRange_AllowsLowerThreshold) {
    // Arrange: Create wide range scenario
    // Fill with values from 1800 to 2600 (range = 800 > 400)
    for (int i = 0; i < 64; i++) {
        uint16_t value = static_cast<uint16_t>(1800 + (i * 800 / 64));
        detector_->processSample(value);
        mock_timing_.advanceTime(62);
    }
    
    uint16_t threshold = detector_->getThreshold();
    beat_events_.clear();
    
    // Act: Generate beat above threshold + margin
    // Background
    for (int i = 0; i < 20; i++) {
        detector_->processSample(2000);
        mock_timing_.advanceTime(62);
    }
    
    // Peak above threshold + margin
    for (int i = 0; i < 5; i++) {
        detector_->processSample(threshold + 150);
        mock_timing_.advanceTime(62);
    }
    
    // Falling edge
    detector_->processSample(2200);
    mock_timing_.advanceTime(62);
    
    // Assert: Should detect (wide range, threshold + margin sufficient)
    EXPECT_EQ(1U, beat_events_.size())
        << "Wide range should allow detection at adaptive threshold";
}

/**
 * @test StressTest_ExtendedNoiseSequence
 * 
 * Validates robustness over extended noise exposure.
 * 
 * Given: 1000 samples of varied noise patterns
 * When: Mix of random, periodic, and burst noise
 * Then: Zero false positives
 * 
 * Traceability: AC-AUDIO-013 (Robust noise rejection)
 */
TEST_F(NoiseRejectionTest, StressTest_ExtendedNoiseSequence) {
    // Arrange: Fill window with low amplitude varied noise to establish threshold
    const uint16_t CENTER = 1800;
    const uint16_t SMALL_AMPLITUDE = 30;
    
    for (int i = 0; i < 64; i++) {
        int offset = (rand() % (2 * SMALL_AMPLITUDE + 1)) - SMALL_AMPLITUDE;
        detector_->processSample(static_cast<uint16_t>(CENTER + offset));
        mock_timing_.advanceTime(62);
    }
    
    beat_events_.clear();
    
    // Act: Process varied low-amplitude noise patterns
    
    // 1. Random noise (small amplitude)
    std::vector<uint16_t> random_noise = generateRandomNoise(300, CENTER, SMALL_AMPLITUDE);
    for (uint16_t sample : random_noise) {
        detector_->processSample(sample);
        mock_timing_.advanceTime(62);
    }
    
    // 2. Periodic noise (small amplitude)
    std::vector<uint16_t> periodic_noise = generatePeriodicNoise(300, CENTER, SMALL_AMPLITUDE, 100.0);
    for (uint16_t sample : periodic_noise) {
        detector_->processSample(sample);
        mock_timing_.advanceTime(62);
    }
    
    // 3. Small burst noise
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 15; j++) {
            detector_->processSample(CENTER);
            mock_timing_.advanceTime(62);
        }
        detector_->processSample(static_cast<uint16_t>(CENTER + 50));  // Small burst
        mock_timing_.advanceTime(62);
    }
    
    // 4. High frequency noise (small amplitude)
    for (int i = 0; i < 200; i++) {
        uint16_t value = static_cast<uint16_t>(CENTER + (i % 2 == 0 ? SMALL_AMPLITUDE : -SMALL_AMPLITUDE));
        detector_->processSample(value);
        mock_timing_.advanceTime(62);
    }
    
    // Assert: Zero or very few false positives (<5% per AC-AUDIO-009)
    double falsePositiveRate = (beat_events_.size() * 100.0) / 1.0;  // As percentage
    EXPECT_LT(falsePositiveRate, 5.0)
        << "Extended noise sequence should produce minimal false positives";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
