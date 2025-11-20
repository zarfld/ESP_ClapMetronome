/**
 * @file test_interval_validation.cpp
 * @brief Unit tests for BPM interval validation (AC-BPM-013)
 * 
 * Tests invalid interval filtering:
 * - Reject intervals < 100ms (too fast, > 600 BPM)
 * - Reject intervals > 2000ms (too slow, < 30 BPM)
 * - Accept valid intervals (100-2000ms, 30-600 BPM)
 * 
 * @component DES-C-002: BPM Calculation Engine
 * @requirement AC-BPM-013: Tempo range validation (40-300 BPM nominal)
 * @phase Wave 2.2, Cycle 5 of 7
 * @standards ISO/IEC/IEEE 12207:2017 (Implementation), XP TDD
 * @date 2025-11-20
 */

#include <gtest/gtest.h>
#include "../../src/bpm/BPMCalculation.h"
#include "../../src/bpm/BPMCalculationState.h"
#include "../../test/mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * @brief Test fixture for interval validation tests
 */
class BPMIntervalValidationTest : public ::testing::Test {
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
};

/**
 * @brief Test: Interval below 100ms is rejected
 * 
 * AC-BPM-013: Intervals < 100ms (> 600 BPM) should be rejected
 * 
 * Scenario: User taps faster than physically reasonable
 * Expected: Tap rejected, tap_count not incremented
 */
TEST_F(BPMIntervalValidationTest, IntervalBelowMin_99ms_Rejected) {
    // Arrange: Establish baseline
    constexpr uint64_t BASELINE = 500000; // 500ms
    mockTiming_->setTimestamp(0);
    bpm_->addTap(0);
    
    mockTiming_->setTimestamp(BASELINE);
    bpm_->addTap(BASELINE);
    
    EXPECT_EQ(bpm_->getTapCount(), 2) << "Should have 2 taps initially";
    
    // Act: Add tap at 99ms interval (below MIN_INTERVAL_US = 100ms)
    constexpr uint64_t INVALID_INTERVAL = 99000; // 99ms
    uint64_t invalid_time = BASELINE + INVALID_INTERVAL;
    mockTiming_->setTimestamp(invalid_time);
    bpm_->addTap(invalid_time);
    
    // Assert: Tap should be rejected
    EXPECT_EQ(bpm_->getTapCount(), 2)
        << "Tap count should not increase for interval < 100ms";
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 0.5f)
        << "BPM should remain at baseline (500ms = 120 BPM)";
}

/**
 * @brief Test: Interval at exactly 100ms is accepted
 * 
 * Boundary test: MIN_INTERVAL_US = 100ms exactly
 */
TEST_F(BPMIntervalValidationTest, IntervalAtMin_100ms_Accepted) {
    // Arrange: First tap
    mockTiming_->setTimestamp(0);
    bpm_->addTap(0);
    
    // Act: Add tap at exactly 100ms interval
    constexpr uint64_t MIN_INTERVAL = 100000; // 100ms (600 BPM)
    mockTiming_->setTimestamp(MIN_INTERVAL);
    bpm_->addTap(MIN_INTERVAL);
    
    // Assert: Should be accepted
    EXPECT_EQ(bpm_->getTapCount(), 2)
        << "Tap at exactly 100ms should be accepted";
    EXPECT_NEAR(bpm_->getBPM(), 600.0f, 1.0f)
        << "BPM should be 600 (100ms interval)";
}

/**
 * @brief Test: Interval above 2000ms is rejected
 * 
 * AC-BPM-013: Intervals > 2000ms (< 30 BPM) should be rejected
 * 
 * Scenario: User pauses too long between taps
 * Expected: Tap rejected, buffer maintains previous state
 */
TEST_F(BPMIntervalValidationTest, IntervalAboveMax_2001ms_Rejected) {
    // Arrange: Establish baseline
    constexpr uint64_t BASELINE = 500000; // 500ms
    mockTiming_->setTimestamp(0);
    bpm_->addTap(0);
    
    mockTiming_->setTimestamp(BASELINE);
    bpm_->addTap(BASELINE);
    
    EXPECT_EQ(bpm_->getTapCount(), 2);
    
    // Act: Add tap at 2001ms interval (above MAX_INTERVAL_US = 2000ms)
    constexpr uint64_t INVALID_INTERVAL = 2001000; // 2001ms
    uint64_t invalid_time = BASELINE + INVALID_INTERVAL;
    mockTiming_->setTimestamp(invalid_time);
    bpm_->addTap(invalid_time);
    
    // Assert: Tap should be rejected
    EXPECT_EQ(bpm_->getTapCount(), 2)
        << "Tap count should not increase for interval > 2000ms";
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 0.5f)
        << "BPM should remain at baseline";
}

/**
 * @brief Test: Interval at exactly 2000ms is accepted
 * 
 * Boundary test: MAX_INTERVAL_US = 2000ms exactly
 */
TEST_F(BPMIntervalValidationTest, IntervalAtMax_2000ms_Accepted) {
    // Arrange: First tap
    mockTiming_->setTimestamp(0);
    bpm_->addTap(0);
    
    // Act: Add tap at exactly 2000ms interval
    constexpr uint64_t MAX_INTERVAL = 2000000; // 2000ms (30 BPM)
    mockTiming_->setTimestamp(MAX_INTERVAL);
    bpm_->addTap(MAX_INTERVAL);
    
    // Assert: Should be accepted
    EXPECT_EQ(bpm_->getTapCount(), 2)
        << "Tap at exactly 2000ms should be accepted";
    EXPECT_NEAR(bpm_->getBPM(), 30.0f, 0.5f)
        << "BPM should be 30 (2000ms interval)";
}

/**
 * @brief Test: Valid interval range accepted
 * 
 * Test nominal range: 100-2000ms (30-600 BPM)
 */
TEST_F(BPMIntervalValidationTest, ValidIntervalRange_AllAccepted) {
    // Test several valid intervals
    const uint64_t valid_intervals[] = {
        100000,   // 100ms  = 600 BPM (min boundary)
        200000,   // 200ms  = 300 BPM
        500000,   // 500ms  = 120 BPM (typical)
        1000000,  // 1000ms = 60 BPM
        2000000   // 2000ms = 30 BPM (max boundary)
    };
    
    uint64_t timestamp = 0;
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    
    for (size_t i = 0; i < sizeof(valid_intervals) / sizeof(valid_intervals[0]); ++i) {
        timestamp += valid_intervals[i];
        mockTiming_->setTimestamp(timestamp);
        bpm_->addTap(timestamp);
        
        EXPECT_EQ(bpm_->getTapCount(), static_cast<uint8_t>(i + 2))
            << "Valid interval " << valid_intervals[i] << "µs should be accepted";
    }
    
    EXPECT_EQ(bpm_->getTapCount(), 6)
        << "All 5 valid intervals should be accepted";
}

/**
 * @brief Test: Mixed valid and invalid intervals
 * 
 * Scenario: Some taps valid, some rejected
 * Expected: Only valid taps counted
 */
TEST_F(BPMIntervalValidationTest, MixedIntervals_OnlyValidAccepted) {
    // Arrange: First tap
    uint64_t timestamp = 0;
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    
    // Act: Add mix of valid and invalid
    // Valid: 500ms
    timestamp += 500000;
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    EXPECT_EQ(bpm_->getTapCount(), 2) << "Valid 500ms accepted";
    uint64_t last_valid = timestamp; // Track last valid
    
    // Invalid: 50ms (too fast)
    timestamp += 50000;
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    EXPECT_EQ(bpm_->getTapCount(), 2) << "Invalid 50ms rejected";
    
    // Valid: 500ms from last valid tap
    timestamp = last_valid + 500000; // 500ms from last valid
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    EXPECT_EQ(bpm_->getTapCount(), 3) << "Valid 500ms accepted";
    last_valid = timestamp; // Update last valid
    
    // Invalid: 3000ms (too slow)
    timestamp = last_valid + 3000000;
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    EXPECT_EQ(bpm_->getTapCount(), 3) << "Invalid 3000ms rejected";
    
    // Valid: 500ms from last valid tap (not from invalid)
    timestamp = last_valid + 500000;
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    EXPECT_EQ(bpm_->getTapCount(), 4) << "Valid 500ms accepted";
    
    // Assert: BPM calculated from valid taps only
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 1.0f)
        << "BPM should be based on 500ms intervals (120 BPM)";
}

/**
 * @brief Test: First tap always accepted
 * 
 * No previous tap to compare against, so first tap has no interval
 */
TEST_F(BPMIntervalValidationTest, FirstTap_AlwaysAccepted) {
    // Act: Add first tap (no interval to validate)
    mockTiming_->setTimestamp(0);
    bpm_->addTap(0);
    
    // Assert
    EXPECT_EQ(bpm_->getTapCount(), 1)
        << "First tap should always be accepted";
    EXPECT_EQ(bpm_->getBPM(), 0.0f)
        << "No BPM with single tap";
}

/**
 * @brief Test: Invalid tap doesn't affect last_tap_us
 * 
 * Implementation detail: Invalid taps should not update last_tap_us
 * Next valid tap should measure interval from last VALID tap
 */
TEST_F(BPMIntervalValidationTest, InvalidTap_DoesNotUpdateLastTap) {
    // Arrange: Establish baseline
    mockTiming_->setTimestamp(0);
    bpm_->addTap(0);
    
    mockTiming_->setTimestamp(500000);
    bpm_->addTap(500000); // Last valid at 500ms
    
    // Act: Add invalid tap (should not update last_tap_us)
    mockTiming_->setTimestamp(550000); // 50ms later (invalid)
    bpm_->addTap(550000);
    
    // Add valid tap 500ms after LAST VALID (not after invalid)
    mockTiming_->setTimestamp(1000000); // 500ms from last valid
    bpm_->addTap(1000000);
    
    // Assert: Should have 3 valid taps (invalid was not counted)
    EXPECT_EQ(bpm_->getTapCount(), 3)
        << "Should have 3 valid taps";
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 0.5f)
        << "BPM should be 120 (500ms intervals)";
}
