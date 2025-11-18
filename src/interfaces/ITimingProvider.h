/**
 * @file ITimingProvider.h
 * @brief Interface for timestamp and timing services
 * 
 * Implements: DES-I-001 (Timestamp Query Interface)
 *             DES-I-002 (RTC Health Status Interface)
 *             DES-I-003 (Time Synchronization Interface)
 * 
 * GitHub Issues: #52 (DES-I-001), #56 (DES-I-002), #57 (DES-I-003)
 * Requirement: REQ-F-007 (RTC3231 I2C timing with fallback)
 * 
 * Standard: ISO/IEC/IEEE 29148:2018 (Interface Specifications)
 */

#ifndef I_TIMING_PROVIDER_H
#define I_TIMING_PROVIDER_H

#include <stdint.h>

/**
 * @interface ITimingProvider
 * @brief Abstract interface for high-precision timestamp services
 * 
 * Provides monotonic microsecond timestamps with automatic RTC/fallback management.
 * All components requiring timing services depend on this interface.
 */
class ITimingProvider {
public:
    virtual ~ITimingProvider() = default;
    
    // DES-I-001: Timestamp Query Interface
    
    /**
     * @brief Get current timestamp in microseconds
     * 
     * Acceptance Criteria:
     * - AC-TIME-001: Monotonicity guarantee (always increasing)
     * - AC-TIME-002: Microsecond precision (<10µs successive call difference)
     * 
     * @return uint64_t Monotonic timestamp in microseconds since boot
     * 
     * Performance:
     * - Execution time: <5µs
     * - Jitter: <10µs
     */
    virtual uint64_t getTimestampUs() = 0;
    
    /**
     * @brief Get current timestamp in milliseconds
     * 
     * Convenience method for millisecond-precision use cases.
     * 
     * @return uint32_t Timestamp in milliseconds since boot
     * 
     * Performance:
     * - Execution time: <2µs
     */
    virtual uint32_t getTimestampMs() = 0;
    
    // DES-I-002: RTC Health Status Interface
    
    /**
     * @brief Check if RTC is available and healthy
     * 
     * Acceptance Criteria:
     * - AC-TIME-005: Returns false after 3 consecutive I2C failures
     * - AC-TIME-004: Automatic fallback when unhealthy
     * 
     * @return bool True if RTC responding correctly, false if using fallback
     * 
     * Performance:
     * - Execution time: <1µs (cached state)
     */
    virtual bool rtcHealthy() = 0;
    
    /**
     * @brief Get RTC temperature sensor reading
     * 
     * Acceptance Criteria:
     * - AC-TIME-006: Returns 20-30°C in typical room temperature
     * 
     * @return float Temperature in Celsius (0.0 if RTC unavailable)
     * 
     * Performance:
     * - Execution time: <100µs (I2C read)
     */
    virtual float getRtcTemperature() = 0;
    
    // DES-I-003: Time Synchronization Interface
    
    /**
     * @brief Synchronize RTC with NTP server
     * 
     * Acceptance Criteria:
     * - AC-TIME-007: Sets RTC time accurately from NTP
     * 
     * @return bool True if sync successful, false otherwise
     * 
     * Performance:
     * - Execution time: <1000ms (network dependent)
     * - Side effects: Updates RTC time register
     */
    virtual bool syncRtc() = 0;
};

#endif // I_TIMING_PROVIDER_H
