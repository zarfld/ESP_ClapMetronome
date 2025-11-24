/**
 * @file test_stability.cpp
 * @brief Unit tests for BPM stability detection (TDD Cycle 3)
 * 
 * Tests Acceptance Criteria:
 * - AC-BPM-006: Coefficient of Variation (CV) < 5% indicates stable tempo
 * 
 * Implements: DES-C-002 (#46) - BPM Calculation Engine
 * Verifies: REQ-F-002 (#3) - Calculate BPM from tap timestamps
 * 
 * TDD Cycle: BPM-03 (RED phase)
 * Phase: 05-implementation
 * Standard: ISO/IEC/IEEE 12207:2017, XP TDD
 */

#include <gtest/gtest.h>
#include "../../src/bpm/BPMCalculation.h"
#include "../../src/bpm/BPMCalculationState.h"
#include "../../test/mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * @brief Test fixture for BPM stability detection tests
 */
class BPMStabilityTest : public ::testing::Test {
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
     */
    void addTapsWithInterval(int count, uint64_t interval_us) {
        for (int i = 0; i < count; ++i) {
            uint64_t timestamp = i * interval_us;
            mockTiming_->setTimestamp(timestamp);
            bpm_->addTap(timestamp);
        }
    }

    /**
     * @brief Helper: Add taps with varying intervals
     * @param intervals Array of intervals (count-1 intervals for count taps)
     * @param count Number of taps to add
     */
    void addTapsWithVariableIntervals(const uint64_t* intervals, int count) {
        uint64_t cumulative_time = 0;
        mockTiming_->setTimestamp(cumulative_time);
        bpm_->addTap(cumulative_time);

        for (int i = 1; i < count; ++i) {
            cumulative_time += intervals[i - 1];
            mockTiming_->setTimestamp(cumulative_time);
            bpm_->addTap(cumulative_time);
        }
    }
};

/**
 * @brief AC-BPM-006: Stable tempo (CV < 5%) sets is_stable = true
 * 
 * Test: Add 10 taps with consistent 500ms intervals (120 BPM)
 * Expected: isStable() returns true, CV < 5%
 * 
 * Formula: CV = (stddev / mean) × 100%
 * For perfect intervals: stddev = 0, CV = 0%
 */
TEST_F(BPMStabilityTest, ConsistentIntervals_CVLessThan5Percent_IsStable) {
    // Arrange: 10 taps at perfect 500ms intervals (120 BPM)
    constexpr uint64_t INTERVAL_US = 500000;  // 500ms
    constexpr int TAP_COUNT = 10;

    // Act
    addTapsWithInterval(TAP_COUNT, INTERVAL_US);

    // Assert: Stable tempo
    EXPECT_TRUE(bpm_->isStable()) 
        << "Perfect intervals should be stable (CV = 0%)";
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 0.5f)
        << "BPM should be 120";
    EXPECT_LT(bpm_->getCoefficientOfVariation(), 5.0f)
        << "CV should be < 5%";
}

/**
 * @brief AC-BPM-006: Small variations (CV < 5%) still considered stable
 * 
 * Test: Add 10 taps with slight timing variations (±2%)
 * Expected: isStable() returns true, CV < 5%
 * 
 * Intervals: 500ms ± 10ms (490-510ms range)
 * This represents realistic human tapping with minor jitter
 */
TEST_F(BPMStabilityTest, MinorVariations_CVLessThan5Percent_IsStable) {
    // Arrange: Slightly varying intervals around 500ms (±2%)
    uint64_t intervals[] = {
        500000,  // 500ms (baseline)
        490000,  // 490ms (-2%)
        510000,  // 510ms (+2%)
        495000,  // 495ms (-1%)
        505000,  // 505ms (+1%)
        500000,  // 500ms
        498000,  // 498ms (-0.4%)
        502000,  // 502ms (+0.4%)
        500000   // 500ms
    };
    constexpr int TAP_COUNT = 10;

    // Act
    addTapsWithVariableIntervals(intervals, TAP_COUNT);

    // Assert: Still stable (minor variations)
    EXPECT_TRUE(bpm_->isStable())
        << "Minor variations (±2%) should still be stable";
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 2.0f)
        << "BPM should be around 120";
    EXPECT_LT(bpm_->getCoefficientOfVariation(), 5.0f)
        << "CV should be < 5% for minor variations";
}

/**
 * @brief AC-BPM-006: Unstable tempo (CV ≥ 5%) sets is_stable = false
 * 
 * Test: Add 10 taps with significant variations (±10%)
 * Expected: isStable() returns false, CV ≥ 5%
 * 
 * Intervals: 500ms ± 50ms (450-550ms range)
 * This represents erratic tapping or tempo changes
 */
TEST_F(BPMStabilityTest, LargeVariations_CVGreaterThan5Percent_IsUnstable) {
    // Arrange: Large variations around 500ms (±10%)
    uint64_t intervals[] = {
        500000,  // 500ms (baseline)
        450000,  // 450ms (-10%)
        550000,  // 550ms (+10%)
        480000,  // 480ms (-4%)
        520000,  // 520ms (+4%)
        460000,  // 460ms (-8%)
        540000,  // 540ms (+8%)
        490000,  // 490ms (-2%)
        510000   // 510ms (+2%)
    };
    constexpr int TAP_COUNT = 10;

    // Act
    addTapsWithVariableIntervals(intervals, TAP_COUNT);

    // Assert: Unstable tempo
    EXPECT_FALSE(bpm_->isStable())
        << "Large variations (±10%) should be unstable";
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 5.0f)
        << "Average BPM should still be around 120";
    EXPECT_GE(bpm_->getCoefficientOfVariation(), 5.0f)
        << "CV should be ≥ 5% for large variations";
}

/**
 * @brief AC-BPM-006: Boundary case - CV around 5%
 * 
 * Test: Add taps with variations that produce CV ≈ 5%
 * Expected: CV calculated correctly, stability based on threshold
 * 
 * Note: Difficult to produce exactly 5% CV, so test accepts range
 */
TEST_F(BPMStabilityTest, BoundaryCase_CVAround5Percent_CorrectCalculation) {
    // Arrange: Intervals with moderate variation
    uint64_t intervals[] = {
        500000,  // 500ms
        475000,  // 475ms (-5%)
        525000,  // 525ms (+5%)
        485000,  // 485ms (-3%)
        515000,  // 515ms (+3%)
        490000,  // 490ms (-2%)
        510000,  // 510ms (+2%)
        495000,  // 495ms (-1%)
        505000   // 505ms (+1%)
    };
    constexpr int TAP_COUNT = 10;

    // Act
    addTapsWithVariableIntervals(intervals, TAP_COUNT);

    // Assert: CV calculated (actual CV ≈ 3.1% for this data)
    float cv = bpm_->getCoefficientOfVariation();
    EXPECT_GT(cv, 0.0f) << "CV should be calculated";
    EXPECT_LT(cv, 5.0f) << "CV should be < 5% for these intervals";
    EXPECT_TRUE(bpm_->isStable()) << "Should be stable with CV < 5%";
}

/**
 * @brief AC-BPM-006: Insufficient data (< 3 taps) cannot determine stability
 * 
 * Test: Add only 2 taps
 * Expected: isStable() returns false (not enough data)
 * 
 * Rationale: Need at least 3 taps (2 intervals) to calculate stddev
 */
TEST_F(BPMStabilityTest, InsufficientData_LessThan3Taps_IsUnstable) {
    // Arrange: Only 2 taps
    constexpr uint64_t INTERVAL_US = 500000;
    addTapsWithInterval(2, INTERVAL_US);

    // Assert: Not enough data for stability
    EXPECT_FALSE(bpm_->isStable())
        << "Need at least 3 taps to determine stability";
    EXPECT_EQ(bpm_->getCoefficientOfVariation(), 0.0f)
        << "CV should be 0 with insufficient data";
}

/**
 * @brief AC-BPM-006: Gradually improving stability
 * 
 * Test: Start with erratic taps, then add consistent taps
 * Expected: CV improves but buffer retains history
 * 
 * Scenario: Drummer finding the beat - buffer keeps recent history
 * Note: With 64-tap buffer, need many consistent taps to overcome initial jitter
 */
TEST_F(BPMStabilityTest, GraduallyImprovingStability_CVImproves) {
    // Arrange: First 5 taps moderately erratic
    uint64_t erratic_intervals[] = {
        550000,  // 550ms (+10%)
        480000,  // 480ms (-4%)
        520000,  // 520ms (+4%)
        490000   // 490ms (-2%)
    };
    addTapsWithVariableIntervals(erratic_intervals, 5);

    // Capture initial CV
    float initial_cv = bpm_->getCoefficientOfVariation();
    bool initially_unstable = !bpm_->isStable();

    // Act: Add 10 more consistent taps (total 15)
    uint64_t last_timestamp = 550000 + 480000 + 520000 + 490000;
    for (int i = 0; i < 10; ++i) {
        last_timestamp += 500000;  // Perfect 500ms intervals
        mockTiming_->setTimestamp(last_timestamp);
        bpm_->addTap(last_timestamp);
    }

    // Assert: CV improves with more consistent data
    float final_cv = bpm_->getCoefficientOfVariation();
    EXPECT_TRUE(initially_unstable) 
        << "Should start unstable or marginally stable";
    EXPECT_LT(final_cv, initial_cv)
        << "CV should decrease with consistent taps (" 
        << initial_cv << " → " << final_cv << ")";
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 5.0f)
        << "BPM should average to around 120";
}

/**
 * @brief AC-BPM-006: Stability with full buffer (64 taps)
 * 
 * Test: Fill entire circular buffer with consistent intervals
 * Expected: isStable() returns true throughout
 * 
 * Verifies: Stability calculation works with full buffer
 */
TEST_F(BPMStabilityTest, FullBuffer_ConsistentIntervals_RemainsStable) {
    // Arrange: Fill buffer with 64 consistent taps
    constexpr uint64_t INTERVAL_US = 500000;
    constexpr int BUFFER_SIZE = 64;

    // Act
    addTapsWithInterval(BUFFER_SIZE, INTERVAL_US);

    // Assert: Stable with full buffer
    EXPECT_TRUE(bpm_->isStable())
        << "Full buffer with consistent intervals should be stable";
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 0.5f)
        << "BPM should be 120 with full buffer";
    EXPECT_LT(bpm_->getCoefficientOfVariation(), 1.0f)
        << "CV should be very low with perfect intervals";
}

/**
 * @brief Real-world test: 92 BPM with subdivisions (captured from hardware)
 * 
 * Test: Replay actual clapping rhythm at 92 BPM including all imperfections
 * Expected: System detects ~92 BPM despite subdivisions and timing variations
 * 
 * Data source: ESP32 serial output from 2025-11-24 07:18:13-07:18:27
 * User was clapping at 92 BPM but system detected on both downbeats and upbeats
 * 
 * Key characteristics:
 * - Mix of quarter notes (~650ms) and eighth notes (~325ms)
 * - Timing variations: 207ms to 651ms range
 * - System should normalize subdivisions to detect base tempo
 * 
 * Acceptance: BPM = 92 ± 5 BPM
 */
TEST_F(BPMStabilityTest, RealWorld_92BPM_WithSubdivisions) {
    // Arrange: Real intervals captured from hardware (milliseconds converted to microseconds)
    // This is a complete rhythm sequence including all messy subdivisions
    uint64_t real_intervals[] = {
        363000,  // 363ms - near quarter note
        288000,  // 288ms - subdivision
        651000,  // 651ms - QUARTER NOTE (target tempo)
        340000,  // 340ms - subdivision
        207000,  // 207ms - subdivision
        218000,  // 218ms - subdivision
        415000,  // 415ms - subdivision
        547000,  // 547ms - close to quarter note
        518000,  // 518ms - subdivision
        60000,   // 60ms - possible false positive
        75000,   // 75ms - possible double-tap
        93000,   // 93ms - subdivision
        352000,  // 352ms - subdivision
        529000,  // 529ms - close to quarter note
        60000,   // 60ms - subdivision/noise
        104000,  // 104ms - subdivision
        320000,  // 320ms - subdivision
        180000,  // 180ms - subdivision
        65000,   // 65ms - subdivision
        137000,  // 137ms - subdivision
        141000,  // 141ms - subdivision
        325000,  // 325ms - subdivision (half tempo)
        301000,  // 301ms - subdivision
        437000,  // 437ms - close to quarter note
        315000,  // 315ms - subdivision
        327000,  // 327ms - subdivision
        212000,  // 212ms - subdivision
        351000,  // 351ms - subdivision
        411000,  // 411ms - subdivision
        231000,  // 231ms - subdivision
        111000,  // 111ms - subdivision
        // More intervals from the continuous rhythm
        697000,  // 697ms - long pause between phrases
        204000,  // 204ms - subdivision
        324000,  // 324ms - subdivision
        312000,  // 312ms - subdivision
        363000,  // 363ms - subdivision
        289000,  // 289ms - subdivision
        311000   // 311ms - subdivision
    };
    constexpr int TAP_COUNT = sizeof(real_intervals) / sizeof(real_intervals[0]) + 1;

    // Act: Feed the real rhythm into the BPM calculator
    addTapsWithVariableIntervals(real_intervals, TAP_COUNT);

    // Assert: Should detect base tempo around 92 BPM
    float detected_bpm = bpm_->getBPM();
    EXPECT_NEAR(detected_bpm, 92.0f, 5.0f)
        << "Should detect 92 BPM despite subdivisions (detected: " 
        << detected_bpm << " BPM)";
    
    // CV will be high due to subdivisions, but algorithm should normalize
    float cv = bpm_->getCoefficientOfVariation();
    EXPECT_GT(cv, 0.0f) 
        << "CV should be calculated with real rhythm";
    
    // With subdivision detection, the normalized intervals should show stability
    // Note: This test validates the algorithm handles real-world complexity
}
