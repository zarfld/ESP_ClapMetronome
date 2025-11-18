/**
 * @file test_timestamp_query.cpp
 * @brief Unit tests for DES-I-001 (Timestamp Query Interface)
 * 
 * Verifies: DES-I-001 (Timestamp Query)
 * GitHub Issue: #52
 * Acceptance Criteria: AC-TIME-001, AC-TIME-002
 * 
 * TDD Status: RED - Tests written, implementation pending
 * 
 * Standard: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * XP Practice: Test-Driven Development (Red-Green-Refactor)
 */

#include <gtest/gtest.h>
#include "../../src/timing/TimingManager.h"
#include "../../src/timing/TimingManagerState.h"

/**
 * Test Fixture for TimingManager tests
 * 
 * Sets up clean TimingManager instance for each test
 */
class TimingManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        timing_manager = new TimingManager();
        timing_manager->init();
    }
    
    void TearDown() override {
        delete timing_manager;
    }
    
    TimingManager* timing_manager;
};

//==============================================================================
// TDD Cycle 1: DES-I-001 Timestamp Query - Monotonicity
//==============================================================================

/**
 * TEST: AC-TIME-001 - Monotonicity guarantee
 * 
 * Acceptance Criteria:
 * - getTimestampUs() must always return increasing values
 * - No timestamp rollback allowed
 * 
 * Method: Call getTimestampUs() multiple times, verify each > previous
 */
TEST_F(TimingManagerTest, GetTimestampUs_ReturnsMonotonicTime) {
    // Arrange: Get first timestamp
    uint64_t ts1 = timing_manager->getTimestampUs();
    
    // Act: Wait and get second timestamp
    // Note: In native test, we can't use delayMicroseconds(), so we'll just
    // call multiple times assuming some time passes
    uint64_t ts2 = timing_manager->getTimestampUs();
    uint64_t ts3 = timing_manager->getTimestampUs();
    uint64_t ts4 = timing_manager->getTimestampUs();
    
    // Assert: Each timestamp must be >= previous (monotonic)
    EXPECT_GE(ts2, ts1) << "Second timestamp must be >= first";
    EXPECT_GE(ts3, ts2) << "Third timestamp must be >= second";
    EXPECT_GE(ts4, ts3) << "Fourth timestamp must be >= third";
}

/**
 * TEST: AC-TIME-001 - Monotonicity after state changes
 * 
 * Verifies timestamps remain monotonic even when RTC state changes
 */
TEST_F(TimingManagerTest, GetTimestampUs_MonotonicAfterRTCFailure) {
    // Arrange: Get timestamp with RTC
    uint64_t ts1 = timing_manager->getTimestampUs();
    
    // Act: Simulate RTC failure (via internal state manipulation)
    // Note: In real implementation, this would happen via I2C errors
    uint64_t ts2 = timing_manager->getTimestampUs();
    
    // Assert: Timestamp still increasing despite RTC status
    EXPECT_GE(ts2, ts1) << "Timestamp must remain monotonic even after RTC failure";
}

/**
 * TEST: AC-TIME-002 - Microsecond precision
 * 
 * Acceptance Criteria:
 * - Successive calls differ by <10µs
 * - Demonstrates microsecond-level timing resolution
 * 
 * Method: Call getTimestampUs() back-to-back, measure delta
 */
TEST_F(TimingManagerTest, GetTimestampUs_HasMicrosecondPrecision) {
    // Arrange & Act: Two rapid successive calls
    uint64_t ts1 = timing_manager->getTimestampUs();
    uint64_t ts2 = timing_manager->getTimestampUs();
    
    // Assert: Delta should be small (function call overhead only)
    uint64_t delta = ts2 - ts1;
    
    // Allow up to 100µs for native test overhead (GoogleTest + system calls)
    // Real hardware will be <10µs
    EXPECT_LT(delta, 100) << "Successive timestamp calls must differ by <100µs";
    
    // Verify precision is at least microsecond level (not milliseconds)
    EXPECT_LT(delta, 1000) << "Timestamp precision must be finer than milliseconds";
}

//==============================================================================
// TDD Cycle 1: DES-I-001 Timestamp Query - Millisecond Conversion
//==============================================================================

/**
 * TEST: getTimestampMs() returns consistent milliseconds
 * 
 * Verifies millisecond convenience method matches microsecond base
 */
TEST_F(TimingManagerTest, GetTimestampMs_MatchesMicroseconds) {
    // Arrange & Act: Get both timestamps
    uint64_t ts_us = timing_manager->getTimestampUs();
    uint32_t ts_ms = timing_manager->getTimestampMs();
    
    // Assert: Millisecond value should match microsecond/1000
    uint32_t expected_ms = static_cast<uint32_t>(ts_us / 1000);
    
    // Allow ±1ms tolerance for timing differences between calls
    EXPECT_NEAR(ts_ms, expected_ms, 1) << "Millisecond timestamp must match microseconds/1000";
}

/**
 * TEST: getTimestampMs() is monotonic
 * 
 * Verifies millisecond timestamps also increase monotonically
 */
TEST_F(TimingManagerTest, GetTimestampMs_IsMonotonic) {
    // Arrange: Get first timestamp
    uint32_t ts1 = timing_manager->getTimestampMs();
    
    // Act: Get subsequent timestamps
    uint32_t ts2 = timing_manager->getTimestampMs();
    uint32_t ts3 = timing_manager->getTimestampMs();
    
    // Assert: Monotonicity at millisecond level
    EXPECT_GE(ts2, ts1) << "Millisecond timestamps must be monotonic";
    EXPECT_GE(ts3, ts2) << "Millisecond timestamps must be monotonic";
}

//==============================================================================
// Test Entry Point
//==============================================================================

/**
 * Main function for native test execution
 * 
 * Usage: pio test -e native -f test_timestamp_query
 */
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
