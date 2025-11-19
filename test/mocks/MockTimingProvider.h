/**
 * MockTimingProvider - Test Double for ITimingProvider
 * 
 * Mock implementation of DES-I-001/002/003 (Timing Manager interfaces)
 * for unit testing components that depend on timing services.
 * 
 * From: Phase 05 TDD Plan, Section 3.2 (Mock Specifications)
 * Issue: #51 (TDD Plan)
 * Used By: Audio Detection, BPM Calculation, Output Controller tests
 * 
 * Standards: IEEE 1012-2016 (V&V - Test Doubles)
 * 
 * Usage:
 *   MockTimingProvider mock;
 *   mock.setTimestamp(1000000);  // 1 second
 *   Audio Detection audio(&mock);
 *   // Test uses mock timestamp
 */

#ifndef MOCK_TIMING_PROVIDER_H
#define MOCK_TIMING_PROVIDER_H

#include "../../src/interfaces/ITimingProvider.h"

namespace clap_metronome {

/**
 * Mock Timing Provider for Testing
 * 
 * Provides controllable timestamps for deterministic tests
 */
class MockTimingProvider : public ITimingProvider {
public:
    MockTimingProvider()
        : current_timestamp_us_(0)
        , rtc_healthy_(true) {
    }
    
    virtual ~MockTimingProvider() = default;
    
    // ===== ITimingProvider Interface =====
    
    /**
     * Get current timestamp (microseconds)
     * Returns value set by setTimestamp()
     */
    uint64_t getTimestampUs() override {
        return current_timestamp_us_;
    }
    
    /**
     * Get current timestamp (milliseconds)
     * Convenience method for millisecond precision
     */
    uint32_t getTimestampMs() override {
        return static_cast<uint32_t>(current_timestamp_us_ / 1000);
    }
    
    /**
     * Check RTC health status
     * Returns value set by setRtcHealthy()
     */
    bool rtcHealthy() override {
        return rtc_healthy_;
    }
    
    /**
     * Get RTC temperature (always 25°C in mock)
     */
    float getRtcTemperature() override {
        return 25.0f;  // Room temperature for testing
    }
    
    /**
     * Synchronize RTC (no-op in mock)
     */
    bool syncRtc() override {
        return true;  // Always succeed in mock
    }
    
    // ===== Test Control Methods =====
    
    /**
     * Set timestamp to return from getTimestampUs()
     * 
     * @param timestamp_us Microseconds since epoch
     * 
     * Example:
     *   mock.setTimestamp(1000000);  // 1 second
     *   mock.setTimestamp(1500000);  // 1.5 seconds
     */
    void setTimestamp(uint64_t timestamp_us) {
        current_timestamp_us_ = timestamp_us;
    }
    
    /**
     * Advance timestamp by delta
     * Useful for simulating time progression
     * 
     * @param delta_us Microseconds to advance
     * 
     * Example:
     *   mock.setTimestamp(1000000);  // Start at 1s
     *   mock.advanceTime(500000);    // Now at 1.5s
     */
    void advanceTime(uint64_t delta_us) {
        current_timestamp_us_ += delta_us;
    }
    
    /**
     * Set RTC health status
     * 
     * @param healthy true if RTC should report healthy
     */
    void setRtcHealthy(bool healthy) {
        rtc_healthy_ = healthy;
    }
    
    /**
     * Reset mock to initial state
     */
    void reset() {
        current_timestamp_us_ = 0;
        rtc_healthy_ = true;
    }

private:
    uint64_t current_timestamp_us_;  ///< Current mock timestamp
    bool     rtc_healthy_;           ///< Mock RTC health status
};

} // namespace clap_metronome

#endif // MOCK_TIMING_PROVIDER_H
