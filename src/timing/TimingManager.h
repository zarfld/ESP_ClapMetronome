/**
 * @file TimingManager.h
 * @brief High-precision timing service with RTC3231 and fallback
 * 
 * Implements: DES-C-005 (Timing Manager component)
 * GitHub Issue: #49
 * Requirement: REQ-F-007 (RTC3231 I2C timing with fallback)
 * Architecture: ADR-TIME-001 (RTC3231 I2C with fallback)
 * 
 * Standard: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * 
 * TDD Status:
 * - RED: Tests written ✅
 * - GREEN: Implementation complete ✅
 * - REFACTOR: Complete ✅
 */

#ifndef TIMING_MANAGER_H
#define TIMING_MANAGER_H

#include "../interfaces/ITimingProvider.h"
#include "TimingManagerState.h"

/**
 * @class TimingManager
 * @brief Provides monotonic microsecond timestamps with automatic RTC fallback
 * 
 * Features:
 * - RTC3231 I2C communication with health monitoring
 * - Automatic fallback to ESP32 micros() on RTC failure
 * - NTP synchronization support
 * - Temperature-compensated timing
 * 
 * Memory footprint: ~120 bytes (40B state + 80B overhead)
 * CPU usage: <1% (I2C polling 1Hz)
 */
class TimingManager : public ITimingProvider {
public:
    /**
     * @brief Constructor
     */
    TimingManager();
    
    /**
     * @brief Destructor
     */
    virtual ~TimingManager() = default;
    
    /**
     * @brief Initialize timing subsystem
     * 
     * Acceptance Criteria:
     * - AC-TIME-008: <100ms to first timestamp
     * 
     * Actions:
     * 1. Detect RTC on I2C bus (address 0x68)
     * 2. Initialize fallback if RTC absent
     * 3. Start health monitoring timer
     * 
     * @return bool True if initialization successful
     */
    bool init();
    
    // ITimingProvider interface implementation
    
    /**
     * @brief Get current timestamp in microseconds
     * 
     * Implementation:
     * - If RTC healthy: Read RTC time + convert to µs
     * - If RTC unhealthy: Use micros() fallback
     * 
     * @return uint64_t Monotonic timestamp in microseconds
     */
    uint64_t getTimestampUs() override;
    
    /**
     * @brief Get current timestamp in milliseconds
     * 
     * @return uint32_t Timestamp in milliseconds
     */
    uint32_t getTimestampMs() override;
    
    /**
     * @brief Check if RTC is healthy
     * 
     * @return bool True if RTC responding, false if using fallback
     */
    bool rtcHealthy() override;
    
    /**
     * @brief Get RTC temperature
     * 
     * @return float Temperature in Celsius (0.0 if unavailable)
     */
    float getRtcTemperature() override;
    
    /**
     * @brief Synchronize RTC with NTP
     * 
     * @return bool True if sync successful
     */
    bool syncRtc() override;
    
    /**
     * @brief Get internal state (for testing/debugging)
     * 
     * @return const TimingManagerState& Read-only state reference
     */
    const TimingManagerState& getState() const { return state_; }
    
private:
    // Constants
    static constexpr uint8_t RTC_I2C_ADDRESS = 0x68;        ///< RTC3231 I2C address
    static constexpr uint16_t RTC_ERROR_THRESHOLD = 10;     ///< Max I2C errors before fallback
    static constexpr uint64_t JITTER_WINDOW_US = 1000000;   ///< Only track jitter for deltas < 1s
    
    TimingManagerState state_;    ///< Internal state tracking
    
    // RTC Management
    
    /**
     * @brief Detect RTC on I2C bus
     * 
     * @return bool True if RTC found at address 0x68
     */
    bool detectRTC();
    
    /**
     * @brief Initialize RTC subsystem after detection
     * 
     * @return bool True if initialization successful
     */
    bool initRTC();
    
    /**
     * @brief Read timestamp from RTC3231
     * 
     * @return uint64_t RTC timestamp in microseconds
     */
    uint64_t readRTCTimestampUs();
    
    /**
     * @brief Update RTC health status based on I2C errors
     */
    void updateRTCHealth();
    
    /**
     * @brief Switch to fallback timing (micros())
     */
    void enableFallback();
    
    // Timestamp Management
    
    /**
     * @brief Get raw platform timestamp without monotonicity guarantee
     * 
     * @return uint64_t Raw timestamp in microseconds
     */
    uint64_t getRawTimestampUs();
    
    /**
     * @brief Ensure timestamp monotonicity
     * 
     * @param current_timestamp Current raw timestamp
     * @return uint64_t Corrected monotonic timestamp
     */
    uint64_t ensureMonotonicity(uint64_t current_timestamp);
    
    /**
     * @brief Update jitter measurement
     * 
     * @param current_timestamp Current timestamp
     */
    void updateJitter(uint64_t current_timestamp);
};

#endif // TIMING_MANAGER_H
