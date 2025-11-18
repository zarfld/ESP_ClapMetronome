/**
 * @file TimingManagerState.h
 * @brief Data model for Timing Manager internal state
 * 
 * Implements: DES-D-006 (TimingManagerState data model)
 * GitHub Issue: #70
 * Requirement: REQ-F-007 (RTC3231 I2C timing with fallback)
 * 
 * Standard: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 */

#ifndef TIMING_MANAGER_STATE_H
#define TIMING_MANAGER_STATE_H

#include <stdint.h>

/**
 * @struct TimingManagerState
 * @brief Internal state tracking for timing subsystem
 * 
 * Tracks RTC health, fallback status, and synchronization state.
 * Memory footprint: ~40 bytes
 */
struct TimingManagerState {
    // RTC Health Status (DES-I-002)
    bool rtc_available;           ///< RTC detected on I2C bus
    bool rtc_healthy;             ///< RTC responding correctly
    uint16_t i2c_error_count;     ///< Consecutive I2C failures
    float rtc_temperature;        ///< RTC temperature sensor reading (°C)
    
    // Fallback Status
    bool using_fallback;          ///< Currently using millis() instead of RTC
    uint64_t fallback_start_us;   ///< Timestamp when fallback started
    
    // Time Synchronization (DES-I-003)
    bool ntp_synced;              ///< Successfully synced with NTP
    uint64_t last_sync_time;      ///< Last successful NTP sync (µs)
    uint32_t sync_failure_count;  ///< Failed sync attempts
    
    // Timing Quality Metrics
    uint32_t jitter_us;           ///< Measured timestamp jitter (µs)
    uint64_t last_timestamp;      ///< Previous timestamp for monotonicity check
    
    /**
     * @brief Initialize state to factory defaults
     */
    void init() {
        rtc_available = false;
        rtc_healthy = false;
        i2c_error_count = 0;
        rtc_temperature = 0.0f;
        
        using_fallback = true;  // Start in fallback until RTC detected
        fallback_start_us = 0;
        
        ntp_synced = false;
        last_sync_time = 0;
        sync_failure_count = 0;
        
        jitter_us = 0;
        last_timestamp = 0;
    }
};

#endif // TIMING_MANAGER_STATE_H
