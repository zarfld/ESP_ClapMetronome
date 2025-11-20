/**
 * @file test_clear_reset.cpp
 * @brief Unit tests for BPM clear/reset functionality (AC-BPM-012)
 * 
 * Tests clear() method:
 * - Resets tap count to 0
 * - Resets BPM to 0
 * - Resets stability flag to false
 * - Clears circular buffer
 * - Allows starting fresh after clear
 * 
 * @component DES-C-002: BPM Calculation Engine
 * @requirement AC-BPM-012: Clear/reset functionality
 * @phase Wave 2.2, Cycle 6 of 7
 * @standards ISO/IEC/IEEE 12207:2017 (Implementation), XP TDD
 * @date 2025-11-20
 */

#include <gtest/gtest.h>
#include "../../src/bpm/BPMCalculation.h"
#include "../../src/bpm/BPMCalculationState.h"
#include "../../test/mocks/MockTimingProvider.h"

using namespace clap_metronome;

/**
 * @brief Test fixture for clear/reset tests
 */
class BPMClearResetTest : public ::testing::Test {
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
     * Helper: Add N taps with specified interval
     */
    void addTapsWithInterval(int count, uint64_t interval_us) {
        uint64_t timestamp = 0;
        for (int i = 0; i < count; ++i) {
            mockTiming_->setTimestamp(timestamp);
            bpm_->addTap(timestamp);
            timestamp += interval_us;
        }
    }
};

/**
 * @brief Test: clear() resets tap count to 0
 * 
 * AC-BPM-012: clear() must reset all state
 */
TEST_F(BPMClearResetTest, Clear_ResetsTapCount) {
    // Arrange: Add some taps
    addTapsWithInterval(5, 500000); // 5 taps at 500ms
    EXPECT_EQ(bpm_->getTapCount(), 5) << "Should have 5 taps before clear";
    
    // Act: Clear
    bpm_->clear();
    
    // Assert: Tap count reset
    EXPECT_EQ(bpm_->getTapCount(), 0)
        << "Tap count should be 0 after clear";
}

/**
 * @brief Test: clear() resets BPM to 0
 */
TEST_F(BPMClearResetTest, Clear_ResetsBPM) {
    // Arrange: Establish BPM
    addTapsWithInterval(4, 500000); // 120 BPM
    EXPECT_NEAR(bpm_->getBPM(), 120.0f, 0.5f) << "Should have 120 BPM";
    
    // Act: Clear
    bpm_->clear();
    
    // Assert: BPM reset
    EXPECT_EQ(bpm_->getBPM(), 0.0f)
        << "BPM should be 0.0 after clear";
}

/**
 * @brief Test: clear() resets stability flag
 */
TEST_F(BPMClearResetTest, Clear_ResetsStability) {
    // Arrange: Achieve stable state
    addTapsWithInterval(10, 500000); // Consistent intervals = stable
    EXPECT_TRUE(bpm_->isStable()) << "Should be stable with consistent taps";
    
    // Act: Clear
    bpm_->clear();
    
    // Assert: Stability reset
    EXPECT_FALSE(bpm_->isStable())
        << "isStable should be false after clear";
}

/**
 * @brief Test: clear() resets coefficient of variation
 */
TEST_F(BPMClearResetTest, Clear_ResetsCV) {
    // Arrange: Add taps with variance to get non-zero CV
    uint64_t timestamp = 0;
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    
    timestamp += 500000; // 500ms
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    
    timestamp += 600000; // 600ms (different interval)
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    
    timestamp += 450000; // 450ms (different interval)
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    
    timestamp += 550000; // 550ms (different interval)
    mockTiming_->setTimestamp(timestamp);
    bpm_->addTap(timestamp);
    
    float cv_before = bpm_->getCoefficientOfVariation();
    EXPECT_GT(cv_before, 0.0f) << "CV should be > 0 with varied intervals";
    
    // Act: Clear
    bpm_->clear();
    
    // Assert: CV reset
    EXPECT_EQ(bpm_->getCoefficientOfVariation(), 0.0f)
        << "CV should be 0.0 after clear";
}

/**
 * @brief Test: Can add taps after clear (buffer cleared)
 * 
 * Ensures circular buffer is properly reset
 */
TEST_F(BPMClearResetTest, Clear_AllowsNewTaps) {
    // Arrange: Fill buffer
    addTapsWithInterval(10, 500000);
    EXPECT_EQ(bpm_->getTapCount(), 10);
    
    // Act: Clear and add new taps
    bpm_->clear();
    addTapsWithInterval(3, 400000); // 150 BPM
    
    // Assert: New taps work correctly
    EXPECT_EQ(bpm_->getTapCount(), 3)
        << "Should have 3 new taps";
    EXPECT_NEAR(bpm_->getBPM(), 150.0f, 0.5f)
        << "BPM should be based on new taps only";
}

/**
 * @brief Test: Multiple clear() calls are safe
 * 
 * Idempotency test
 */
TEST_F(BPMClearResetTest, MultipleClear_Safe) {
    // Arrange: Add taps
    addTapsWithInterval(5, 500000);
    
    // Act: Clear multiple times
    bpm_->clear();
    bpm_->clear();
    bpm_->clear();
    
    // Assert: Still in reset state
    EXPECT_EQ(bpm_->getTapCount(), 0);
    EXPECT_EQ(bpm_->getBPM(), 0.0f);
    EXPECT_FALSE(bpm_->isStable());
}

/**
 * @brief Test: Clear on empty state is safe
 * 
 * Edge case: clearing when already empty
 */
TEST_F(BPMClearResetTest, ClearEmpty_Safe) {
    // Arrange: No taps added (already empty)
    EXPECT_EQ(bpm_->getTapCount(), 0);
    
    // Act: Clear empty state
    bpm_->clear();
    
    // Assert: Still empty
    EXPECT_EQ(bpm_->getTapCount(), 0);
    EXPECT_EQ(bpm_->getBPM(), 0.0f);
    EXPECT_FALSE(bpm_->isStable());
}

/**
 * @brief Test: Clear resets all internal state variables
 * 
 * Comprehensive check that all state is reset
 */
TEST_F(BPMClearResetTest, Clear_ResetsAllInternalState) {
    // Arrange: Establish full state
    addTapsWithInterval(10, 500000); // Baseline at 120 BPM
    
    // Verify state is populated
    EXPECT_GT(bpm_->getTapCount(), 0);
    EXPECT_GT(bpm_->getBPM(), 0.0f);
    
    // Act: Clear
    bpm_->clear();
    
    // Assert: All state reset
    EXPECT_EQ(bpm_->getTapCount(), 0) << "Tap count should be 0";
    EXPECT_EQ(bpm_->getBPM(), 0.0f) << "BPM should be 0";
    EXPECT_FALSE(bpm_->isStable()) << "Stability should be false";
    EXPECT_EQ(bpm_->getCoefficientOfVariation(), 0.0f) << "CV should be 0";
    
    // Add new taps - should start fresh
    addTapsWithInterval(3, 400000); // 150 BPM
    EXPECT_NEAR(bpm_->getBPM(), 150.0f, 0.5f) 
        << "Should calculate BPM from fresh start";
}
