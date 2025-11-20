/**
 * Test: AC-AUDIO-002 - Rising Edge Detection State Machine
 * 
 * Verifies state transitions: IDLE → RISING → TRIGGERED → DEBOUNCE → IDLE
 * 
 * Implements: #45 (DES-C-001 Audio Detection Engine)
 * Acceptance Criteria: AC-AUDIO-002
 * Test Method: Unit test with mock timing and controlled ADC samples
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
 * Test Fixture for State Machine Tests
 */
class StateMachineTest : public ::testing::Test {
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
    
    /**
     * Helper: No baseline needed - tests use direct state/threshold manipulation
     * The issue with feeding samples is that zeros in uninitialized window
     * cause threshold calculation problems
     */
    void establishBaseline() {
        // Not used - tests will work with initial threshold of 50
    }
};

/**
 * TEST 1: Initial state is IDLE
 * 
 * Given: Audio detection initialized
 * When: No samples processed
 * Then: State = IDLE
 */
TEST_F(StateMachineTest, InitialStateIsIdle) {
    EXPECT_EQ(DetectionState::IDLE, audio_->getState()) 
        << "Initial state should be IDLE";
}

/**
 * TEST 2: Transition IDLE → RISING on threshold crossing
 * 
 * Given: State = IDLE, window initialized with zeros
 * When: ADC value exceeds adaptive threshold + margin + noise floor
 * Then: State = RISING
 * 
 * Note: With zero-init window, establish baseline first:
 * - Feed 50 samples of ~500 to establish min=500, max=500, threshold=500
 * - Then sample 1000 > (500 + 80 margin) and > (100 noise + 200 min) triggers RISING
 */
TEST_F(StateMachineTest, IdleToRisingOnThresholdCrossing) {
    // Arrange: Establish baseline with low samples
    for (int i = 0; i < 50; i++) {
        audio_->processSample(500);  // Low baseline
        mock_timing_.advanceTime(125);   // 8kHz sampling
    }
    EXPECT_EQ(DetectionState::IDLE, audio_->getState());
    
    // Act: Feed high sample that exceeds threshold + margin + noise floor
    audio_->processSample(1000);  // High value > (500+80) and > (100+200)
    
    // Assert: State changed to RISING_EDGE
    EXPECT_EQ(DetectionState::RISING_EDGE, audio_->getState())
        << "State should transition to RISING_EDGE when threshold crossed";
}

/**
 * TEST 3: Stay in IDLE if below adaptive threshold
 * 
 * Given: State = IDLE
 * When: ADC values equal or below the adaptive threshold
 * Then: State remains IDLE
 * 
 * Note: Threshold calculation: threshold = 0.8 * (max - min) + min
 * With consistent value V: min=0 (zeros), max=V, threshold = 0.8*V
 * So V <= 0.8*V is only true for V=0
 * We'll use constant value that equals threshold to test "at threshold" behavior
 */
TEST_F(StateMachineTest, StayInIdleBelowThreshold) {
    // Arrange: Initial state, threshold=50
    EXPECT_EQ(DetectionState::IDLE, audio_->getState());
    uint16_t initial_threshold = audio_->getThreshold();
    EXPECT_EQ(50, initial_threshold);
    
    // Act: Feed samples AT initial threshold (should not cross)
    for (int i = 0; i < 10; i++) {
        audio_->processSample(initial_threshold);  // Equal to threshold, not greater
        mock_timing_.advanceTime(1000);
    }
    
    // Assert: Still in IDLE (equal to threshold is not a crossing)
    EXPECT_EQ(DetectionState::IDLE, audio_->getState())
        << "State should remain IDLE when at or below threshold";
}

/**
 * TEST 4: Transition RISING → TRIGGERED after measuring rise
 * 
 * Given: State = RISING
 * When: Peak detected (sample starts falling)
 * Then: State = TRIGGERED, beat event emitted
 */
TEST_F(StateMachineTest, RisingToTriggeredOnPeak) {
    // Arrange: Establish baseline and get to RISING state
    for (int i = 0; i < 50; i++) {
        audio_->processSample(500);  // Low baseline
        mock_timing_.advanceTime(125);
    }
    
    bool beat_fired = false;
    audio_->onBeat([&beat_fired](const BeatEvent& event) {
        beat_fired = true;
        (void)event;
    });
    
    audio_->processSample(1000);  // IDLE → RISING_EDGE
    EXPECT_EQ(DetectionState::RISING_EDGE, audio_->getState());
    
    // Act: Continue rising then peak
    mock_timing_.advanceTime(1000);
    audio_->processSample(1200);  // Still rising
    mock_timing_.advanceTime(1000);
    audio_->processSample(1500);  // Peak
    mock_timing_.advanceTime(1000);
    audio_->processSample(1400);  // Starting to fall → trigger
    
    // Assert: Transitioned to TRIGGERED and beat emitted
    EXPECT_EQ(DetectionState::TRIGGERED, audio_->getState())
        << "State should transition to TRIGGERED at peak";
    EXPECT_TRUE(beat_fired)
        << "Beat event should be emitted in TRIGGERED state";
}

/**
 * TEST 5: Transition TRIGGERED → DEBOUNCE immediately
 * 
 * Given: State = TRIGGERED (beat just emitted)
 * When: Next sample processed
 * Then: State = DEBOUNCE
 */
TEST_F(StateMachineTest, TriggeredToDebounceImmediately) {
    // Arrange: Get to TRIGGERED state
    audio_->onBeat([](const BeatEvent&) {});  // Register callback
    audio_->processSample(3000);  // IDLE → RISING
    mock_timing_.advanceTime(1000);
    audio_->processSample(3500);  // Peak
    mock_timing_.advanceTime(1000);
    audio_->processSample(3400);  // RISING → TRIGGERED
    EXPECT_EQ(DetectionState::TRIGGERED, audio_->getState());
    
    // Act: Process next sample
    mock_timing_.advanceTime(1000);
    audio_->processSample(200);
    
    // Assert: Now in DEBOUNCE
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState())
        << "State should transition to DEBOUNCE after TRIGGERED";
}

/**
 * TEST 6: Stay in DEBOUNCE for 50ms (AC-AUDIO-005)
 * 
 * Given: State = DEBOUNCE, last beat at t=10ms
 * When: Process samples at t=20ms, t=30ms, t=40ms
 * Then: State remains DEBOUNCE (still within 50ms)
 */
TEST_F(StateMachineTest, StayInDebounceFor50ms) {
    // Arrange: Get to DEBOUNCE state at t=10ms
    mock_timing_.setTimestamp(10000);  // 10ms
    audio_->onBeat([](const BeatEvent&) {});
    audio_->processSample(3000);  // IDLE → RISING
    mock_timing_.advanceTime(1000);
    audio_->processSample(3500);  // Peak
    mock_timing_.advanceTime(1000);
    audio_->processSample(3400);  // TRIGGERED
    mock_timing_.advanceTime(1000);
    audio_->processSample(3000);  // → DEBOUNCE
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState());
    
    // Act: Process samples within 50ms window
    mock_timing_.advanceTime(10000);  // t=23ms
    audio_->processSample(200);
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState()) << "Still in debounce at t=23ms";
    
    mock_timing_.advanceTime(10000);  // t=33ms
    audio_->processSample(200);
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState()) << "Still in debounce at t=33ms";
    
    mock_timing_.advanceTime(10000);  // t=43ms
    audio_->processSample(200);
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState()) << "Still in debounce at t=43ms";
}

/**
 * TEST 7: Transition DEBOUNCE → IDLE after 50ms
 * 
 * Given: State = DEBOUNCE, last beat at t=10ms
 * When: Process sample at t=65ms (>50ms elapsed)
 * Then: State = IDLE (ready for next beat)
 */
TEST_F(StateMachineTest, DebounceToIdleAfter50ms) {
    // Arrange: Get to DEBOUNCE state at t=10ms
    mock_timing_.setTimestamp(10000);
    audio_->onBeat([](const BeatEvent&) {});
    audio_->processSample(3000);
    mock_timing_.advanceTime(1000);
    audio_->processSample(3500);
    mock_timing_.advanceTime(1000);
    audio_->processSample(3400);
    mock_timing_.advanceTime(1000);
    audio_->processSample(3000);
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState());
    
    // Act: Advance past debounce period (50ms)
    mock_timing_.advanceTime(55000);  // t=68ms (>50ms since beat at 10ms)
    audio_->processSample(150);  // Below threshold
    
    // Assert: Back to IDLE
    EXPECT_EQ(DetectionState::IDLE, audio_->getState())
        << "State should return to IDLE after 50ms debounce period";
}

/**
 * TEST 8: No beat event during DEBOUNCE period
 * 
 * Given: State = DEBOUNCE
 * When: Threshold crossed during debounce
 * Then: No beat event emitted (still in DEBOUNCE)
 */
TEST_F(StateMachineTest, NoBeatsDetectedDuringDebounce) {
    // Arrange: Get to DEBOUNCE
    int beat_count = 0;
    audio_->onBeat([&beat_count](const BeatEvent&) { beat_count++; });
    
    audio_->processSample(3000);
    mock_timing_.advanceTime(1000);
    audio_->processSample(3500);
    mock_timing_.advanceTime(1000);
    audio_->processSample(3400);
    EXPECT_EQ(1, beat_count) << "First beat detected";
    
    mock_timing_.advanceTime(1000);
    audio_->processSample(300);
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState());
    
    // Act: Try to trigger another beat during debounce
    mock_timing_.advanceTime(10000);  // Still within 50ms
    audio_->processSample(150);  // Fall below
    mock_timing_.advanceTime(1000);
    audio_->processSample(400);  // Try to trigger again
    mock_timing_.advanceTime(1000);
    audio_->processSample(390);
    
    // Assert: Still only 1 beat
    EXPECT_EQ(1, beat_count) << "No additional beats during debounce period";
    EXPECT_EQ(DetectionState::DEBOUNCE, audio_->getState());
}

/**
 * Test main entry point
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
