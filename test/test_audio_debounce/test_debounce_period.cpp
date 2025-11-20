/**
 * TDD Cycle 5: AC-AUDIO-005 Debounce Period Tests
 * 
 * VALIDATION Phase: Test 50ms debounce period enforcement
 * 
 * Acceptance Criteria (AC-AUDIO-005):
 *   Given: Beat detected at timestamp T
 *   When: Additional beats occur within 50ms
 *   Then: No beat events emitted until 50ms elapsed
 *   
 * Requirements:
 *   - DEBOUNCE_PERIOD_US = 50000 (50ms)
 *   - No beats detected during debounce window
 *   - State remains DEBOUNCE until period expires
 *   - After 50ms, return to IDLE and detect new beats
 * 
 * Implements: AC-AUDIO-005 Debounce Period
 * Part of: DES-C-001 Audio Detection Engine (Wave 2.1)
 * GitHub Issue: #45
 */

#include <gtest/gtest.h>
#include "../../src/audio/AudioDetection.h"
#include "../../test/mocks/MockTimingProvider.h"

using namespace clap_metronome;

// ===== Test Fixture =====

class DebouncePeriodTest : public ::testing::Test {
protected:
    MockTimingProvider mock_timing_;
    AudioDetection* audio_;
    bool baseline_established_ = false;
    
    void SetUp() override {
        audio_ = new AudioDetection(&mock_timing_);
        audio_->init();
        mock_timing_.setTimestamp(0);
    }
    
    void TearDown() override {
        delete audio_;
    }
    
    /**
     * Helper: Establish baseline window (50 samples @ 500)
     * Creates: min=500, max=500, threshold=500
     */
    void establishBaseline() {
        if (baseline_established_) return;
        for (int i = 0; i < 50; i++) {
            audio_->processSample(500);
            mock_timing_.advanceTime(125);
        }
        baseline_established_ = true;
    }
    
    /**
     * Helper: Trigger a beat at current timestamp
     * Returns: timestamp when beat was triggered
     */
    uint64_t triggerBeat() {
        establishBaseline();
        
        uint64_t beat_timestamp = mock_timing_.getTimestampUs() + 3000;
        
        // Rising edge: amplitude crosses threshold (IDLE → RISING)
        audio_->processSample(1000);
        
        // Continue rising
        mock_timing_.advanceTime(1000);  // 1ms later
        audio_->processSample(1100);
        
        // Peak
        mock_timing_.advanceTime(1000);  // 1ms later
        audio_->processSample(1200);
        
        // Start falling (RISING → TRIGGERED, emits beat event)
        mock_timing_.advanceTime(1000);  // 1ms later
        audio_->processSample(1100);
        
        // Should now be in TRIGGERED
        EXPECT_EQ(DetectionState::TRIGGERED, audio_->getState());
        
        // Next sample moves to DEBOUNCE
        mock_timing_.advanceTime(100);
        audio_->processSample(500);  // Back to baseline
        EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState());
        
        return beat_timestamp;
    }
};

// ===== Test Cases =====

/**
 * Test: DebouncePeriodIs50Milliseconds
 * 
 * Given: System constants
 * When: Check DEBOUNCE_PERIOD_US
 * Then: Value equals 50000 microseconds (50ms)
 */
TEST_F(DebouncePeriodTest, DebouncePeriodIs50Milliseconds) {
    EXPECT_EQ(50000u, AudioDetectionState::DEBOUNCE_PERIOD_US)
        << "Debounce period must be 50ms (50000us)";
}

/**
 * Test: NoBeatsDetectedDuringDebounce
 * 
 * Given: Beat detected, system in DEBOUNCE state
 * When: High amplitude samples arrive during 50ms window
 * Then: No additional beat events emitted
 */
TEST_F(DebouncePeriodTest, NoBeatsDetectedDuringDebounce) {
    int beat_count = 0;
    audio_->onBeat([&beat_count](const BeatEvent&) {
        beat_count++;
    });
    
    // Trigger first beat
    triggerBeat();
    EXPECT_EQ(1, beat_count) << "First beat should be detected";
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState());
    
    // Try to trigger more beats within 50ms window
    for (int i = 0; i < 10; i++) {
        mock_timing_.advanceTime(1000);  // Advance 1ms at a time
        
        // Feed high samples (would normally trigger beat)
        audio_->processSample(1500);
        audio_->processSample(1600);
        audio_->processSample(1400);
    }
    
    // Should still be only 1 beat (no additional beats during debounce)
    EXPECT_EQ(1, beat_count) << "No beats should be detected during debounce";
}

/**
 * Test: StateRemainsDebounceFor50ms
 * 
 * Given: Beat detected at t=0
 * When: Samples processed at t=10ms, t=30ms, t=49ms
 * Then: State remains DEBOUNCE
 */
TEST_F(DebouncePeriodTest, StateRemainsDebounceFor50ms) {
    audio_->onBeat([](const BeatEvent&) {});  // Register callback
    
    // Trigger beat at t=0
    uint64_t beat_time = triggerBeat();
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState());
    
    // Check at t=10ms (40ms remaining)
    mock_timing_.setTimestamp(beat_time + 10000);
    audio_->processSample(100);
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState())
        << "Should remain in DEBOUNCE at +10ms";
    
    // Check at t=30ms (20ms remaining)
    mock_timing_.setTimestamp(beat_time + 30000);
    audio_->processSample(100);
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState())
        << "Should remain in DEBOUNCE at +30ms";
    
    // Check at t=49ms (1ms remaining)
    mock_timing_.setTimestamp(beat_time + 49000);
    audio_->processSample(100);
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState())
        << "Should remain in DEBOUNCE at +49ms";
}

/**
 * Test: TransitionToIdleAfter50ms
 * 
 * Given: Beat detected at t=0, system in DEBOUNCE
 * When: Sample processed at t=50ms (exactly at boundary)
 * Then: State transitions to IDLE
 */
TEST_F(DebouncePeriodTest, TransitionToIdleAfter50ms) {
    audio_->onBeat([](const BeatEvent&) {});
    
    // Trigger beat at t=0
    uint64_t beat_time = triggerBeat();
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState());
    
    // Process sample at exactly t=50ms
    mock_timing_.setTimestamp(beat_time + 50000);
    audio_->processSample(100);
    
    // Should transition to IDLE
    EXPECT_EQ(DetectionState::IDLE, audio_->getState())
        << "Should transition to IDLE at +50ms";
}

/**
 * Test: NewBeatsDetectedAfterDebounce
 * 
 * Given: Beat detected, debounce period expired
 * When: New high amplitude signal arrives
 * Then: New beat is detected and emitted
 */
TEST_F(DebouncePeriodTest, NewBeatsDetectedAfterDebounce) {
    int beat_count = 0;
    std::vector<uint64_t> beat_timestamps;
    
    audio_->onBeat([&](const BeatEvent& event) {
        beat_count++;
        beat_timestamps.push_back(event.timestamp_us);
    });
    
    // First beat at t=0
    uint64_t first_beat = triggerBeat();
    EXPECT_EQ(1, beat_count);
    
    // Wait for debounce to expire
    mock_timing_.setTimestamp(first_beat + 50000);
    audio_->processSample(100);  // Low sample to exit debounce
    EXPECT_EQ(DetectionState::IDLE, audio_->getState());
    
    // Trigger second beat at t=52ms (2ms after debounce)
    mock_timing_.advanceTime(2000);
    triggerBeat();
    
    // Should have 2 beats now
    EXPECT_EQ(2, beat_count);
    EXPECT_EQ(2u, beat_timestamps.size());
    
    // Verify timestamps are >50ms apart
    uint64_t delta = beat_timestamps[1] - beat_timestamps[0];
    EXPECT_GE(delta, 50000u) << "Beats should be >=50ms apart";
}

/**
 * Test: DebouncePreventsFalsePositives
 * 
 * Given: Noisy signal with multiple peaks after beat
 * When: Multiple threshold crossings within 50ms
 * Then: Only one beat event emitted
 */
TEST_F(DebouncePeriodTest, DebouncePreventsFalsePositives) {
    int beat_count = 0;
    audio_->onBeat([&beat_count](const BeatEvent&) {
        beat_count++;
    });
    
    // Trigger beat
    triggerBeat();
    EXPECT_EQ(1, beat_count);
    
    // Simulate noisy signal with rapid threshold crossings
    for (int i = 0; i < 20; i++) {
        mock_timing_.advanceTime(2000);  // Every 2ms
        
        // Alternate high/low (would trigger beats if not debounced)
        if (i % 2 == 0) {
            audio_->processSample(1500);  // High
        } else {
            audio_->processSample(500);   // Low
        }
    }
    
    // Should still be only 1 beat (debounce prevented false positives)
    EXPECT_EQ(1, beat_count) << "Debounce should prevent false positives";
}

/**
 * Test: DebounceBoundaryCondition49ms
 * 
 * Given: Beat at t=0
 * When: Sample at t=49999us (1us before boundary)
 * Then: Still in DEBOUNCE
 */
TEST_F(DebouncePeriodTest, DebounceBoundaryCondition49ms) {
    audio_->onBeat([](const BeatEvent&) {});
    
    uint64_t beat_time = triggerBeat();
    
    // Sample at 49999us (1us before expiry)
    mock_timing_.setTimestamp(beat_time + 49999);
    audio_->processSample(100);
    
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState())
        << "Should still be in DEBOUNCE at +49999us";
}

/**
 * Test: DebounceBoundaryCondition50ms
 * 
 * Given: Beat at t=0
 * When: Sample at t=50000us (exactly at boundary)
 * Then: Transitioned to IDLE
 */
TEST_F(DebouncePeriodTest, DebounceBoundaryCondition50ms) {
    audio_->onBeat([](const BeatEvent&) {});
    
    uint64_t beat_time = triggerBeat();
    
    // Sample at exactly 50000us
    mock_timing_.setTimestamp(beat_time + 50000);
    audio_->processSample(100);
    
    EXPECT_EQ(DetectionState::IDLE, audio_->getState())
        << "Should transition to IDLE at +50000us";
}

/**
 * Test: MultipleBeatsWithProperSpacing
 * 
 * Given: Multiple beats arriving with >50ms spacing
 * When: Each beat separated by at least 50ms
 * Then: All beats detected correctly
 */
TEST_F(DebouncePeriodTest, MultipleBeatsWithProperSpacing) {
    std::vector<uint64_t> beat_timestamps;
    
    audio_->onBeat([&beat_timestamps](const BeatEvent& event) {
        beat_timestamps.push_back(event.timestamp_us);
    });
    
    // Trigger 5 beats, each 60ms apart (10ms margin above debounce)
    for (int i = 0; i < 5; i++) {
        if (i > 0) {
            // Wait for debounce + margin
            mock_timing_.setTimestamp(beat_timestamps.back() + 60000);
            audio_->processSample(100);  // Low sample to return to IDLE
            EXPECT_EQ(DetectionState::IDLE, audio_->getState());
        }
        
        triggerBeat();
    }
    
    // All 5 beats should be detected
    EXPECT_EQ(5u, beat_timestamps.size());
    
    // Verify spacing between consecutive beats
    for (size_t i = 1; i < beat_timestamps.size(); i++) {
        uint64_t delta = beat_timestamps[i] - beat_timestamps[i-1];
        EXPECT_GE(delta, 50000u) 
            << "Beat " << i << " should be >=50ms after beat " << (i-1);
    }
}

/**
 * Test: DebouncePerBeatIndependent
 * 
 * Given: Multiple beats over time
 * When: Each beat has its own 50ms debounce window
 * Then: Debounce resets after each beat
 */
TEST_F(DebouncePeriodTest, DebouncePerBeatIndependent) {
    std::vector<uint64_t> beat_timestamps;
    
    audio_->onBeat([&beat_timestamps](const BeatEvent& event) {
        beat_timestamps.push_back(event.timestamp_us);
    });
    
    // Beat 1 at t=0
    uint64_t beat1 = triggerBeat();
    
    // Wait 51ms, return to IDLE
    mock_timing_.setTimestamp(beat1 + 51000);
    audio_->processSample(100);
    EXPECT_EQ(DetectionState::IDLE, audio_->getState());
    
    // Beat 2 at t=53ms
    mock_timing_.advanceTime(2000);
    uint64_t beat2 = triggerBeat();
    
    // Check that debounce is now relative to beat2, not beat1
    // At beat2 + 49ms, should still be in debounce
    mock_timing_.setTimestamp(beat2 + 49000);
    audio_->processSample(100);
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState())
        << "Debounce should be relative to most recent beat";
    
    // At beat2 + 50ms, should exit debounce
    mock_timing_.setTimestamp(beat2 + 50000);
    audio_->processSample(100);
    EXPECT_EQ(DetectionState::IDLE, audio_->getState());
}

/**
 * Test: DebounceWithVaryingAmplitudes
 * 
 * Given: Beat detected, various amplitude samples during debounce
 * When: High, medium, and low amplitudes fed during 50ms window
 * Then: State remains DEBOUNCE regardless of amplitude
 */
TEST_F(DebouncePeriodTest, DebounceWithVaryingAmplitudes) {
    audio_->onBeat([](const BeatEvent&) {});
    
    uint64_t beat_time = triggerBeat();
    
    // Feed various amplitudes during debounce
    uint16_t test_amplitudes[] = {100, 500, 1000, 2000, 3000, 4000, 4095};
    
    for (uint16_t amp : test_amplitudes) {
        mock_timing_.advanceTime(6000);  // 6ms increments
        audio_->processSample(amp);
        
        // Should remain in DEBOUNCE regardless of amplitude
        if (mock_timing_.getTimestampUs() - beat_time < 50000) {
            EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState())
                << "Should remain in DEBOUNCE for amplitude " << amp;
        }
    }
}

/**
 * Test: BeatCountIncrementsProperly
 * 
 * Given: Multiple beats with proper spacing
 * When: Each beat detected after debounce period
 * Then: Beat count increments correctly
 */
TEST_F(DebouncePeriodTest, BeatCountIncrementsProperly) {
    audio_->onBeat([](const BeatEvent&) {});
    
    // Initial beat count
    EXPECT_EQ(0u, audio_->getBeatCount());
    
    // Trigger 3 beats with proper spacing
    for (int i = 1; i <= 3; i++) {
        if (i > 1) {
            // Wait for debounce to expire
            mock_timing_.advanceTime(55000);  // 55ms > 50ms
            audio_->processSample(100);
            EXPECT_EQ(DetectionState::IDLE, audio_->getState());
        }
        
        triggerBeat();
        EXPECT_EQ(static_cast<uint32_t>(i), audio_->getBeatCount())
            << "Beat count should be " << i << " after beat " << i;
    }
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

