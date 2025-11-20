/**
 * BPM Calculation - Circular Buffer & Wrap-around Tests
 * 
 * @file test_circular_buffer.cpp
 * @component DES-C-002: BPM Calculation Engine
 * @verifies AC-BPM-003: 64 taps at 140 BPM returns 140.0 ± 0.5
 * @verifies AC-BPM-007: 65th tap overwrites oldest
 * 
 * @test_type Unit Test
 * @phase Phase 05 - Implementation (TDD Cycle 2)
 * @standards ISO/IEC/IEEE 12207:2017, XP Test-First
 * 
 * @description
 * Tests circular buffer behavior when buffer fills and wraps around.
 * Verifies that BPM remains accurate with full buffer and that oldest
 * taps are correctly overwritten.
 * 
 * Test Scenarios:
 * 1. AC-BPM-003: 64 taps fill buffer, BPM accurate
 * 2. AC-BPM-007: 65th tap overwrites oldest, BPM remains accurate
 * 3. Edge case: Verify write index wraps to 0
 * 4. Edge case: 128 taps (2 full rotations)
 * 
 * @see https://github.com/zarfld/ESP_ClapMetronome/issues/46
 * @date 2025-11-20
 */

#include <gtest/gtest.h>
#include "../../src/bpm/BPMCalculation.h"
#include "../../test/mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * Test Fixture for Circular Buffer Tests
 */
class BPMCalculationCircularBufferTest : public ::testing::Test {
protected:
    MockTimingProvider* mockTiming_;
    BPMCalculation* bpm_;
    
    void SetUp() override {
        mockTiming_ = new MockTimingProvider();
        mockTiming_->setTimestamp(0);
        
        bpm_ = new BPMCalculation(mockTiming_);
        bpm_->init();
    }
    
    void TearDown() override {
        delete bpm_;
        delete mockTiming_;
    }
    
    /**
     * Helper: Add N taps with fixed interval
     * @param count Number of taps
     * @param interval_us Interval between taps in microseconds
     */
    void addTapsWithInterval(int count, uint64_t interval_us) {
        for (int i = 0; i < count; ++i) {
            uint64_t timestamp = i * interval_us;
            mockTiming_->setTimestamp(timestamp);
            bpm_->addTap(timestamp);
        }
    }
};

// ============================================================================
// AC-BPM-003: 64 taps at 140 BPM returns 140.0 ± 0.5
// ============================================================================

/**
 * @test AC-BPM-003: Full buffer (64 taps) should maintain BPM accuracy
 * 
 * @scenario Given BPM calculator initialized
 *           When user adds 64 taps at 140 BPM (428.571ms intervals)
 *           Then getBPM() returns 140.0 ± 0.5
 *           And tap_count = 64
 * 
 * @calculation
 * - 140 BPM = 140 beats per minute
 * - Interval = 60,000,000 µs / 140 = 428,571 µs ≈ 428.6ms
 * - 64 taps with consistent interval
 * - Average interval should remain 428,571 µs
 * - BPM = 60,000,000 / 428,571 ≈ 140.0
 */
TEST_F(BPMCalculationCircularBufferTest, FullBuffer_64Taps_140BPM_Accurate) {
    // Arrange: Calculate interval for 140 BPM
    constexpr uint64_t INTERVAL_US = 428571;  // 60,000,000 / 140
    
    // Act: Add 64 taps to fill buffer completely
    addTapsWithInterval(64, INTERVAL_US);
    
    // Assert: BPM should be 140.0 ± 0.5
    EXPECT_NEAR(bpm_->getBPM(), 140.0f, 0.5f)
        << "64 taps at 428.571ms intervals should yield 140 BPM";
    
    EXPECT_EQ(bpm_->getTapCount(), 64)
        << "Buffer should contain exactly 64 taps";
}

// ============================================================================
// AC-BPM-007: 65th tap overwrites oldest
// ============================================================================

/**
 * @test AC-BPM-007: Buffer wrap-around (65th tap) should maintain accuracy
 * 
 * @scenario Given buffer filled with 64 taps at 140 BPM
 *           When user adds 65th tap at same interval
 *           Then getBPM() still returns 140.0 ± 0.5
 *           And tap_count remains 64 (buffer full)
 *           And oldest tap (tap 0) is overwritten
 * 
 * @rationale Circular buffer should wrap at index 0, oldest data lost
 */
TEST_F(BPMCalculationCircularBufferTest, WrapAround_65thTap_OverwritesOldest) {
    // Arrange: Fill buffer with 64 taps at 140 BPM
    constexpr uint64_t INTERVAL_US = 428571;
    addTapsWithInterval(64, INTERVAL_US);
    
    float bpm_before_wrap = bpm_->getBPM();
    
    // Act: Add 65th tap (should overwrite oldest)
    uint64_t tap_65_timestamp = 64 * INTERVAL_US;
    mockTiming_->setTimestamp(tap_65_timestamp);
    bpm_->addTap(tap_65_timestamp);
    
    // Assert: BPM should remain stable
    EXPECT_NEAR(bpm_->getBPM(), 140.0f, 0.5f)
        << "65th tap should maintain BPM accuracy after wrap";
    
    EXPECT_FLOAT_EQ(bpm_->getBPM(), bpm_before_wrap)
        << "BPM should not change when adding tap at same interval";
    
    EXPECT_EQ(bpm_->getTapCount(), 64)
        << "Tap count should remain 64 (buffer full, oldest overwritten)";
}

// ============================================================================
// Edge Case: Multiple wrap-arounds (128 taps = 2 full rotations)
// ============================================================================

/**
 * @test Edge case: Multiple buffer rotations should maintain accuracy
 * 
 * @scenario Given buffer starts empty
 *           When user adds 128 taps at 120 BPM (2 full buffer rotations)
 *           Then getBPM() returns 120.0 ± 0.5
 *           And tap_count = 64 (last 64 taps retained)
 * 
 * @rationale Multiple wrap-arounds should work correctly
 */
TEST_F(BPMCalculationCircularBufferTest, MultipleWraps_128Taps_Accurate) {
    // Arrange: 120 BPM = 500ms intervals
    constexpr uint64_t INTERVAL_US = 500000;
    
    // Act: Add 128 taps (2× buffer size)
    addTapsWithInterval(128, INTERVAL_US);
    
    // Assert: Should use last 64 taps for BPM
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 0.5f)
        << "128 taps (2 rotations) should yield accurate BPM from last 64";
    
    EXPECT_EQ(bpm_->getTapCount(), 64)
        << "Buffer should contain last 64 taps only";
}

// ============================================================================
// Edge Case: Verify BPM accuracy across wrap boundary
// ============================================================================

/**
 * @test Edge case: BPM calculation should be accurate across wrap boundary
 * 
 * @scenario Given buffer filled with 64 taps at 130 BPM
 *           When user adds 10 more taps (wraps to index 10)
 *           Then getBPM() uses intervals spanning wrap boundary correctly
 *           And result is 130.0 ± 0.5
 */
TEST_F(BPMCalculationCircularBufferTest, WrapBoundary_IntervalCalculation_Accurate) {
    // Arrange: 130 BPM = 461,538 µs intervals
    constexpr uint64_t INTERVAL_US = 461538;
    
    // Act: Add 74 taps (wraps 10 times past start)
    addTapsWithInterval(74, INTERVAL_US);
    
    // Assert: BPM should be calculated from last 64 taps
    EXPECT_NEAR(bpm_->getBPM(), 130.0f, 0.5f)
        << "BPM calculation should handle wrap boundary correctly";
    
    EXPECT_EQ(bpm_->getTapCount(), 64);
}

// ============================================================================
// Edge Case: Tempo change after buffer fills
// ============================================================================

/**
 * @test Edge case: BPM should adapt when tempo changes after buffer fills
 * 
 * @scenario Given buffer filled with 64 taps at 120 BPM
 *           When user taps at new tempo 140 BPM for next 32 taps
 *           Then BPM gradually shifts toward 140
 *           And eventually converges to 140 ± 0.5
 * 
 * @rationale Buffer should adapt to tempo changes over time
 */
TEST_F(BPMCalculationCircularBufferTest, TempoChange_AfterBufferFull_Adapts) {
    // Arrange: Fill buffer with 120 BPM
    constexpr uint64_t INTERVAL_120 = 500000;  // 120 BPM
    addTapsWithInterval(64, INTERVAL_120);
    
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 0.5f)
        << "Initial BPM should be 120";
    
    // Act: Add 64 more taps at 140 BPM (overwrites all old taps)
    constexpr uint64_t INTERVAL_140 = 428571;  // 140 BPM
    uint64_t start_timestamp = 64 * INTERVAL_120;
    
    for (int i = 0; i < 64; ++i) {
        uint64_t timestamp = start_timestamp + (i * INTERVAL_140);
        mockTiming_->setTimestamp(timestamp);
        bpm_->addTap(timestamp);
    }
    
    // Assert: BPM should now be 140 (all old taps overwritten)
    EXPECT_NEAR(bpm_->getBPM(), 140.0f, 0.5f)
        << "After 64 new taps, BPM should reflect new tempo";
}

// ============================================================================
// Edge Case: Consistent intervals through wrap
// ============================================================================

/**
 * @test Edge case: Interval consistency through buffer wrap
 * 
 * @scenario Given 60 taps added at 150 BPM
 *           When 10 more taps added (wraps at 64, continues to 70)
 *           Then all intervals should be consistent
 *           And BPM = 150.0 ± 0.5
 */
TEST_F(BPMCalculationCircularBufferTest, ConsistentIntervals_ThroughWrap_Maintained) {
    // Arrange & Act: 150 BPM = 400,000 µs = 400ms
    constexpr uint64_t INTERVAL_US = 400000;
    addTapsWithInterval(70, INTERVAL_US);
    
    // Assert
    EXPECT_NEAR(bpm_->getBPM(), 150.0f, 0.5f)
        << "Intervals should remain consistent through wrap";
    
    EXPECT_EQ(bpm_->getTapCount(), 64)
        << "Buffer maxes at 64 taps";
}
