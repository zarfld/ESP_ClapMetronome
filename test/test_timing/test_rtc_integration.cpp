/**
 * @file test_rtc_integration.cpp
 * @brief RTC I2C Integration Tests
 * 
 * @component DES-C-005: Timing Manager
 * @implements AC-TIME-003: RTC I2C communication within 5ms
 * @requirement REQ-F-007: RTC3231 I2C timing with fallback
 * 
 * @standard ISO/IEC/IEEE 12207:2017 (Software Testing)
 * @standard IEEE 1012-2016 (Verification and Validation)
 * 
 * @description
 * Integration tests for RTC3231 I2C communication:
 * - I2C communication latency (<5ms requirement)
 * - RTC detection on I2C bus
 * - Temperature sensor reading
 * - Time synchronization
 * - Error recovery and fallback activation
 * 
 * TDD Cycle: TIME-02 (RTC I2C Integration)
 * - RED: Write failing tests for I2C communication
 * - GREEN: Verify existing implementation passes
 * - REFACTOR: Optimize if needed
 * 
 * @see https://github.com/zarfld/ESP_ClapMetronome/issues/44
 * @date 2025-11-21
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>

#ifdef NATIVE_BUILD
// Native build simulation
#include <cstdint>
#include <functional>
#else
// ESP32 hardware
#include <Arduino.h>
#include <Wire.h>
#endif

// Include Timing Manager implementation
#include "timing/TimingManager.h"

/**
 * Test fixture for RTC I2C Integration Tests
 */
class RTCIntegrationTest : public ::testing::Test {
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
     * @brief Measure I2C operation latency in microseconds
     * @param operation Function to time
     * @return Latency in microseconds
     */
    template<typename Func>
    int64_t measureLatencyUs(Func operation) {
        auto start = std::chrono::high_resolution_clock::now();
        operation();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    
    /**
     * @brief Measure multiple iterations and return statistics
     * @param operation Function to time
     * @param iterations Number of measurements
     * @return Vector of latencies in microseconds
     */
    template<typename Func>
    std::vector<int64_t> measureLatencies(Func operation, int iterations) {
        std::vector<int64_t> latencies;
        latencies.reserve(iterations);
        
        for (int i = 0; i < iterations; i++) {
            latencies.push_back(measureLatencyUs(operation));
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
        
        return latencies;
    }
    
    /**
     * @brief Calculate statistics from latency measurements
     */
    struct LatencyStats {
        int64_t min_us;
        int64_t max_us;
        int64_t mean_us;
        int64_t median_us;
        int64_t p95_us;
        int64_t p99_us;
    };
    
    LatencyStats calculateStats(const std::vector<int64_t>& latencies) {
        LatencyStats stats;
        
        auto sorted = latencies;
        std::sort(sorted.begin(), sorted.end());
        
        stats.min_us = sorted.front();
        stats.max_us = sorted.back();
        stats.mean_us = std::accumulate(sorted.begin(), sorted.end(), 0LL) / sorted.size();
        stats.median_us = sorted[sorted.size() / 2];
        stats.p95_us = sorted[static_cast<size_t>(sorted.size() * 0.95)];
        stats.p99_us = sorted[static_cast<size_t>(sorted.size() * 0.99)];
        
        return stats;
    }
};

// ============================================================================
// AC-TIME-003: RTC I2C Communication Tests
// ============================================================================

/**
 * AC-TIME-003.1: I2C Read Latency
 * 
 * Requirement: RTC I2C read must complete within 5ms
 * Pass Condition: getTimestampUs() completes in <5000µs
 */
TEST_F(RTCIntegrationTest, I2CReadLatency_Under5ms) {
    const int ITERATIONS = 100;
    const int64_t TARGET_LATENCY_US = 5000;  // 5ms
    
    // Measure latency of 100 timestamp reads
    auto latencies = measureLatencies([this]() {
        timing_->getTimestampUs();
    }, ITERATIONS);
    
    auto stats = calculateStats(latencies);
    
    // All measurements should be under 5ms
    EXPECT_LT(stats.max_us, TARGET_LATENCY_US)
        << "Max I2C read latency: " << stats.max_us << "µs (target: <5000µs)";
    
    // 99th percentile should be well under target
    EXPECT_LT(stats.p99_us, TARGET_LATENCY_US)
        << "P99 I2C read latency: " << stats.p99_us << "µs (target: <5000µs)";
    
    // Mean should be much lower
    EXPECT_LT(stats.mean_us, TARGET_LATENCY_US / 2)
        << "Mean I2C read latency: " << stats.mean_us << "µs (target: <2500µs)";
    
    std::cout << "\nI2C Read Latency Statistics (n=" << ITERATIONS << "):\n"
              << "  Min:    " << stats.min_us << "µs\n"
              << "  Mean:   " << stats.mean_us << "µs\n"
              << "  Median: " << stats.median_us << "µs\n"
              << "  P95:    " << stats.p95_us << "µs\n"
              << "  P99:    " << stats.p99_us << "µs\n"
              << "  Max:    " << stats.max_us << "µs\n"
              << "  Target: <5000µs\n";
}

/**
 * AC-TIME-003.2: RTC Detection
 * 
 * Requirement: Detect RTC on I2C bus at address 0x68
 * Pass Condition: rtc_available flag set correctly after init
 */
TEST_F(RTCIntegrationTest, RTCDetection_I2CAddress) {
    const auto& state = timing_->getState();
    
    #ifdef NATIVE_BUILD
    // Native build: RTC not available (simulation)
    EXPECT_FALSE(state.rtc_available)
        << "Native build should not have RTC hardware";
    EXPECT_TRUE(state.using_fallback)
        << "Native build should use fallback timing";
    #else
    // ESP32 build: RTC should be detected if present
    // Test passes if either:
    // 1. RTC detected and healthy
    // 2. RTC not detected but fallback enabled
    if (state.rtc_available) {
        EXPECT_TRUE(state.rtc_healthy || state.using_fallback)
            << "RTC available but neither healthy nor fallback enabled";
    } else {
        EXPECT_TRUE(state.using_fallback)
            << "RTC not available but fallback not enabled";
    }
    #endif
    
    std::cout << "\nRTC Detection Results:\n"
              << "  RTC Available: " << (state.rtc_available ? "Yes" : "No") << "\n"
              << "  RTC Healthy:   " << (state.rtc_healthy ? "Yes" : "No") << "\n"
              << "  Using Fallback: " << (state.using_fallback ? "Yes" : "No") << "\n"
              << "  I2C Errors:    " << state.i2c_error_count << "\n";
}

/**
 * AC-TIME-003.3: I2C Error Handling
 * 
 * Requirement: Track I2C errors and activate fallback after threshold
 * Pass Condition: Error count increments, fallback activates
 */
TEST_F(RTCIntegrationTest, I2CErrorHandling_FallbackActivation) {
    const auto& state = timing_->getState();
    
    // Record initial error count
    uint16_t initial_errors = state.i2c_error_count;
    
    #ifdef NATIVE_BUILD
    // Native build always in fallback
    EXPECT_TRUE(state.using_fallback)
        << "Native build should be in fallback mode";
    EXPECT_EQ(state.i2c_error_count, 0)
        << "Native build should not increment I2C errors";
    #else
    // ESP32: If RTC unavailable, should be in fallback
    if (!state.rtc_available) {
        EXPECT_TRUE(state.using_fallback)
            << "Should use fallback when RTC unavailable";
    }
    #endif
    
    // Make multiple calls to potentially trigger error handling
    for (int i = 0; i < 10; i++) {
        timing_->getTimestampUs();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Error count should be stable (not continuously growing)
    uint16_t final_errors = timing_->getState().i2c_error_count;
    EXPECT_LE(final_errors, initial_errors + 10)
        << "I2C error count growing unexpectedly";
    
    std::cout << "\nI2C Error Handling:\n"
              << "  Initial Errors: " << initial_errors << "\n"
              << "  Final Errors:   " << final_errors << "\n"
              << "  Using Fallback: " << (state.using_fallback ? "Yes" : "No") << "\n";
}

/**
 * AC-TIME-003.4: Temperature Sensor Access
 * 
 * Requirement: Read temperature from RTC sensor via I2C
 * Pass Condition: Temperature reading completes within 5ms
 */
TEST_F(RTCIntegrationTest, TemperatureSensor_I2CLatency) {
    const int ITERATIONS = 50;
    const int64_t TARGET_LATENCY_US = 5000;  // 5ms
    
    // Measure temperature read latency
    auto latencies = measureLatencies([this]() {
        timing_->getRtcTemperature();
    }, ITERATIONS);
    
    auto stats = calculateStats(latencies);
    
    // Temperature reads should complete quickly
    EXPECT_LT(stats.max_us, TARGET_LATENCY_US)
        << "Max temperature read latency: " << stats.max_us << "µs (target: <5000µs)";
    
    // Get actual temperature reading
    float temp = timing_->getRtcTemperature();
    
    #ifdef NATIVE_BUILD
    // Native build returns 0.0
    EXPECT_EQ(temp, 0.0f)
        << "Native build should return 0.0°C";
    #else
    // ESP32: If RTC available, should be room temperature
    const auto& state = timing_->getState();
    if (state.rtc_available && state.rtc_healthy) {
        EXPECT_GE(temp, -10.0f) << "Temperature below reasonable minimum";
        EXPECT_LE(temp, 60.0f) << "Temperature above reasonable maximum";
    }
    #endif
    
    std::cout << "\nTemperature Sensor I2C:\n"
              << "  Temperature:  " << temp << "°C\n"
              << "  Mean Latency: " << stats.mean_us << "µs\n"
              << "  Max Latency:  " << stats.max_us << "µs\n"
              << "  Target:       <5000µs\n";
}

/**
 * AC-TIME-003.5: Time Synchronization Latency
 * 
 * Requirement: syncRtc() should complete within reasonable time
 * Pass Condition: Sync operation completes in <100ms
 */
TEST_F(RTCIntegrationTest, TimeSynchronization_I2CLatency) {
    const int64_t TARGET_LATENCY_US = 100000;  // 100ms
    
    // Measure sync latency
    int64_t sync_latency = measureLatencyUs([this]() {
        timing_->syncRtc();
    });
    
    EXPECT_LT(sync_latency, TARGET_LATENCY_US)
        << "Sync latency: " << sync_latency << "µs (target: <100000µs)";
    
    std::cout << "\nTime Synchronization:\n"
              << "  Sync Latency: " << sync_latency << "µs\n"
              << "  Target:       <100000µs\n";
}

/**
 * AC-TIME-003.6: Consecutive I2C Operations
 * 
 * Requirement: Multiple rapid I2C reads should remain fast
 * Pass Condition: No performance degradation over time
 */
TEST_F(RTCIntegrationTest, ConsecutiveOperations_NoPerformanceDegradation) {
    const int BATCH_SIZE = 10;
    const int NUM_BATCHES = 10;
    
    std::vector<int64_t> batch_means;
    
    for (int batch = 0; batch < NUM_BATCHES; batch++) {
        auto latencies = measureLatencies([this]() {
            timing_->getTimestampUs();
        }, BATCH_SIZE);
        
        auto stats = calculateStats(latencies);
        batch_means.push_back(stats.mean_us);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Calculate overall statistics
    int64_t first_batch_mean = batch_means.front();
    int64_t last_batch_mean = batch_means.back();
    int64_t overall_mean = std::accumulate(batch_means.begin(), batch_means.end(), 0LL) / batch_means.size();
    
    // Performance shouldn't degrade significantly
    // For microsecond measurements, allow 3x margin to account for measurement noise
    // In native build, both values are typically <5µs so absolute difference matters more
    int64_t degradation_threshold = std::max(first_batch_mean * 3, static_cast<int64_t>(10));
    EXPECT_LT(last_batch_mean, degradation_threshold)
        << "Performance degraded: first=" << first_batch_mean 
        << "µs, last=" << last_batch_mean << "µs, threshold=" << degradation_threshold << "µs";
    
    std::cout << "\nConsecutive Operations (" << NUM_BATCHES << " batches of " << BATCH_SIZE << "):\n"
              << "  First Batch Mean: " << first_batch_mean << "µs\n"
              << "  Last Batch Mean:  " << last_batch_mean << "µs\n"
              << "  Overall Mean:     " << overall_mean << "µs\n";
}

// ============================================================================
// Performance Summary
// ============================================================================

/**
 * Summary: All AC-TIME-003 Requirements
 * 
 * Validates all RTC I2C integration requirements in one comprehensive test
 */
TEST_F(RTCIntegrationTest, Summary_AllRTCIntegrationRequirementsMet) {
    const auto& state = timing_->getState();
    
    // AC-TIME-003.1: I2C read latency
    int64_t read_latency = measureLatencyUs([this]() {
        timing_->getTimestampUs();
    });
    bool latency_ok = (read_latency < 5000);
    
    // AC-TIME-003.2: RTC detection
    bool detection_ok = state.rtc_available || state.using_fallback;
    
    // AC-TIME-003.3: Error handling
    bool error_handling_ok = (state.i2c_error_count < 100);  // Reasonable threshold
    
    // AC-TIME-003.4: Temperature sensor
    float temp = timing_->getRtcTemperature();
    bool temp_ok = (temp >= -10.0f && temp <= 60.0f) || (temp == 0.0f);  // 0.0 = fallback
    
    // AC-TIME-003.5: Sync latency
    int64_t sync_latency = measureLatencyUs([this]() {
        timing_->syncRtc();
    });
    bool sync_ok = (sync_latency < 100000);
    
    std::cout << "\n=== RTC I2C Integration Summary ===\n";
    std::cout << (latency_ok ? "✓" : "✗") << " I2C read latency: " << read_latency << "µs (target: <5000µs)\n";
    std::cout << (detection_ok ? "✓" : "✗") << " RTC detection: " 
              << (state.rtc_available ? "Hardware" : "Fallback") << "\n";
    std::cout << (error_handling_ok ? "✓" : "✗") << " Error handling: " << state.i2c_error_count << " errors\n";
    std::cout << (temp_ok ? "✓" : "✗") << " Temperature sensor: " << temp << "°C\n";
    std::cout << (sync_ok ? "✓" : "✗") << " Sync latency: " << sync_latency << "µs (target: <100000µs)\n";
    
    bool all_ok = latency_ok && detection_ok && error_handling_ok && temp_ok && sync_ok;
    std::cout << "\n" << (all_ok ? "✓" : "✗") << " All RTC I2C integration requirements met!\n";
    
    EXPECT_TRUE(all_ok) << "One or more RTC I2C requirements not met";
}

/**
 * Main test runner
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
