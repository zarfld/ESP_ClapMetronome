/**
 * @file test_tempo_correction.cpp
 * @brief Unit tests for BPM half/double tempo correction (TDD Cycle 4)
 * 
 * Tests Acceptance Criteria:
 * - AC-BPM-004: 5 consecutive intervals ~2× average → BPM halved
 * - AC-BPM-005: 5 consecutive intervals ~0.5× average → BPM doubled
 * 
 * Implements: DES-C-002 (#46) - BPM Calculation Engine
 * Verifies: REQ-F-002 (#3) - Calculate BPM from tap timestamps
 * 
 * TDD Cycle: BPM-04 (RED phase)
 * Phase: 05-implementation
 * Standard: ISO/IEC/IEEE 12207:2017, XP TDD
 */

#include <gtest/gtest.h>
#include "../../src/bpm/BPMCalculation.h"
#include "../../src/bpm/BPMCalculationState.h"
#include "../../test/mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * @brief Test fixture for tempo correction tests
 */
class BPMTempoCorrectionTest : public ::testing::Test {
protected:
    MockTimingProvider* mockTiming_;
    BPMCalculation* bpm_;

    void SetUp() override {
        mockTiming_ = new MockTimingProvider();
        bpm_ = new BPMCalculation(mockTiming_);
        bpm_->init();
    }

    void TearDown() override {
        delete bpm_;
        delete mockTiming_;
    }

    /**
     * @brief Helper: Add taps with consistent intervals
     * @param count Number of taps to add
     * @param interval_us Interval between taps in microseconds
     * @param start_time Starting timestamp
     * @return Last timestamp added
     */
    uint64_t addTapsWithInterval(int count, uint64_t interval_us, uint64_t start_time = 0) {
        uint64_t timestamp = start_time;
        for (int i = 0; i < count; ++i) {
            mockTiming_->setTimestamp(timestamp);
            bpm_->addTap(timestamp);
            timestamp += interval_us;
        }
        return timestamp - interval_us;  // Return last timestamp
    }
};

/**
 * @brief AC-BPM-004: Half-tempo detection
 * 
 * Test: Establish 120 BPM (500ms intervals), then add 5 intervals at 1000ms
 * Expected: After 5 consecutive 2× intervals, BPM corrects to 60 (halved)
 * 
 * Scenario: Drummer tapping on every other beat (half-time feel)
 * Design: HALF_TEMPO_RATIO = 1.8× average, THRESHOLD = 5 consecutive
 */
TEST_F(BPMTempoCorrectionTest, HalfTempo_5Consecutive2xIntervals_BPMHalved) {
    // Arrange: Establish 120 BPM baseline with 10 taps at 500ms
    constexpr uint64_t BASELINE_INTERVAL = 500000;  // 500ms = 120 BPM
    uint64_t last_time = addTapsWithInterval(10, BASELINE_INTERVAL);
    
    float initial_bpm = bpm_->getBPM();
    EXPECT_NEAR(initial_bpm, 120.0f, 0.5f) << "Baseline should be 120 BPM";
    
    // Act: Add 5 taps at 1000ms intervals (2× baseline)
    constexpr uint64_t HALF_TEMPO_INTERVAL = 1000000;  // 1000ms = 2× baseline
    addTapsWithInterval(5, HALF_TEMPO_INTERVAL, last_time + HALF_TEMPO_INTERVAL);
    
    // Assert: BPM should be halved to 60
    float corrected_bpm = bpm_->getBPM();
    EXPECT_NEAR(corrected_bpm, 60.0f, 1.0f)
        << "After 5 consecutive 2× intervals, BPM should halve to ~60";
}

/**
 * @brief AC-BPM-004: Half-tempo not triggered with only 4 intervals
 * 
 * Test: 4 consecutive 2× intervals should NOT trigger correction
 * Expected: BPM remains closer to average of all intervals
 * 
 * Threshold: Need exactly 5 consecutive to trigger correction
 */
TEST_F(BPMTempoCorrectionTest, HalfTempo_Only4Consecutive_NoCorrection) {
    // Arrange: Establish 120 BPM
    constexpr uint64_t BASELINE_INTERVAL = 500000;
    uint64_t last_time = addTapsWithInterval(10, BASELINE_INTERVAL);
    
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 0.5f) << "Baseline should be 120 BPM";
    
    // Act: Add only 4 taps at 1000ms (one short of threshold)
    constexpr uint64_t HALF_TEMPO_INTERVAL = 1000000;
    addTapsWithInterval(4, HALF_TEMPO_INTERVAL, last_time + HALF_TEMPO_INTERVAL);
    
    // Assert: BPM should NOT be corrected (averages to ~80-90 BPM)
    float bpm_after = bpm_->getBPM();
    EXPECT_GT(bpm_after, 70.0f)
        << "With only 4 intervals, should not trigger correction";
    EXPECT_LT(bpm_after, 110.0f)
        << "BPM averages the mixed intervals";
}

/**
 * @brief AC-BPM-005: Double-tempo detection
 * 
 * Test: Establish 120 BPM (500ms), then add 5 intervals at 250ms
 * Expected: After 5 consecutive 0.5× intervals, BPM corrects to 240 (doubled)
 * 
 * Scenario: Drummer switching to double-time (hitting twice as fast)
 * Design: DOUBLE_TEMPO_RATIO = 0.6× average, THRESHOLD = 5 consecutive
 */
TEST_F(BPMTempoCorrectionTest, DoubleTempo_5Consecutive05xIntervals_BPMDoubled) {
    // Arrange: Establish 120 BPM baseline
    constexpr uint64_t BASELINE_INTERVAL = 500000;  // 500ms = 120 BPM
    uint64_t last_time = addTapsWithInterval(10, BASELINE_INTERVAL);
    
    float initial_bpm = bpm_->getBPM();
    EXPECT_NEAR(initial_bpm, 120.0f, 0.5f) << "Baseline should be 120 BPM";
    
    // Act: Add 5 taps at 250ms intervals (0.5× baseline)
    constexpr uint64_t DOUBLE_TEMPO_INTERVAL = 250000;  // 250ms = 0.5× baseline
    addTapsWithInterval(5, DOUBLE_TEMPO_INTERVAL, last_time + DOUBLE_TEMPO_INTERVAL);
    
    // Assert: BPM should be doubled to 240
    float corrected_bpm = bpm_->getBPM();
    EXPECT_NEAR(corrected_bpm, 240.0f, 2.0f)
        << "After 5 consecutive 0.5× intervals, BPM should double to ~240";
}

/**
 * @brief AC-BPM-005: Double-tempo not triggered with only 4 intervals
 * 
 * Test: 4 consecutive 0.5× intervals should NOT trigger correction
 * Expected: BPM remains closer to average
 */
TEST_F(BPMTempoCorrectionTest, DoubleTempo_Only4Consecutive_NoCorrection) {
    // Arrange: Establish 120 BPM
    constexpr uint64_t BASELINE_INTERVAL = 500000;
    uint64_t last_time = addTapsWithInterval(10, BASELINE_INTERVAL);
    
    // Act: Add only 4 taps at 250ms
    constexpr uint64_t DOUBLE_TEMPO_INTERVAL = 250000;
    addTapsWithInterval(4, DOUBLE_TEMPO_INTERVAL, last_time + DOUBLE_TEMPO_INTERVAL);
    
    // Assert: BPM should NOT be doubled (averages to ~140-160 BPM)
    float bpm_after = bpm_->getBPM();
    EXPECT_GT(bpm_after, 130.0f)
        << "With only 4 intervals, should not trigger correction";
    EXPECT_LT(bpm_after, 200.0f)
        << "BPM averages the mixed intervals";
}

/**
 * @brief Edge case: Alternating tempos don't trigger correction
 * 
 * Test: Alternating fast/slow intervals (not 5 consecutive)
 * Expected: No tempo correction applied
 * 
 * Scenario: Prevents false detection from rhythmic patterns
 */
TEST_F(BPMTempoCorrectionTest, AlternatingTempos_NoConsecutive5_NoCorrection) {
    // Arrange: Establish 120 BPM
    constexpr uint64_t BASELINE_INTERVAL = 500000;
    uint64_t last_time = addTapsWithInterval(10, BASELINE_INTERVAL);
    
    // Act: Alternate between 1000ms and 250ms (not 5 consecutive of either)
    uint64_t time = last_time;
    for (int i = 0; i < 8; ++i) {
        uint64_t interval = (i % 2 == 0) ? 1000000 : 250000;
        time += interval;
        mockTiming_->setTimestamp(time);
        bpm_->addTap(time);
    }
    
    // Assert: No extreme BPM (neither halved nor doubled)
    float bpm_after = bpm_->getBPM();
    EXPECT_GT(bpm_after, 70.0f) << "Should not halve";
    EXPECT_LT(bpm_after, 200.0f) << "Should not double";
}

/**
 * @brief Edge case: Half-tempo resets if interrupted
 * 
 * Test: 4 half-tempo intervals, then 1 normal, then 5 more half-tempo
 * Expected: Counter resets, second sequence triggers correction
 * 
 * Design: Consecutive counter resets on non-matching interval
 */
TEST_F(BPMTempoCorrectionTest, HalfTempoCounter_ResetsOnInterruption_RequiresNew5) {
    // Arrange: Establish 120 BPM
    constexpr uint64_t BASELINE_INTERVAL = 500000;
    uint64_t last_time = addTapsWithInterval(10, BASELINE_INTERVAL);
    
    // Act Part 1: Add 4 half-tempo intervals (one short)
    constexpr uint64_t HALF_TEMPO_INTERVAL = 1000000;
    last_time = addTapsWithInterval(4, HALF_TEMPO_INTERVAL, last_time + HALF_TEMPO_INTERVAL);
    
    // Act Part 2: Add 1 normal interval (resets counter)
    last_time += BASELINE_INTERVAL;
    mockTiming_->setTimestamp(last_time);
    bpm_->addTap(last_time);
    
    // Act Part 3: Add 5 more half-tempo intervals
    addTapsWithInterval(5, HALF_TEMPO_INTERVAL, last_time + HALF_TEMPO_INTERVAL);
    
    // Assert: Should trigger correction from second sequence
    float corrected_bpm = bpm_->getBPM();
    EXPECT_LT(corrected_bpm, 80.0f)
        << "Second sequence of 5 should trigger correction";
}

/**
 * @brief Edge case: Double-tempo resets if interrupted
 * 
 * Similar to half-tempo reset test
 */
TEST_F(BPMTempoCorrectionTest, DoubleTempoCounter_ResetsOnInterruption_RequiresNew5) {
    // Arrange: Establish 120 BPM
    constexpr uint64_t BASELINE_INTERVAL = 500000;
    uint64_t last_time = addTapsWithInterval(10, BASELINE_INTERVAL);
    
    // Act Part 1: Add 4 double-tempo intervals
    constexpr uint64_t DOUBLE_TEMPO_INTERVAL = 250000;
    last_time = addTapsWithInterval(4, DOUBLE_TEMPO_INTERVAL, last_time + DOUBLE_TEMPO_INTERVAL);
    
    // Act Part 2: Add 1 normal interval (resets counter)
    last_time += BASELINE_INTERVAL;
    mockTiming_->setTimestamp(last_time);
    bpm_->addTap(last_time);
    
    // Act Part 3: Add 5 more double-tempo intervals
    addTapsWithInterval(5, DOUBLE_TEMPO_INTERVAL, last_time + DOUBLE_TEMPO_INTERVAL);
    
    // Assert: Should trigger correction from second sequence
    float corrected_bpm = bpm_->getBPM();
    EXPECT_GT(corrected_bpm, 180.0f)
        << "Second sequence of 5 should trigger correction";
}

/**
 * @brief Boundary: Interval exactly at 1.8× threshold
 * 
 * Test: Intervals at exactly HALF_TEMPO_RATIO (1.8×)
 * Expected: Should trigger half-tempo detection
 */
TEST_F(BPMTempoCorrectionTest, HalfTempoBoundary_Exactly18x_TriggersCorrection) {
    // Arrange: Establish 100 BPM (600ms intervals for easy math)
    constexpr uint64_t BASELINE_INTERVAL = 600000;  // 600ms = 100 BPM
    uint64_t last_time = addTapsWithInterval(10, BASELINE_INTERVAL);
    
    // Act: Add 5 intervals at exactly 1.8× = 1080ms
    constexpr uint64_t BOUNDARY_INTERVAL = static_cast<uint64_t>(600000 * 1.8f);
    addTapsWithInterval(5, BOUNDARY_INTERVAL, last_time + BOUNDARY_INTERVAL);
    
    // Assert: Should trigger half-tempo correction
    float corrected_bpm = bpm_->getBPM();
    EXPECT_LT(corrected_bpm, 70.0f)
        << "Exactly 1.8× should trigger half-tempo";
}

/**
 * @brief Boundary: Interval exactly at 0.6× threshold
 * 
 * Test: Intervals at exactly DOUBLE_TEMPO_RATIO (0.6×)
 * Expected: Should trigger double-tempo detection
 */
TEST_F(BPMTempoCorrectionTest, DoubleTempoBoundary_Exactly06x_TriggersCorrection) {
    // Arrange: Establish 100 BPM (600ms intervals)
    constexpr uint64_t BASELINE_INTERVAL = 600000;
    uint64_t last_time = addTapsWithInterval(10, BASELINE_INTERVAL);
    
    // Act: Add 5 intervals at exactly 0.6× = 360ms
    constexpr uint64_t BOUNDARY_INTERVAL = static_cast<uint64_t>(600000 * 0.6f);
    addTapsWithInterval(5, BOUNDARY_INTERVAL, last_time + BOUNDARY_INTERVAL);
    
    // Assert: Should trigger double-tempo correction
    float corrected_bpm = bpm_->getBPM();
    EXPECT_GT(corrected_bpm, 140.0f)
        << "Exactly 0.6× should trigger double-tempo";
}
