/**
 * BPM Calculation - Basic Tap Addition Tests
 * 
 * @file test_basic_taps.cpp
 * @component DES-C-002: BPM Calculation Engine
 * @verifies AC-BPM-001: Single tap returns BPM = 0
 * @verifies AC-BPM-002: 4 taps at 120 BPM returns 120.0 ± 0.5
 * 
 * @test_type Unit Test
 * @phase Phase 05 - Implementation (TDD Cycle 1)
 * @standards ISO/IEC/IEEE 12207:2017, XP Test-First
 * 
 * @description
 * Tests basic tap addition and BPM calculation for small tap counts.
 * 
 * Test Scenarios:
 * 1. AC-BPM-001: Single tap returns 0 BPM (need ≥2 taps for interval)
 * 2. AC-BPM-002: 4 taps at 120 BPM (500ms intervals) returns 120.0 ± 0.5
 * 3. Edge case: 2 taps (minimum for BPM calculation)
 * 4. Edge case: 3 taps (verify averaging works)
 * 
 * @see https://github.com/zarfld/ESP_ClapMetronome/issues/46
 * @date 2025-11-20
 */

#include <gtest/gtest.h>
#include "../../src/bpm/BPMCalculation.h"
#include "../../test/mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * Test Fixture for BPM Calculation Basic Tests
 * 
 * Sets up:
 * - Mock timing provider (controllable timestamps)
 * - BPM calculation instance
 */
class BPMCalculationBasicTest : public ::testing::Test {
protected:
    MockTimingProvider* mockTiming_;
    BPMCalculation* bpm_;
    
    void SetUp() override {
        mockTiming_ = new MockTimingProvider();
        mockTiming_->setTimestamp(0);  // Start at T=0
        
        bpm_ = new BPMCalculation(mockTiming_);
        bpm_->init();
    }
    
    void TearDown() override {
        delete bpm_;
        delete mockTiming_;
    }
    
    /**
     * Helper: Add tap at specific timestamp
     */
    void addTapAt(uint64_t timestamp_us) {
        mockTiming_->setTimestamp(timestamp_us);
        bpm_->addTap(timestamp_us);
    }
    
    /**
     * Helper: Add N taps with fixed interval
     * @param count Number of taps
     * @param interval_us Interval between taps in microseconds
     */
    void addTapsWithInterval(int count, uint64_t interval_us) {
        for (int i = 0; i < count; ++i) {
            uint64_t timestamp = i * interval_us;
            addTapAt(timestamp);
        }
    }
};

// ============================================================================
// AC-BPM-001: Single tap returns BPM = 0
// ============================================================================

/**
 * @test AC-BPM-001: Single tap should return 0 BPM
 * 
 * @scenario Given BPM calculator initialized
 *           When user adds 1 tap
 *           Then getBPM() returns 0.0
 *           And isStable() returns false
 * 
 * @rationale Need at least 2 taps to calculate an interval
 */
TEST_F(BPMCalculationBasicTest, SingleTap_ReturnsZeroBPM) {
    // Arrange: Already initialized in SetUp()
    
    // Act: Add single tap at T=0
    addTapAt(0);
    
    // Assert: BPM should be 0 (insufficient data)
    EXPECT_FLOAT_EQ(bpm_->getBPM(), 0.0f) 
        << "Single tap should return 0 BPM (need ≥2 taps for interval)";
    
    EXPECT_FALSE(bpm_->isStable()) 
        << "Single tap cannot be stable";
    
    EXPECT_EQ(bpm_->getTapCount(), 1) 
        << "Tap count should be 1";
}

// ============================================================================
// AC-BPM-002: 4 taps at 120 BPM returns 120.0 ± 0.5
// ============================================================================

/**
 * @test AC-BPM-002: Four taps at 120 BPM should return 120.0 ± 0.5
 * 
 * @scenario Given BPM calculator initialized
 *           When user adds 4 taps at 120 BPM (500ms intervals)
 *           Then getBPM() returns 120.0 ± 0.5
 * 
 * @calculation
 * - 120 BPM = 120 beats per minute
 * - Interval = 60,000,000 µs / 120 = 500,000 µs = 500ms
 * - Timestamps: T=0, T=500ms, T=1000ms, T=1500ms
 * - Average interval = (500 + 500 + 500) / 3 = 500ms
 * - BPM = 60,000,000 / 500,000 = 120.0
 */
TEST_F(BPMCalculationBasicTest, FourTaps_120BPM_ReturnsCorrectBPM) {
    // Arrange: Calculate interval for 120 BPM
    constexpr uint64_t INTERVAL_US = 500000;  // 60,000,000 / 120 = 500ms
    
    // Act: Add 4 taps at 120 BPM
    addTapsWithInterval(4, INTERVAL_US);
    
    // Assert: BPM should be 120.0 ± 0.5
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 0.5f) 
        << "4 taps at 500ms intervals should yield 120 BPM";
    
    EXPECT_EQ(bpm_->getTapCount(), 4) 
        << "Tap count should be 4";
}

// ============================================================================
// Edge Case: 2 taps (minimum for BPM calculation)
// ============================================================================

/**
 * @test Edge case: Two taps should calculate BPM from single interval
 * 
 * @scenario Given BPM calculator initialized
 *           When user adds 2 taps at 500ms apart
 *           Then getBPM() returns 120.0 (60,000,000 / 500,000)
 *           And isStable() returns false (need more samples)
 */
TEST_F(BPMCalculationBasicTest, TwoTaps_MinimumForBPM_ReturnsCalculatedBPM) {
    // Arrange & Act: Add 2 taps at 500ms interval (120 BPM)
    addTapAt(0);
    addTapAt(500000);  // 500ms = 500,000 µs
    
    // Assert: Should calculate BPM from single interval
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 0.5f) 
        << "2 taps at 500ms interval should yield 120 BPM";
    
    EXPECT_FALSE(bpm_->isStable()) 
        << "2 taps insufficient for stability (need more data)";
    
    EXPECT_EQ(bpm_->getTapCount(), 2);
}

// ============================================================================
// Edge Case: 3 taps (verify averaging works)
// ============================================================================

/**
 * @test Edge case: Three taps should average 2 intervals
 * 
 * @scenario Given BPM calculator initialized
 *           When user adds 3 taps with varying intervals
 *           Then getBPM() averages the 2 intervals correctly
 * 
 * @calculation
 * - Tap 1: T=0
 * - Tap 2: T=500ms (interval 1 = 500ms)
 * - Tap 3: T=1100ms (interval 2 = 600ms)
 * - Average interval = (500 + 600) / 2 = 550ms
 * - BPM = 60,000,000 / 550,000 ≈ 109.1
 */
TEST_F(BPMCalculationBasicTest, ThreeTaps_VaryingIntervals_AveragesCorrectly) {
    // Arrange & Act: Add 3 taps with 500ms and 600ms intervals
    addTapAt(0);
    addTapAt(500000);   // +500ms
    addTapAt(1100000);  // +600ms
    
    // Assert: BPM should be 60,000,000 / 550,000 ≈ 109.09
    EXPECT_NEAR(bpm_->getBPM(), 109.1f, 0.5f) 
        << "3 taps with 500ms and 600ms intervals should average to ~109 BPM";
    
    EXPECT_EQ(bpm_->getTapCount(), 3);
}

// ============================================================================
// Edge Case: Zero taps (initial state)
// ============================================================================

/**
 * @test Edge case: Newly initialized BPM calculator returns 0
 * 
 * @scenario Given BPM calculator just initialized
 *           When no taps added yet
 *           Then getBPM() returns 0.0
 */
TEST_F(BPMCalculationBasicTest, NoTaps_InitialState_ReturnsZero) {
    // Act: Query BPM without adding any taps
    
    // Assert: Should return 0
    EXPECT_FLOAT_EQ(bpm_->getBPM(), 0.0f);
    EXPECT_FALSE(bpm_->isStable());
    EXPECT_EQ(bpm_->getTapCount(), 0);
}

// ============================================================================
// Helper Method Tests
// ============================================================================

/**
 * @test Verify getTapCount() tracks additions correctly
 */
TEST_F(BPMCalculationBasicTest, GetTapCount_TracksAdditionsCorrectly) {
    EXPECT_EQ(bpm_->getTapCount(), 0);
    
    addTapAt(0);
    EXPECT_EQ(bpm_->getTapCount(), 1);
    
    addTapAt(500000);
    EXPECT_EQ(bpm_->getTapCount(), 2);
    
    addTapAt(1000000);
    EXPECT_EQ(bpm_->getTapCount(), 3);
}
