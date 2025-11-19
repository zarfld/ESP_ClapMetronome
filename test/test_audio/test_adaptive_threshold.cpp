/**
 * Test: AC-AUDIO-001 - Adaptive Threshold Calculation
 * 
 * Verifies adaptive threshold formula: threshold = 0.8 × (max - min) + min
 * 
 * Implements: #45 (DES-C-001 Audio Detection Engine)
 * Acceptance Criteria: AC-AUDIO-001
 * Test Method: Unit test with mock timing provider
 * 
 * TDD Status: RED ⏳ (Tests written, expecting failures)
 * 
 * Standards: IEEE 1012-2016 (V&V), XP Test-First Practice
 * 
 * See: https://github.com/zarfld/ESP_ClapMetronome/issues/45
 */

#include <gtest/gtest.h>
#include "../../src/audio/AudioDetection.h"
#include "../../test/mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * Test Fixture for Adaptive Threshold Tests
 * 
 * Sets up AudioDetection with mock timing provider
 */
class AdaptiveThresholdTest : public ::testing::Test {
protected:
    MockTimingProvider mock_timing_;
    AudioDetection* audio_;
    
    void SetUp() override {
        audio_ = new AudioDetection(&mock_timing_);
        audio_->init();
    }
    
    void TearDown() override {
        delete audio_;
    }
    
    /**
     * Helper: Feed samples to update window
     * Simulates ADC readings to establish min/max
     */
    void feedSamples(const std::vector<uint16_t>& samples) {
        for (uint16_t sample : samples) {
            audio_->processSample(sample);
        }
    }
};

/**
 * TEST 1: Initial threshold after init()
 * 
 * Given: Audio detection initialized
 * When: No samples processed
 * Then: Threshold = 50 (initial value from legacy code)
 */
TEST_F(AdaptiveThresholdTest, InitialThreshold) {
    // Arrange: Already done in SetUp()
    
    // Act: Query threshold
    uint16_t threshold = audio_->getThreshold();
    
    // Assert: Initial threshold
    EXPECT_EQ(50, threshold) << "Initial threshold should be 50 ADC units";
}

/**
 * TEST 2: Threshold calculation with known min/max
 * 
 * Given: Window with min=100, max=500
 * When: Threshold calculated
 * Then: threshold = 0.8 × (500 - 100) + 100 = 0.8 × 400 + 100 = 420
 */
TEST_F(AdaptiveThresholdTest, CalculationWithKnownRange) {
    // Arrange: Feed samples to establish min=100, max=500
    // Need 64 samples to fill window (DES-D-002 window size)
    std::vector<uint16_t> samples;
    for (int i = 0; i < 32; i++) {
        samples.push_back(100);  // Min value
        samples.push_back(500);  // Max value
    }
    
    // Act: Feed samples and query threshold
    feedSamples(samples);
    uint16_t threshold = audio_->getThreshold();
    
    // Assert: threshold = 0.8 × (500 - 100) + 100 = 420
    EXPECT_EQ(420, threshold) << "Threshold should be 0.8 × range + min";
}

/**
 * TEST 3: Threshold with narrow range (low dynamic)
 * 
 * Given: Window with min=200, max=230 (quiet environment)
 * When: Threshold calculated
 * Then: threshold = 0.8 × (230 - 200) + 200 = 0.8 × 30 + 200 = 224
 */
TEST_F(AdaptiveThresholdTest, NarrowRangeQuietEnvironment) {
    // Arrange: Feed samples with narrow range
    std::vector<uint16_t> samples;
    for (int i = 0; i < 64; i++) {
        samples.push_back((i % 2 == 0) ? 200 : 230);
    }
    
    // Act
    feedSamples(samples);
    uint16_t threshold = audio_->getThreshold();
    
    // Assert: threshold = 0.8 × 30 + 200 = 224
    EXPECT_EQ(224, threshold) << "Threshold should adapt to narrow range";
}

/**
 * TEST 4: Threshold with wide range (loud environment)
 * 
 * Given: Window with min=50, max=3000 (loud stage with claps)
 * When: Threshold calculated
 * Then: threshold = 0.8 × (3000 - 50) + 50 = 0.8 × 2950 + 50 = 2410
 */
TEST_F(AdaptiveThresholdTest, WideRangeLoudEnvironment) {
    // Arrange: Feed samples with wide range
    std::vector<uint16_t> samples;
    for (int i = 0; i < 32; i++) {
        samples.push_back(50);    // Background noise
        samples.push_back(3000);  // Clap peak
    }
    
    // Act
    feedSamples(samples);
    uint16_t threshold = audio_->getThreshold();
    
    // Assert: threshold = 0.8 × 2950 + 50 = 2410
    EXPECT_EQ(2410, threshold) << "Threshold should adapt to wide range";
}

/**
 * TEST 5: Threshold updates dynamically as samples change
 * 
 * Given: Initial quiet environment (min=100, max=200)
 * When: Environment becomes loud (min=100, max=1000)
 * Then: Threshold increases from 180 to 820
 */
TEST_F(AdaptiveThresholdTest, DynamicThresholdUpdate) {
    // Arrange Phase 1: Quiet environment
    std::vector<uint16_t> quiet_samples;
    for (int i = 0; i < 64; i++) {
        quiet_samples.push_back((i % 2 == 0) ? 100 : 200);
    }
    feedSamples(quiet_samples);
    
    uint16_t threshold_quiet = audio_->getThreshold();
    EXPECT_EQ(180, threshold_quiet) << "Initial quiet threshold: 0.8 × 100 + 100 = 180";
    
    // Act Phase 2: Loud environment
    std::vector<uint16_t> loud_samples;
    for (int i = 0; i < 64; i++) {
        loud_samples.push_back((i % 2 == 0) ? 100 : 1000);
    }
    feedSamples(loud_samples);
    
    uint16_t threshold_loud = audio_->getThreshold();
    
    // Assert: Threshold increased
    EXPECT_EQ(820, threshold_loud) << "Updated loud threshold: 0.8 × 900 + 100 = 820";
    EXPECT_GT(threshold_loud, threshold_quiet) << "Threshold should increase in loud environment";
}

/**
 * Test main entry point
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
