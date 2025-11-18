/**
 * @file test_rtc_health.cpp
 * @brief Unit tests for DES-I-002 (RTC Health Status Interface)
 * 
 * Verifies: DES-I-002 (RTC Health Status)
 * GitHub Issue: #56
 * Acceptance Criteria: AC-TIME-004, AC-TIME-005, AC-TIME-006
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
 * Test Fixture for RTC Health tests
 */
class RTCHealthTest : public ::testing::Test {
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
// TDD Cycle 2: DES-I-002 RTC Health Status - Detection
//==============================================================================

/**
 * TEST: AC-TIME-004 - Automatic fallback to millis() when RTC unavailable
 * 
 * Acceptance Criteria:
 * - System uses micros() fallback when RTC not detected
 * - Timestamps still monotonic in fallback mode
 * - rtcHealthy() returns false when using fallback
 * 
 * Method: Check state after init() in native environment (no RTC)
 */
TEST_F(RTCHealthTest, UsesAutomaticFallback_WhenRTCNotAvailable) {
    // Arrange: Native build has no RTC hardware
    // Act: Already initialized in SetUp()
    
    // Assert: Should be using fallback
    const TimingManagerState& state = timing_manager->getState();
    
    #ifdef NATIVE_BUILD
    // Native environment: No RTC, should use fallback
    EXPECT_TRUE(state.using_fallback) << "Should use fallback when RTC not available";
    EXPECT_FALSE(state.rtc_available) << "RTC should not be detected in native build";
    EXPECT_FALSE(timing_manager->rtcHealthy()) << "rtcHealthy() should return false in fallback";
    #else
    // ESP32 environment: May have RTC, test should adapt
    if (!state.rtc_available) {
        EXPECT_TRUE(state.using_fallback) << "Should use fallback when RTC not detected";
        EXPECT_FALSE(timing_manager->rtcHealthy()) << "rtcHealthy() should match RTC availability";
    }
    #endif
}

/**
 * TEST: AC-TIME-004 - Timestamps remain valid in fallback mode
 * 
 * Verifies that fallback mode still provides monotonic timestamps
 */
TEST_F(RTCHealthTest, ProvidesValidTimestamps_InFallbackMode) {
    // Arrange: Force fallback mode (already in fallback in native)
    const TimingManagerState& state = timing_manager->getState();
    ASSERT_TRUE(state.using_fallback) << "Test requires fallback mode";
    
    // Act: Get multiple timestamps
    uint64_t ts1 = timing_manager->getTimestampUs();
    uint64_t ts2 = timing_manager->getTimestampUs();
    uint64_t ts3 = timing_manager->getTimestampUs();
    
    // Assert: Monotonicity maintained in fallback
    EXPECT_GE(ts2, ts1) << "Timestamps must be monotonic in fallback mode";
    EXPECT_GE(ts3, ts2) << "Timestamps must be monotonic in fallback mode";
}

//==============================================================================
// TDD Cycle 2: DES-I-002 RTC Health Status - Temperature Reading
//==============================================================================

/**
 * TEST: AC-TIME-006 - RTC temperature reading
 * 
 * Acceptance Criteria:
 * - Returns 0.0 when RTC unavailable (fallback mode)
 * - Would return 20-30°C range in real hardware with RTC
 * 
 * Method: Call getRtcTemperature() in fallback mode
 */
TEST_F(RTCHealthTest, GetRtcTemperature_ReturnsZero_InFallbackMode) {
    // Arrange: Verify in fallback mode
    const TimingManagerState& state = timing_manager->getState();
    ASSERT_TRUE(state.using_fallback) << "Test requires fallback mode";
    
    // Act: Get temperature reading
    float temperature = timing_manager->getRtcTemperature();
    
    // Assert: Should return 0.0 when RTC unavailable
    EXPECT_FLOAT_EQ(temperature, 0.0f) << "Temperature should be 0.0 in fallback mode";
}

/**
 * TEST: Temperature range validation (for future hardware testing)
 * 
 * This test will pass in native (returns 0.0) but validates
 * reasonable range for real RTC hardware
 */
TEST_F(RTCHealthTest, GetRtcTemperature_ReturnsReasonableRange_WhenRTCAvailable) {
    // Arrange: Get current state
    const TimingManagerState& state = timing_manager->getState();
    float temperature = timing_manager->getRtcTemperature();
    
    // Act & Assert: Validate based on RTC availability
    if (state.rtc_available && state.rtc_healthy) {
        // Real RTC: Should be room temperature range
        EXPECT_GE(temperature, -40.0f) << "RTC temp should be >= -40°C";
        EXPECT_LE(temperature, 85.0f) << "RTC temp should be <= 85°C (RTC3231 range)";
        
        // Typical room temperature: 15-35°C
        // (This might fail in extreme environments, but that's informative)
        if (temperature < 15.0f || temperature > 35.0f) {
            GTEST_SKIP() << "Temperature " << temperature 
                        << "°C is outside typical room range (15-35°C) - may indicate environmental issue";
        }
    } else {
        // No RTC: Should return 0.0
        EXPECT_FLOAT_EQ(temperature, 0.0f) << "Temperature should be 0.0 without RTC";
    }
}

//==============================================================================
// TDD Cycle 2: DES-I-002 RTC Health Status - State Tracking
//==============================================================================

/**
 * TEST: RTC health state initialization
 * 
 * Verifies state is properly initialized on startup
 */
TEST_F(RTCHealthTest, InitializesHealthState_Correctly) {
    // Arrange & Act: State initialized in SetUp()
    const TimingManagerState& state = timing_manager->getState();
    
    // Assert: Health state fields should be initialized
    EXPECT_GE(state.i2c_error_count, 0) << "Error count should be non-negative";
    
    #ifdef NATIVE_BUILD
    // Native: Should start in fallback
    EXPECT_TRUE(state.using_fallback) << "Native should use fallback";
    EXPECT_EQ(state.i2c_error_count, 0) << "No I2C errors in native (no I2C bus)";
    #endif
}

/**
 * TEST: rtcHealthy() consistency with state
 * 
 * Verifies rtcHealthy() returns value consistent with internal state
 */
TEST_F(RTCHealthTest, RtcHealthy_ConsistentWithState) {
    // Arrange: Get state and health status
    const TimingManagerState& state = timing_manager->getState();
    bool healthy = timing_manager->rtcHealthy();
    
    // Assert: rtcHealthy() should match state.rtc_healthy
    EXPECT_EQ(healthy, state.rtc_healthy) 
        << "rtcHealthy() should return consistent value with state";
    
    // If using fallback, RTC cannot be healthy
    if (state.using_fallback) {
        EXPECT_FALSE(healthy) << "RTC cannot be healthy when using fallback";
    }
}

//==============================================================================
// Test Entry Point
//==============================================================================

/**
 * Main function for native test execution
 * 
 * Usage: 
 * - CMake: .\test\test_timing\build\Debug\test_rtc_health.exe
 * - PlatformIO: pio test -e native -f test_rtc_health
 */
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
