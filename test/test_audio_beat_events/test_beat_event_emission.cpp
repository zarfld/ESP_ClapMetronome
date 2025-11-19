/**
 * TDD Cycle 4: AC-AUDIO-004 Beat Event Emission Tests
 * 
 * RED Phase: Test beat event callback with all fields validated
 * 
 * Acceptance Criteria (AC-AUDIO-004):
 *   Given: AudioDetection with registered callback
 *   When: Beat detected (state machine TRIGGERED → DEBOUNCE)
 *   Then: Callback fired with correct fields:
 *     - timestamp_us = detection timestamp
 *     - amplitude = peak ADC value
 *     - threshold = current adaptive threshold
 *     - gain_level = current AGC level (0/1/2)
 *     - kick_only = true if rise_time > 4ms
 * 
 * Implements: AC-AUDIO-004 Beat Event Emission
 * Part of: DES-C-001 Audio Detection Engine (Wave 2.1)
 * GitHub Issue: #45
 */

#include <gtest/gtest.h>
#include "../../src/audio/AudioDetection.h"
#include "../../test/mocks/MockTimingProvider.h"
#include <memory>

using namespace clap_metronome;

// ===== Test Fixture =====

class BeatEventEmissionTest : public ::testing::Test {
protected:
    MockTimingProvider mock_timing_;
    AudioDetection* audio_;
    
    void SetUp() override {
        audio_ = new AudioDetection(&mock_timing_);
        audio_->init();
        mock_timing_.setTimestamp(0);
    }
    
    void TearDown() override {
        delete audio_;
    }
    
    // Helper: Simulate beat detection sequence (simplified - matches StateMachineTest pattern)
    void simulateBeat(uint16_t amplitude, uint64_t beat_timestamp_us) {
        mock_timing_.setTimestamp(beat_timestamp_us - 3000);  // 3ms before peak
        
        // Rising edge: amplitude crosses threshold (IDLE → RISING)
        audio_->processSample(amplitude);
        
        // Continue rising
        mock_timing_.advanceTime(1000);  // 1ms later
        audio_->processSample(amplitude + 100);
        
        // Peak
        mock_timing_.advanceTime(1000);  // 1ms later
        audio_->processSample(amplitude + 200);
        
        // Start falling (RISING → TRIGGERED, emits beat event)
        mock_timing_.setTimestamp(beat_timestamp_us);
        audio_->processSample(amplitude + 100);
    }
};

// ===== Test Cases =====

/**
 * Test: CallbackFiredOnBeatDetection
 * 
 * Given: AudioDetection with registered callback
 * When: Beat detected (amplitude crosses threshold)
 * Then: Callback invoked exactly once
 */
TEST_F(BeatEventEmissionTest, CallbackFiredOnBeatDetection) {
    int callback_count = 0;
    
    audio_->onBeat([&callback_count](const BeatEvent&) {
        callback_count++;
    });
    
    // Simulate beat
    simulateBeat(3000, 1000000);  // 3000 ADC units at 1 second
    
    EXPECT_EQ(1, callback_count);
}

/**
 * Test: NoCallbackWhenNotRegistered
 * 
 * Given: AudioDetection without registered callback
 * When: Beat detected
 * Then: No crash or error (graceful handling)
 */
TEST_F(BeatEventEmissionTest, NoCallbackWhenNotRegistered) {
    // No callback registered
    EXPECT_FALSE(audio_->hasBeatCallback());
    
    // Simulate beat - should not crash
    EXPECT_NO_THROW(simulateBeat(3000, 1000000));
}

/**
 * Test: TimestampFieldCorrect
 * 
 * Given: Beat detected at specific timestamp
 * When: Callback invoked
 * Then: event.timestamp_us matches detection time
 */
TEST_F(BeatEventEmissionTest, TimestampFieldCorrect) {
    const uint64_t expected_timestamp = 5000000;  // 5 seconds
    uint64_t captured_timestamp = 0;
    
    audio_->onBeat([&captured_timestamp](const BeatEvent& event) {
        captured_timestamp = event.timestamp_us;
    });
    
    simulateBeat(3000, expected_timestamp);
    
    EXPECT_EQ(expected_timestamp, captured_timestamp);
}

/**
 * Test: AmplitudeFieldCorrect
 * 
 * Given: Beat with peak amplitude = 3700 ADC units (starting at 3500, peaks at 3700)
 * When: Callback invoked
 * Then: event.amplitude = 3700 (the actual peak)
 */
TEST_F(BeatEventEmissionTest, AmplitudeFieldCorrect) {
    const uint16_t start_amplitude = 3500;
    const uint16_t expected_peak = start_amplitude + 200;  // simulateBeat peaks at +200
    uint16_t captured_amplitude = 0;
    
    audio_->onBeat([&captured_amplitude](const BeatEvent& event) {
        captured_amplitude = event.amplitude;
    });
    
    simulateBeat(start_amplitude, 1000000);
    
    EXPECT_EQ(expected_peak, captured_amplitude);
}

/**
 * Test: ThresholdFieldCorrect
 * 
 * Given: Adaptive threshold calculated from baseline
 * When: Beat detected and callback invoked
 * Then: event.threshold = current adaptive threshold
 */
TEST_F(BeatEventEmissionTest, ThresholdFieldCorrect) {
    uint16_t captured_threshold = 0;
    
    audio_->onBeat([&captured_threshold](const BeatEvent& event) {
        captured_threshold = event.threshold;
    });
    
    simulateBeat(3000, 1000000);
    
    // Threshold should be > 0 (adaptive threshold computed)
    EXPECT_GT(captured_threshold, 0);
    
    // Should match AudioDetection's getThreshold()
    EXPECT_EQ(audio_->getThreshold(), captured_threshold);
}

/**
 * Test: GainLevelFieldCorrect
 * 
 * Given: Current gain level = GAIN_50DB (default)
 * When: Beat detected
 * Then: event.gain_level = 1 (50dB)
 */
TEST_F(BeatEventEmissionTest, GainLevelFieldCorrect) {
    uint8_t captured_gain = 0xFF;  // Invalid sentinel
    
    audio_->onBeat([&captured_gain](const BeatEvent& event) {
        captured_gain = event.gain_level;
    });
    
    // Default gain should be GAIN_50DB = 1
    EXPECT_EQ(AGCLevel::GAIN_50DB, audio_->getGainLevel());
    
    simulateBeat(3000, 1000000);
    
    // Captured gain should match
    EXPECT_EQ(static_cast<uint8_t>(AGCLevel::GAIN_50DB), captured_gain);
}

/**
 * Test: KickOnlyFalseForFastRiseTime
 * 
 * Given: Beat with rise time = 3ms (fast attack, like clap)
 * When: Beat detected
 * Then: event.kick_only = false (3ms < 4ms threshold)
 */
TEST_F(BeatEventEmissionTest, KickOnlyFalseForFastRiseTime) {
    bool captured_kick_only = true;  // Start with opposite
    
    audio_->onBeat([&captured_kick_only](const BeatEvent& event) {
        captured_kick_only = event.kick_only;
    });
    
    // simulateBeat creates 3ms rise time (3 samples @ 1ms each)
    // 3ms < 4ms threshold → kick_only should be false
    simulateBeat(3000, 1000000);
    
    EXPECT_FALSE(captured_kick_only);
}

/**
 * Test: KickOnlyTrueForSlowRiseTime
 * 
 * Given: Beat with rise time = 5ms (slow attack, like kick drum)
 * When: Beat detected
 * Then: event.kick_only = true (5ms > 4ms threshold)
 */
TEST_F(BeatEventEmissionTest, KickOnlyTrueForSlowRiseTime) {
    bool captured_kick_only = false;  // Start with opposite
    
    audio_->onBeat([&captured_kick_only](const BeatEvent& event) {
        captured_kick_only = event.kick_only;
    });
    
    // Manually create 5ms rise time (>4ms threshold)
    mock_timing_.setTimestamp(0);
    
    // Rising edge starts (IDLE → RISING)
    audio_->processSample(3000);
    
    // Continue rising for 5ms total
    mock_timing_.advanceTime(1000);  // 1ms
    audio_->processSample(3100);
    
    mock_timing_.advanceTime(1000);  // 2ms
    audio_->processSample(3200);
    
    mock_timing_.advanceTime(1000);  // 3ms
    audio_->processSample(3300);
    
    mock_timing_.advanceTime(1000);  // 4ms
    audio_->processSample(3400);
    
    mock_timing_.advanceTime(1000);  // 5ms - peak reached
    audio_->processSample(3500);
    
    // Start falling (RISING → TRIGGERED, rise_time = 5ms > 4ms)
    mock_timing_.advanceTime(1000);  // 6ms
    audio_->processSample(3400);
    
    EXPECT_TRUE(captured_kick_only);
}

/**
 * Test: MultipleBeatsMultipleCallbacks
 * 
 * Given: Multiple beats detected over time
 * When: Each beat crosses threshold
 * Then: Callback invoked for each beat with correct timestamps
 */
TEST_F(BeatEventEmissionTest, MultipleBeatsMultipleCallbacks) {
    std::vector<uint64_t> captured_timestamps;
    
    audio_->onBeat([&captured_timestamps](const BeatEvent& event) {
        captured_timestamps.push_back(event.timestamp_us);
    });
    
    // Simulate 3 beats at different times
    simulateBeat(3000, 1000000);   // 1 second
    simulateBeat(3000, 2000000);   // 2 seconds
    simulateBeat(3000, 3000000);   // 3 seconds
    
    ASSERT_EQ(3u, captured_timestamps.size());
    EXPECT_EQ(1000000u, captured_timestamps[0]);
    EXPECT_EQ(2000000u, captured_timestamps[1]);
    EXPECT_EQ(3000000u, captured_timestamps[2]);
}

/**
 * Test: AllFieldsPopulatedSimultaneously
 * 
 * Given: Beat detection
 * When: Callback invoked
 * Then: All BeatEvent fields contain valid data (no zeros/defaults)
 */
TEST_F(BeatEventEmissionTest, AllFieldsPopulatedSimultaneously) {
    BeatEvent captured_event = {};  // Zero-initialize
    
    audio_->onBeat([&captured_event](const BeatEvent& event) {
        captured_event = event;
    });
    
    simulateBeat(3500, 2000000);
    
    // Validate all fields populated
    EXPECT_EQ(2000000u, captured_event.timestamp_us);
    EXPECT_EQ(3700, captured_event.amplitude);  // Peak is start+200
    EXPECT_GT(captured_event.threshold, 0);  // Should be computed
    EXPECT_LE(captured_event.gain_level, 2);  // Valid range: 0-2
    EXPECT_FALSE(captured_event.kick_only);  // 3ms rise time < 4ms threshold
}

/**
 * Test: CallbackReplaceable
 * 
 * Given: Initial callback registered
 * When: New callback registered (replaces old)
 * Then: Only new callback is invoked
 */
TEST_F(BeatEventEmissionTest, CallbackReplaceable) {
    int old_callback_count = 0;
    int new_callback_count = 0;
    
    // Register first callback
    audio_->onBeat([&old_callback_count](const BeatEvent&) {
        old_callback_count++;
    });
    
    // Replace with new callback
    audio_->onBeat([&new_callback_count](const BeatEvent&) {
        new_callback_count++;
    });
    
    simulateBeat(3000, 1000000);
    
    EXPECT_EQ(0, old_callback_count);  // Old callback not invoked
    EXPECT_EQ(1, new_callback_count);  // New callback invoked
}

/**
 * Test: DebouncePreventsDuplicateCallbacks
 * 
 * Given: Beat detected, system in DEBOUNCE state
 * When: Additional samples arrive during debounce period
 * Then: Callback invoked only once (no duplicates)
 */
TEST_F(BeatEventEmissionTest, DebouncePreventsDuplicateCallbacks) {
    int callback_count = 0;
    
    audio_->onBeat([&callback_count](const BeatEvent&) {
        callback_count++;
    });
    
    // Simulate beat
    simulateBeat(3000, 1000000);
    
    EXPECT_EQ(1, callback_count);
    
    // Try to trigger again immediately (during debounce)
    for (int i = 0; i < 20; i++) {
        audio_->processSample(3500);
        mock_timing_.advanceTime(1000);  // 1ms steps
    }
    
    // Should still be 1 (debounce prevents duplicate)
    EXPECT_EQ(1, callback_count);
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

