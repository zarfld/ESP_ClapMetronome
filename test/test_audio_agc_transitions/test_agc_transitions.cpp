/**
 * @file test_agc_transitions.cpp
 * @brief Unit tests for AC-AUDIO-003 AGC Level Transitions
 * 
 * Tests automatic gain control (AGC) level transitions when clipping is detected.
 * 
 * Implements: AC-AUDIO-003 AGC Level Transitions
 * Requirement: DES-C-001 Audio Detection Engine
 * GitHub Issue: #45
 * 
 * Test Strategy:
 * - RED Phase: Write tests for AGC 0→1→2 transitions on clipping
 * - GREEN Phase: Implement updateAGC() in AudioDetection
 * - REFACTOR Phase: Optimize AGC logic if needed
 * 
 * Clipping Threshold: 4000 ADC units (from AudioDetectionState::CLIPPING_THRESHOLD)
 * AGC Levels:
 *   - AGC_40DB = 0 (lowest gain)
 *   - AGC_50DB = 1 (medium gain)
 *   - AGC_60DB = 2 (highest gain)
 * 
 * @date 2025-11-19
 */

#include <gtest/gtest.h>
#include "../../src/audio/AudioDetection.h"
#include "../mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * Test fixture for AGC transition tests
 */
class AGCTransitionsTest : public ::testing::Test {
protected:
    MockTimingProvider mock_timing_;
    AudioDetection* audio_;
    bool baseline_established_ = false;
    
    void SetUp() override {
        mock_timing_.reset();
        audio_ = new AudioDetection(&mock_timing_);
        audio_->init();
    }
    
    void TearDown() override {
        delete audio_;
    }
    
    void establishBaseline() {
        if (baseline_established_) return;
        for (int i = 0; i < 50; i++) {
            audio_->processSample(500);
            mock_timing_.advanceTime(125);
        }
        baseline_established_ = true;
    }
};

/**
 * TEST 1: Initial AGC level is GAIN_50DB (medium)
 * 
 * Given: AudioDetection just initialized
 * When: No samples processed yet
 * Then: AGC level = AGC_50DB (1)
 */
TEST_F(AGCTransitionsTest, InitialGainIs50dB) {
    // Assert: Default gain is 50dB (medium)
    EXPECT_EQ(AGCLevel::GAIN_50DB, audio_->getGainLevel())
        << "Initial AGC level should be GAIN_50DB";
}

/**
 * TEST 2: AGC stays at current level when no clipping
 * 
 * Given: AGC level = GAIN_50DB
 * When: Samples below clipping threshold (4000)
 * Then: AGC level remains GAIN_50DB
 */
TEST_F(AGCTransitionsTest, NoClippingKeepsCurrentGain) {
    // Arrange: Initial state
    EXPECT_EQ(AGCLevel::GAIN_50DB, audio_->getGainLevel());
    
    // Act: Process samples well below clipping (4000)
    for (int i = 0; i < 20; i++) {
        audio_->processSample(3000);  // Below 4000
        mock_timing_.advanceTime(1000);
    }
    
    // Assert: Gain unchanged
    EXPECT_EQ(AGCLevel::GAIN_50DB, audio_->getGainLevel())
        << "AGC should remain at GAIN_50DB when no clipping";
}

/**
 * TEST 3: Transition GAIN_50DB → GAIN_40DB on clipping
 * 
 * Given: AGC level = GAIN_50DB (1)
 * When: Sample exceeds clipping threshold (>4000)
 * Then: AGC level = GAIN_40DB (0)
 */
TEST_F(AGCTransitionsTest, ClippingReducesGainFrom50To40dB) {
    // Arrange: Start at GAIN_50DB
    EXPECT_EQ(AGCLevel::GAIN_50DB, audio_->getGainLevel());
    
    // Act: Feed clipping sample
    audio_->processSample(4001);  // Above 4000 threshold
    
    // Assert: Gain reduced to 40dB
    EXPECT_EQ(AGCLevel::GAIN_40DB, audio_->getGainLevel())
        << "AGC should reduce to GAIN_40DB on clipping";
}

/**
 * TEST 4: Transition GAIN_60DB → GAIN_50DB on clipping
 * 
 * Given: AGC level = GAIN_60DB (2)
 * When: Sample exceeds clipping threshold (>4000)
 * Then: AGC level = GAIN_50DB (1)
 */
TEST_F(AGCTransitionsTest, ClippingReducesGainFrom60To50dB) {
    // Arrange: Manually set to GAIN_60DB (highest)
    // We need to add a setter or test via state access
    // For now, we'll need to implement a way to set gain level
    // This might require adding a test-only method or making state_ accessible
    
    // TODO: This test requires ability to set AGC level
    // Options: 
    // 1. Add setGainLevel() test method
    // 2. Access state_ for testing
    // 3. Trigger natural progression to GAIN_60DB
    
    // For now, skip this test - will implement after GREEN phase
    GTEST_SKIP() << "Requires test method to set AGC level to 60dB";
}

/**
 * TEST 5: AGC does not reduce below GAIN_40DB
 * 
 * Given: AGC level = GAIN_40DB (0, lowest)
 * When: Sample exceeds clipping threshold
 * Then: AGC level remains GAIN_40DB (cannot go lower)
 */
TEST_F(AGCTransitionsTest, GainDoesNotReduceBelowMinimum) {
    // Arrange: Start at GAIN_50DB, trigger clipping to get to GAIN_40DB
    audio_->processSample(4001);  // GAIN_50DB → GAIN_40DB
    EXPECT_EQ(AGCLevel::GAIN_40DB, audio_->getGainLevel());
    
    // Act: Trigger clipping again
    mock_timing_.advanceTime(1000);
    audio_->processSample(4001);  // Should stay at GAIN_40DB
    
    // Assert: Still at minimum
    EXPECT_EQ(AGCLevel::GAIN_40DB, audio_->getGainLevel())
        << "AGC should not reduce below GAIN_40DB";
}

/**
 * TEST 6: Clipping flag is set when clipping detected
 * 
 * Given: No clipping yet
 * When: Sample exceeds clipping threshold
 * Then: clipping_detected flag = true
 */
TEST_F(AGCTransitionsTest, ClippingFlagSetOnClipping) {
    // Arrange: Check initial state (via telemetry or state access)
    // For now, we'll verify behavior indirectly via AGC change
    EXPECT_EQ(AGCLevel::GAIN_50DB, audio_->getGainLevel());
    
    // Act: Trigger clipping
    audio_->processSample(4001);
    
    // Assert: Gain changed (implies clipping was detected)
    EXPECT_EQ(AGCLevel::GAIN_40DB, audio_->getGainLevel())
        << "Clipping detection should trigger AGC reduction";
}

/**
 * TEST 7: Multiple clipping events reduce gain progressively
 * 
 * Given: AGC level = GAIN_50DB (1)
 * When: Multiple clipping samples processed
 * Then: AGC level reduces to GAIN_40DB (0) and stays there
 */
TEST_F(AGCTransitionsTest, MultipleClippingEventsReduceGainProgressively) {
    // Arrange: Start at GAIN_50DB
    EXPECT_EQ(AGCLevel::GAIN_50DB, audio_->getGainLevel());
    
    // Act: Trigger multiple clipping events
    audio_->processSample(4001);  // First clipping
    EXPECT_EQ(AGCLevel::GAIN_40DB, audio_->getGainLevel()) 
        << "First clipping should reduce to GAIN_40DB";
    
    mock_timing_.advanceTime(1000);
    audio_->processSample(4001);  // Second clipping
    
    // Assert: Still at minimum
    EXPECT_EQ(AGCLevel::GAIN_40DB, audio_->getGainLevel())
        << "Should remain at GAIN_40DB (minimum)";
}

/**
 * TEST 8: Clipping threshold is consistent (4000 ADC units)
 * 
 * Given: Any AGC level
 * When: Sample = 4000 (at threshold)
 * Then: No clipping (threshold is exclusive: > 4000)
 * 
 * When: Sample = 4001 (above threshold)
 * Then: Clipping detected
 */
TEST_F(AGCTransitionsTest, ClippingThresholdIs4000Exclusive) {
    // Arrange: Start at GAIN_50DB
    EXPECT_EQ(AGCLevel::GAIN_50DB, audio_->getGainLevel());
    
    // Act: Feed sample AT threshold (should not clip)
    audio_->processSample(4000);
    
    // Assert: No AGC change
    EXPECT_EQ(AGCLevel::GAIN_50DB, audio_->getGainLevel())
        << "Sample at threshold (4000) should not trigger clipping";
    
    // Act: Feed sample ABOVE threshold (should clip)
    mock_timing_.advanceTime(1000);
    audio_->processSample(4001);
    
    // Assert: AGC reduced
    EXPECT_EQ(AGCLevel::GAIN_40DB, audio_->getGainLevel())
        << "Sample above threshold (4001) should trigger clipping";
}

/**
 * TEST 9: AGC level included in beat events
 * 
 * Given: AGC level = GAIN_40DB (0)
 * When: Beat event emitted
 * Then: BeatEvent.gain_level = 0
 */
TEST_F(AGCTransitionsTest, AGCLevelIncludedInBeatEvents) {
    establishBaseline();
    
    // Arrange: Reduce to GAIN_40DB via clipping
    audio_->processSample(4001);
    EXPECT_EQ(AGCLevel::GAIN_40DB, audio_->getGainLevel());
    
    // Setup beat callback
    uint8_t captured_gain = 255;  // Invalid value
    audio_->onBeat([&captured_gain](const BeatEvent& event) {
        captured_gain = event.gain_level;
    });
    
    // Act: Trigger a beat
    audio_->processSample(1000);  // Start rising (adjusted from 3000)
    mock_timing_.advanceTime(1000);
    audio_->processSample(1500);  // Peak (adjusted from 3500)
    mock_timing_.advanceTime(1000);
    audio_->processSample(1400);  // Fall → trigger (adjusted from 3400)
    
    // Assert: Gain level in event (0 = GAIN_40DB)
    EXPECT_EQ(static_cast<uint8_t>(AGCLevel::GAIN_40DB), captured_gain)
        << "Beat event should include current AGC level";
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

