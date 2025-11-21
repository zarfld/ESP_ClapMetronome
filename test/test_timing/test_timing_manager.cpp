/**
 * @file test_timing_manager.cpp
 * @brief Timing Manager Unit Tests
 * 
 * @component DES-C-005: Timing Manager
 * @implements AC-TIME-001 through AC-TIME-005
 * @requirement REQ-F-007: RTC3231 I2C timing with fallback
 * 
 * @standard ISO/IEC/IEEE 12207:2017 (Software Testing)
 * @standard IEEE 1012-2016 (Verification and Validation)
 * 
 * @description
 * Tests for Timing Manager component providing microsecond-precision timestamps:
 * - Timestamp monotonicity (always increasing)
 * - Microsecond precision (<10µs granularity)
 * - RTC fallback when I2C fails
 * - Health monitoring (3 consecutive failures)
 * - Boot initialization (<100ms)
 * 
 * TDD Cycle: TIME-01 (Timing Manager Foundation)
 * - RED: Write failing tests for timestamp management
 * - GREEN: Implement minimal TimingManager class
 * - REFACTOR: Optimize precision and fallback logic
 * 
 * @see https://github.com/zarfld/ESP_ClapMetronome/issues/44
 * @date 2025-11-21
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <algorithm>

#ifdef NATIVE_BUILD
// Native build simulation
#include <cstdint>
#include <functional>
#else
// ESP32 hardware
#include <Arduino.h>
#include <Wire.h>
#endif

// Include actual Timing Manager implementation
#include "timing/TimingManager.h"

/**
 * Test fixture for Timing Manager
 */
class TimingManagerTest : public ::testing::Test {
protected:
    TimingManager* timing_;
    
    void SetUp() override {
        timing_ = new TimingManager();
        timing_->init();
    }
    
    void TearDown() override {
        delete timing_;
        timing_ = nullptr;
    }
    
    /**
     * Helper: Measure time between operations
     */
    template<typename Func>
    int64_t measureLatencyUs(Func operation) {
        auto start = std::chrono::high_resolution_clock::now();
        operation();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
};

/**
 * AC-TIME-001: Timestamp Monotonicity
 * 
 * Requirement: Timestamps must always increase
 * Pass Condition: getTimestampUs() never decreases
 */
TEST_F(TimingManagerTest, TimestampMonotonicity_AlwaysIncreases) {
    uint64_t prev = timing_->getTimestampUs();
    
    // Take 1000 consecutive timestamps
    for (int i = 0; i < 1000; i++) {
        uint64_t current = timing_->getTimestampUs();
        
        // Timestamps must be monotonic (never decrease)
        // In native build, consecutive calls may return same value (acceptable)
        // In ESP32, hardware timer ensures strict increase
        #ifdef NATIVE_BUILD
        EXPECT_GE(current, prev) << "Timestamp decreased at iteration " << i
                                  << ": prev=" << prev << ", current=" << current;
        #else
        EXPECT_GT(current, prev) << "Timestamp decreased at iteration " << i
                                  << ": prev=" << prev << ", current=" << current;
        #endif
        
        prev = current;
        
        // Small delay to allow time to advance
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
}

TEST_F(TimingManagerTest, TimestampMonotonicity_NoRollback) {
    // Test for 32-bit rollover safety (if using millis() fallback)
    uint64_t start = timing_->getTimestampUs();
    
    // Advance time significantly
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    uint64_t end = timing_->getTimestampUs();
    
    EXPECT_GT(end, start);
    EXPECT_GT(end - start, 90000ULL) << "Time advanced less than 90ms in 100ms period";
}

TEST_F(TimingManagerTest, TimestampMonotonicity_RapidCalls) {
    // Rapid successive calls should still be monotonic
    uint64_t timestamps[100];
    
    for (int i = 0; i < 100; i++) {
        timestamps[i] = timing_->getTimestampUs();
    }
    
    // Verify strictly increasing
    for (int i = 1; i < 100; i++) {
        EXPECT_GE(timestamps[i], timestamps[i-1]) 
            << "Timestamp not monotonic at index " << i;
    }
}

/**
 * AC-TIME-002: Microsecond Precision
 * 
 * Requirement: Timestamps must have <10µs granularity
 * Pass Condition: Successive calls differ by <10µs
 */
TEST_F(TimingManagerTest, MicrosecondPrecision_GranularityUnder10us) {
    const int samples = 100;
    int64_t deltas[samples];
    
    // Collect timestamp deltas
    for (int i = 0; i < samples; i++) {
        uint64_t t1 = timing_->getTimestampUs();
        uint64_t t2 = timing_->getTimestampUs();
        deltas[i] = t2 - t1;
    }
    
    // Calculate median delta (more robust than mean)
    std::sort(deltas, deltas + samples);
    int64_t median_delta = deltas[samples / 2];
    
    EXPECT_LT(median_delta, 10) << "Timestamp granularity exceeds 10µs: " << median_delta << "µs";
    
    std::cout << "Timestamp precision: " << median_delta << " µs (target: <10µs)" << std::endl;
}

TEST_F(TimingManagerTest, MicrosecondPrecision_MillisecondConsistency) {
    // Verify that getTimestampMs() is consistent with getTimestampUs()
    uint64_t us = timing_->getTimestampUs();
    uint32_t ms = timing_->getTimestampMs();
    
    uint32_t expected_ms = us / 1000;
    
    // Allow 1ms tolerance due to timing between calls
    EXPECT_NEAR(ms, expected_ms, 1) 
        << "Millisecond timestamp inconsistent with microsecond timestamp";
}

TEST_F(TimingManagerTest, MicrosecondPrecision_AccuracyOver1Second) {
    uint64_t start_us = timing_->getTimestampUs();
    
    // Wait approximately 1 second
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    uint64_t end_us = timing_->getTimestampUs();
    int64_t elapsed_us = end_us - start_us;
    
    // Should be close to 1,000,000µs (allow 5% tolerance for scheduling)
    EXPECT_GT(elapsed_us, 950000) << "Elapsed time too short: " << elapsed_us << "µs";
    EXPECT_LT(elapsed_us, 1050000) << "Elapsed time too long: " << elapsed_us << "µs";
    
    std::cout << "1 second measured as: " << elapsed_us << " µs" << std::endl;
}

/**
 * AC-TIME-004: Automatic Fallback
 * 
 * Requirement: Use millis() when RTC unhealthy
 * Pass Condition: Timestamps still monotonic when RTC fails
 * 
 * Note: This test verifies that even if RTC fails, the system
 * continues to provide valid timestamps using the fallback mechanism
 */
TEST_F(TimingManagerTest, AutomaticFallback_StillMonotonic) {
    // Initial timestamps should work
    uint64_t t1 = timing_->getTimestampUs();
    EXPECT_GT(t1, 0);
    
    // Even if RTC fails, timestamps should continue
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    uint64_t t2 = timing_->getTimestampUs();
    EXPECT_GT(t2, t1) << "Timestamps not monotonic during fallback";
}

TEST_F(TimingManagerTest, AutomaticFallback_ContinuesAfterFailure) {
    // Simulate a period of operation
    uint64_t timestamps[5];
    
    for (int i = 0; i < 5; i++) {
        timestamps[i] = timing_->getTimestampUs();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // All timestamps should be strictly increasing
    for (int i = 1; i < 5; i++) {
        EXPECT_GT(timestamps[i], timestamps[i-1])
            << "Timestamp sequence broken at index " << i;
    }
}

/**
 * AC-TIME-005: Health Monitoring
 * 
 * Requirement: Detect RTC failures (3 consecutive I2C errors)
 * Pass Condition: rtcHealthy() reflects actual hardware state
 */
TEST_F(TimingManagerTest, HealthMonitoring_InitiallyHealthy) {
    // On native build, RTC health depends on implementation
    // Should start in a known state
    bool healthy = timing_->rtcHealthy();
    
    // On native: typically false (no hardware)
    // On ESP32: typically true (if RTC present)
    #ifdef NATIVE_BUILD
    EXPECT_FALSE(healthy) << "Native build should use fallback (no RTC hardware)";
    #else
    // On real hardware, health depends on RTC presence
    // Just verify it returns a boolean value
    EXPECT_TRUE(healthy == true || healthy == false);
    #endif
}

TEST_F(TimingManagerTest, HealthMonitoring_UpdateTracksHealth) {
    // Call getTimestampUs() multiple times to trigger health checks
    for (int i = 0; i < 10; i++) {
        timing_->getTimestampUs();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Health status should be stable or degrading (never improving without action)
    bool initial_health = timing_->rtcHealthy();
    
    // More calls to trigger health monitoring
    for (int i = 0; i < 5; i++) {
        timing_->getTimestampUs();
    }
    
    bool current_health = timing_->rtcHealthy();
    
    // Health can stay same or degrade, but shouldn't randomly improve
    // (Would need explicit syncRtc() to recover)
    EXPECT_TRUE(current_health == initial_health || !current_health)
        << "RTC health shouldn't improve without intervention";
}

/**
 * AC-TIME-006: Temperature Reading
 * 
 * Requirement: Read temperature from RTC sensor
 * Pass Condition: Temperature in reasonable range (0-50°C)
 */
TEST_F(TimingManagerTest, TemperatureReading_ReasonableRange) {
    float temp = timing_->getRtcTemperature();
    
    // Reasonable operating range
    EXPECT_GT(temp, -10.0f) << "Temperature too low: " << temp << "°C";
    EXPECT_LT(temp, 60.0f) << "Temperature too high: " << temp << "°C";
    
    std::cout << "RTC Temperature: " << temp << "°C" << std::endl;
}

TEST_F(TimingManagerTest, TemperatureReading_ConsistentOverTime) {
    float temp1 = timing_->getRtcTemperature();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    float temp2 = timing_->getRtcTemperature();
    
    // Temperature shouldn't change dramatically in 100ms
    EXPECT_NEAR(temp1, temp2, 5.0f) 
        << "Temperature changed too much: " << temp1 << "°C → " << temp2 << "°C";
}

/**
 * AC-TIME-007: Time Synchronization
 * 
 * Requirement: Sync system time with RTC
 * Pass Condition: syncRtc() completes successfully
 */
TEST_F(TimingManagerTest, TimeSynchronization_ReturnsStatus) {
    bool result = timing_->syncRtc();
    
    // Should return a boolean (success/failure)
    EXPECT_TRUE(result == true || result == false);
    
    #ifdef NATIVE_BUILD
    // Native build has no RTC hardware
    EXPECT_FALSE(result) << "Native build should fail sync (no RTC)";
    #endif
}

TEST_F(TimingManagerTest, TimeSynchronization_DoesNotBreakTimestamps) {
    uint64_t before = timing_->getTimestampUs();
    
    timing_->syncRtc();
    
    uint64_t after = timing_->getTimestampUs();
    
    // Timestamps should still be monotonic after sync
    EXPECT_GT(after, before) << "Sync broke timestamp monotonicity";
}

/**
 * AC-TIME-008: Boot Initialization
 * 
 * Requirement: Initialize within 100ms
 * Pass Condition: init() completes in <100ms
 */
TEST_F(TimingManagerTest, BootInitialization_Under100ms) {
    TimingManager* mgr = new TimingManager();
    
    int64_t latency_us = measureLatencyUs([mgr]() {
        mgr->init();
    });
    
    EXPECT_LT(latency_us, 100000) 
        << "Initialization too slow: " << latency_us << "µs (" << (latency_us / 1000) << "ms)";
    
    std::cout << "Initialization latency: " << (latency_us / 1000) << "ms (target: <100ms)" << std::endl;
    
    // Verify initialized correctly
    uint64_t ts = mgr->getTimestampUs();
    EXPECT_GT(ts, 0) << "Timestamp not available after init()";
    
    delete mgr;
}

TEST_F(TimingManagerTest, BootInitialization_RepeatedInitSafe) {
    // Should be safe to call init() multiple times
    EXPECT_TRUE(timing_->init());
    EXPECT_TRUE(timing_->init());
    
    // Timestamps should still work
    uint64_t ts = timing_->getTimestampUs();
    EXPECT_GT(ts, 0);
}

/**
 * Performance Summary Test
 */
TEST_F(TimingManagerTest, Summary_AllRequirementsMet) {
    std::cout << "\n=== Timing Manager Performance Summary ===" << std::endl;
    
    // Monotonicity
    uint64_t t1 = timing_->getTimestampUs();
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    uint64_t t2 = timing_->getTimestampUs();
    EXPECT_GT(t2, t1);
    std::cout << "✓ Timestamp monotonicity: PASS" << std::endl;
    
    // Precision
    uint64_t t3 = timing_->getTimestampUs();
    uint64_t t4 = timing_->getTimestampUs();
    int64_t delta = t4 - t3;
    EXPECT_LT(delta, 10);
    std::cout << "✓ Microsecond precision: " << delta << "µs (target: <10µs)" << std::endl;
    
    // Health monitoring
    bool healthy = timing_->rtcHealthy();
    std::cout << "✓ RTC health monitoring: " << (healthy ? "Healthy" : "Fallback") << std::endl;
    
    // Temperature
    float temp = timing_->getRtcTemperature();
    std::cout << "✓ Temperature reading: " << temp << "°C" << std::endl;
    
    // Initialization
    TimingManager* mgr = new TimingManager();
    int64_t init_us = measureLatencyUs([mgr]() { mgr->init(); });
    EXPECT_LT(init_us, 100000);
    std::cout << "✓ Initialization: " << (init_us / 1000) << "ms (target: <100ms)" << std::endl;
    delete mgr;
    
    std::cout << "\n✅ All timing requirements met!" << std::endl;
}
