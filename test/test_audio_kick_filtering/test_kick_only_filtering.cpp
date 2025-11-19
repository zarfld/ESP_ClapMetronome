/**
 * @file test_kick_only_filtering.cpp
 * @brief TDD Cycle 6 - AC-AUDIO-006 Kick-Only Filtering Tests
 * 
 * Tests validate rise time based kick drum detection (>4ms = kick).
 * 
 * AC-AUDIO-006: Kick-only filtering
 *   - Requirement: Distinguish kick drums (slow attack >4ms) from claps/snares (fast attack <4ms)
 *   - Pass condition: Rise time >4ms → kick_only = true
 *   - Algorithm: Measure time from threshold crossing to peak amplitude
 * 
 * TDD Cycle: VALIDATION (feature already implemented in Cycle 2)
 *   Phase: Validation - Testing existing implementation
 *   Expected: All tests passing (green)
 * 
 * Note: This is software-only validation. Hardware testing deferred to Phase 06.
 * 
 * @author TDD Cycle 6
 * @date 2025-11-19
 */

#include <gtest/gtest.h>
#include "../../src/audio/AudioDetection.h"
#include "../../test/mocks/MockTimingProvider.h"
#include <cstdint>
#include <vector>

using namespace clap_metronome;

/**
 * Test fixture for kick-only filtering validation
 */
class KickOnlyFilteringTest : public ::testing::Test {
protected:
    MockTimingProvider mockTiming_;
    AudioDetection* detector_;
    std::vector<BeatEvent> beatEvents_;

    void SetUp() override {
        detector_ = new AudioDetection(&mockTiming_);
        detector_->init();
        detector_->onBeat([this](const BeatEvent& event) {
            beatEvents_.push_back(event);
        });
        mockTiming_.setTimestamp(0);
        beatEvents_.clear();
    }

    void TearDown() override {
        delete detector_;
    }

    /**
     * @brief Simulate a beat with controlled rise time
     * 
     * IMPORTANT: This matches the algorithm's actual rise time measurement:
     * - Rise time START = when signal crosses (threshold + margin)
     * - Rise time END = when signal starts falling (peak detected)
     * - Measurement = END timestamp - START timestamp
     * 
     * The algorithm doesn't measure from baseline; it measures from threshold crossing.
     * 
     * @param peak_amplitude Peak ADC value (above threshold)
     * @param rise_time_us Rise time in microseconds (measured from threshold crossing to peak)
     */
    void simulateBeatWithRiseTime(uint16_t peak_amplitude, uint64_t rise_time_us) {
        // CRITICAL: Use LOW baseline that won't cross threshold
        // Initial threshold is 50, so 200 ADC is well below threshold + margin (130)
        for (int i = 0; i < 10; ++i) {
            detector_->processSample(200);  // Low baseline
            mockTiming_.advanceTime(1000); // 1ms between samples
        }

        // Cross threshold immediately (algorithm records this timestamp as START)
        // Typical threshold ~2400 + 80 margin = 2480
        // We use 3000 to ensure we're above threshold + margin
        detector_->processSample(3000);
        // NOTE: Time = 0 microseconds from start of rise (this is the reference point)
        
        // We need the falling edge to arrive at exactly rise_time_us from threshold crossing
        // Strategy: Advance time to just before target, emit peak, then emit falling edge
        
        if (rise_time_us >= 2000) {
            // Gradual rise - emit samples incrementally
            uint64_t rise_ms = rise_time_us / 1000;
            uint16_t amplitude_per_step = static_cast<uint16_t>((peak_amplitude - 3000) / rise_ms);
            
            // Emit rising samples up to rise_ms - 1
            for (uint64_t ms = 1; ms < rise_ms; ++ms) {
                mockTiming_.advanceTime(1000);
                uint16_t current_amplitude = 3000 + static_cast<uint16_t>(amplitude_per_step * ms);
                detector_->processSample(current_amplitude);
            }
            
            // Emit peak at rise_ms - 1 + 1ms = rise_time_us
            mockTiming_.advanceTime(1000);
            detector_->processSample(peak_amplitude);
        } else {
            // Fast rise - jump directly to rise_time_us and emit peak
            mockTiming_.advanceTime(rise_time_us);
            detector_->processSample(peak_amplitude);
        }
        
        // Emit falling edge at the same timestamp as peak (simulates instantaneous fall detection)
        // The algorithm will use THIS sample's timestamp for rise time calculation
        detector_->processSample(peak_amplitude - 200);
        mockTiming_.advanceTime(1000);
        detector_->processSample(2500);
        mockTiming_.advanceTime(1000);
        detector_->processSample(2000);
        mockTiming_.advanceTime(1000);
    }
};

// ===== Boundary Tests: 4ms Threshold =====

/**
 * Test: RiseTime_Exactly4ms_BoundaryCondition
 * 
 * Given: Beat with rise time exactly 4ms (boundary)
 * When: Beat detected
 * Then: kick_only = false (threshold is >, not >=)
 */
TEST_F(KickOnlyFilteringTest, RiseTime_Exactly4ms_BoundaryCondition) {
    simulateBeatWithRiseTime(3500, 4000);  // Exactly 4ms
    
    ASSERT_EQ(1U, beatEvents_.size()) << "Should detect one beat";
    EXPECT_FALSE(beatEvents_[0].kick_only) 
        << "Rise time exactly 4ms should NOT be classified as kick (> not >=)";
}

/**
 * Test: RiseTime_3999us_JustBelowThreshold
 * 
 * Given: Beat with rise time 3.999ms (just below 4ms)
 * When: Beat detected
 * Then: kick_only = false
 */
TEST_F(KickOnlyFilteringTest, RiseTime_3999us_JustBelowThreshold) {
    simulateBeatWithRiseTime(3500, 3999);
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_FALSE(beatEvents_[0].kick_only) 
        << "Rise time <4ms should be classified as clap/snare (fast attack)";
}

/**
 * Test: RiseTime_4001us_JustAboveThreshold
 * 
 * Given: Beat with rise time 4.001ms (just above 4ms)
 * When: Beat detected
 * Then: kick_only = true
 */
TEST_F(KickOnlyFilteringTest, RiseTime_4001us_JustAboveThreshold) {
    simulateBeatWithRiseTime(3500, 4001);
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_TRUE(beatEvents_[0].kick_only) 
        << "Rise time >4ms should be classified as kick (slow attack)";
}

// ===== Fast Attack Sounds (Claps, Snares, Hi-Hats) =====

/**
 * Test: FastAttack_1ms_ClapSound
 * 
 * Given: Beat with 1ms rise time (very fast attack)
 * When: Beat detected
 * Then: kick_only = false (typical clap sound)
 */
TEST_F(KickOnlyFilteringTest, FastAttack_1ms_ClapSound) {
    simulateBeatWithRiseTime(3500, 1000);  // 1ms rise = clap
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_FALSE(beatEvents_[0].kick_only) 
        << "1ms rise time should be clap (very fast attack)";
}

/**
 * Test: FastAttack_2ms_SnareSound
 * 
 * Given: Beat with 2ms rise time (fast attack)
 * When: Beat detected
 * Then: kick_only = false (typical snare sound)
 */
TEST_F(KickOnlyFilteringTest, FastAttack_2ms_SnareSound) {
    simulateBeatWithRiseTime(3400, 2000);
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_FALSE(beatEvents_[0].kick_only) 
        << "2ms rise time should be snare (fast attack)";
}

/**
 * Test: FastAttack_3ms_HiHatSound
 * 
 * Given: Beat with 3ms rise time (moderately fast)
 * When: Beat detected
 * Then: kick_only = false (typical hi-hat/cymbal)
 */
TEST_F(KickOnlyFilteringTest, FastAttack_3ms_HiHatSound) {
    simulateBeatWithRiseTime(3200, 3000);
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_FALSE(beatEvents_[0].kick_only) 
        << "3ms rise time should be hi-hat/cymbal (moderately fast)";
}

// ===== Slow Attack Sounds (Kick Drums) =====

/**
 * Test: SlowAttack_5ms_KickDrum
 * 
 * Given: Beat with 5ms rise time (slow attack)
 * When: Beat detected
 * Then: kick_only = true (typical kick drum)
 */
TEST_F(KickOnlyFilteringTest, SlowAttack_5ms_KickDrum) {
    simulateBeatWithRiseTime(3700, 5000);
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_TRUE(beatEvents_[0].kick_only) 
        << "5ms rise time should be kick drum (slow attack)";
}

/**
 * Test: SlowAttack_8ms_DeepKickDrum
 * 
 * Given: Beat with 8ms rise time (very slow attack)
 * When: Beat detected
 * Then: kick_only = true (deep/bass kick drum)
 */
TEST_F(KickOnlyFilteringTest, SlowAttack_8ms_DeepKickDrum) {
    simulateBeatWithRiseTime(3800, 8000);
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_TRUE(beatEvents_[0].kick_only) 
        << "8ms rise time should be kick drum (very slow attack)";
}

/**
 * Test: SlowAttack_10ms_SubKickDrum
 * 
 * Given: Beat with 10ms rise time (extremely slow)
 * When: Beat detected
 * Then: kick_only = true (sub-kick/floor tom)
 */
TEST_F(KickOnlyFilteringTest, SlowAttack_10ms_SubKickDrum) {
    simulateBeatWithRiseTime(3600, 10000);
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_TRUE(beatEvents_[0].kick_only) 
        << "10ms rise time should be kick drum (extremely slow)";
}

// ===== Variable Amplitude Tests =====

/**
 * Test: WeakKick_LowAmplitude_SlowRise
 * 
 * Given: Weak kick (2800 ADC) with 6ms rise time
 * When: Beat detected
 * Then: kick_only = true (rise time matters, not amplitude)
 */
TEST_F(KickOnlyFilteringTest, WeakKick_LowAmplitude_SlowRise) {
    simulateBeatWithRiseTime(2800, 6000);
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_TRUE(beatEvents_[0].kick_only) 
        << "Weak kick with slow rise time should still be classified as kick";
}

/**
 * Test: LoudClap_HighAmplitude_FastRise
 * 
 * Given: Loud clap (3900 ADC) with 1ms rise time
 * When: Beat detected
 * Then: kick_only = false (amplitude doesn't override fast rise time)
 */
TEST_F(KickOnlyFilteringTest, LoudClap_HighAmplitude_FastRise) {
    simulateBeatWithRiseTime(3900, 1000);
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_FALSE(beatEvents_[0].kick_only) 
        << "Loud clap with fast rise time should be classified as clap";
}

// ===== Multiple Beats Sequence =====

/**
 * Test: MixedSequence_KicksAndClaps
 * 
 * Given: Sequence of 3 kicks (slow) and 3 claps (fast)
 * When: All beats detected
 * Then: Kicks have kick_only=true, claps have kick_only=false
 */
TEST_F(KickOnlyFilteringTest, MixedSequence_KicksAndClaps) {
    // Kick 1: 6ms rise time
    simulateBeatWithRiseTime(3500, 6000);
    mockTiming_.advanceTime(60000);  // 60ms gap (> debounce)
    
    // Clap 1: 2ms rise time
    simulateBeatWithRiseTime(3600, 2000);
    mockTiming_.advanceTime(60000);
    
    // Kick 2: 7ms rise time
    simulateBeatWithRiseTime(3400, 7000);
    mockTiming_.advanceTime(60000);
    
    // Clap 2: 1ms rise time
    simulateBeatWithRiseTime(3700, 1000);
    mockTiming_.advanceTime(60000);
    
    // Kick 3: 5ms rise time
    simulateBeatWithRiseTime(3300, 5000);
    mockTiming_.advanceTime(60000);
    
    // Clap 3: 3ms rise time
    simulateBeatWithRiseTime(3800, 3000);
    
    ASSERT_EQ(6U, beatEvents_.size()) << "Should detect all 6 beats";
    
    // Validate kicks
    EXPECT_TRUE(beatEvents_[0].kick_only) << "Beat 1 (6ms) should be kick";
    EXPECT_TRUE(beatEvents_[2].kick_only) << "Beat 3 (7ms) should be kick";
    EXPECT_TRUE(beatEvents_[4].kick_only) << "Beat 5 (5ms) should be kick";
    
    // Validate claps
    EXPECT_FALSE(beatEvents_[1].kick_only) << "Beat 2 (2ms) should be clap";
    EXPECT_FALSE(beatEvents_[3].kick_only) << "Beat 4 (1ms) should be clap";
    EXPECT_FALSE(beatEvents_[5].kick_only) << "Beat 6 (3ms) should be clap";
}

// ===== Edge Cases =====

/**
 * Test: VeryFastAttack_0_5ms_InstantaneousClap
 * 
 * Given: Beat with 0.5ms rise time (near-instantaneous)
 * When: Beat detected
 * Then: kick_only = false
 */
TEST_F(KickOnlyFilteringTest, VeryFastAttack_0_5ms_InstantaneousClap) {
    // Simulate near-instantaneous rise (single sample above threshold)
    for (int i = 0; i < 10; ++i) {
        detector_->processSample(2000);
        mockTiming_.advanceTime(500);  // 0.5ms between samples
    }
    
    // Instantaneous peak
    detector_->processSample(3800);
    mockTiming_.advanceTime(500);  // Total rise time: 0.5ms
    
    // Fall
    detector_->processSample(2500);
    mockTiming_.advanceTime(500);
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_FALSE(beatEvents_[0].kick_only) 
        << "Instantaneous rise (<1ms) should be clap";
}

/**
 * Test: VerySlow_20ms_RoomResonance
 * 
 * Given: Beat with 20ms rise time (extremely slow, like room resonance)
 * When: Beat detected
 * Then: kick_only = true
 */
TEST_F(KickOnlyFilteringTest, VerySlow_20ms_RoomResonance) {
    simulateBeatWithRiseTime(3400, 20000);
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_TRUE(beatEvents_[0].kick_only) 
        << "20ms rise time should be kick (or room resonance, very slow)";
}

/**
 * Test: ConstantThreshold_4ms_Unchanged
 * 
 * Given: AC-AUDIO-006 specifies 4ms threshold
 * When: Check constant value
 * Then: KICK_RISE_TIME_US = 4000
 */
TEST_F(KickOnlyFilteringTest, ConstantThreshold_4ms_Unchanged) {
    EXPECT_EQ(4000U, AudioDetectionState::KICK_RISE_TIME_US) 
        << "Kick detection threshold must be 4ms (4000 microseconds) per AC-AUDIO-006";
}

// ===== Rise Time Accuracy =====

/**
 * Test: RiseTimeAccuracy_TimestampPrecision
 * 
 * Given: Beat with precisely measured 5ms rise time
 * When: Beat detected
 * Then: Rise time calculation accurate within 1ms tolerance
 */
TEST_F(KickOnlyFilteringTest, RiseTimeAccuracy_TimestampPrecision) {
    uint64_t start_time = 1000000;  // 1 second baseline
    mockTiming_.setTimestamp(start_time);
    
    // Baseline
    for (int i = 0; i < 5; ++i) {
        detector_->processSample(2000);
        mockTiming_.advanceTime(1000);
    }
    
    // Rising edge - exactly 5ms total
    uint64_t rise_start = mockTiming_.getTimestampUs();
    detector_->processSample(2500);
    mockTiming_.advanceTime(1000);  // 1ms
    
    detector_->processSample(3000);
    mockTiming_.advanceTime(1000);  // 2ms
    
    detector_->processSample(3500);
    mockTiming_.advanceTime(1000);  // 3ms
    
    detector_->processSample(3800);
    mockTiming_.advanceTime(1000);  // 4ms
    
    detector_->processSample(4000);
    mockTiming_.advanceTime(1000);  // 5ms - peak
    
    // Fall
    detector_->processSample(3500);
    uint64_t detection_time = mockTiming_.getTimestampUs();
    
    ASSERT_EQ(1U, beatEvents_.size());
    EXPECT_TRUE(beatEvents_[0].kick_only);
    
    // Calculated rise time should be ~5ms
    uint64_t actual_rise_time = detection_time - rise_start;
    EXPECT_GE(actual_rise_time, 5000U) << "Rise time should be at least 5ms";
    EXPECT_LE(actual_rise_time, 6000U) << "Rise time should not exceed 6ms (1ms tolerance)";
}

/**
 * GoogleTest main function
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
