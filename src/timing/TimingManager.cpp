/**
 * @file TimingManager.cpp
 * @brief Implementation of high-precision timing service
 * 
 * Implements: DES-C-005 (Timing Manager component)
 * GitHub Issue: #49
 * Requirement: REQ-F-007 (RTC3231 I2C timing with fallback)
 * 
 * TDD Status:
 * - RED: Tests written ✅
 * - GREEN: Minimal implementation (current phase)
 * - REFACTOR: Pending
 * 
 * Standard: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 */

#include "TimingManager.h"

// Platform-specific includes
#ifdef NATIVE_BUILD
    // Native test environment - use system time
    #include <chrono>
    #include <thread>
#else
    // ESP32/ESP8266 environment
    #include <Arduino.h>
    #include <Wire.h>
#endif

//==============================================================================
// Constructor / Destructor
//==============================================================================

TimingManager::TimingManager() {
    state_.init();
}

//==============================================================================
// Initialization
//==============================================================================

bool TimingManager::init() {
    // Initialize state
    state_.init();
    
    #ifndef NATIVE_BUILD
    // ESP32/ESP8266: Attempt RTC detection
    if (detectRTC()) {
        state_.rtc_available = true;
        state_.rtc_healthy = true;
        state_.using_fallback = false;
    } else {
        // No RTC detected, use fallback
        enableFallback();
    }
    #else
    // Native test: Always use fallback (no real RTC)
    enableFallback();
    #endif
    
    return true;
}

//==============================================================================
// DES-I-001: Timestamp Query Interface (TDD Cycle 1 - GREEN)
//==============================================================================

/**
 * GREEN Phase: Minimal implementation to pass AC-TIME-001, AC-TIME-002
 * 
 * Strategy:
 * - Use platform-specific microsecond counter
 * - Ensure monotonicity by comparing with last_timestamp
 * - Handle potential rollover (though uint64_t won't rollover for 584,000 years)
 */
uint64_t TimingManager::getTimestampUs() {
    uint64_t current_timestamp;
    
    #ifdef NATIVE_BUILD
    // Native test: Use std::chrono high-resolution clock
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    current_timestamp = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    #else
    // ESP32/ESP8266: Use micros() built-in function
    if (state_.using_fallback || !state_.rtc_healthy) {
        // Fallback: ESP32 micros() function
        current_timestamp = micros();
    } else {
        // RTC path: Read from RTC3231
        current_timestamp = readRTCTimestampUs();
    }
    #endif
    
    // Monotonicity guarantee (AC-TIME-001)
    if (current_timestamp < state_.last_timestamp) {
        // Rollover or clock adjustment - use last_timestamp + 1
        current_timestamp = state_.last_timestamp + 1;
    }
    
    // Update jitter measurement
    if (state_.last_timestamp > 0) {
        uint64_t delta = current_timestamp - state_.last_timestamp;
        if (delta < 1000000) {  // Only track if <1s delta
            state_.jitter_us = static_cast<uint32_t>(delta);
        }
    }
    
    state_.last_timestamp = current_timestamp;
    return current_timestamp;
}

/**
 * GREEN Phase: Millisecond conversion
 * 
 * Simply divides microsecond timestamp by 1000
 */
uint32_t TimingManager::getTimestampMs() {
    return static_cast<uint32_t>(getTimestampUs() / 1000);
}

//==============================================================================
// DES-I-002: RTC Health Status Interface (TDD Cycle 2 - Stub)
//==============================================================================

bool TimingManager::rtcHealthy() {
    return state_.rtc_healthy;
}

float TimingManager::getRtcTemperature() {
    #ifndef NATIVE_BUILD
    if (state_.rtc_available && state_.rtc_healthy) {
        // TODO: Implement RTC temperature reading via I2C
        // For now, return cached value
        return state_.rtc_temperature;
    }
    #endif
    return 0.0f;
}

//==============================================================================
// DES-I-003: Time Synchronization Interface (TDD Cycle 3 - Stub)
//==============================================================================

bool TimingManager::syncRtc() {
    #ifndef NATIVE_BUILD
    if (state_.rtc_available) {
        // TODO: Implement NTP sync logic
        // For now, return success if RTC available
        state_.ntp_synced = true;
        state_.last_sync_time = getTimestampUs();
        return true;
    }
    #endif
    return false;
}

//==============================================================================
// Private Helper Methods
//==============================================================================

bool TimingManager::detectRTC() {
    #ifndef NATIVE_BUILD
    // Try to communicate with RTC3231 at I2C address 0x68
    Wire.begin();
    Wire.beginTransmission(0x68);
    uint8_t error = Wire.endTransmission();
    
    return (error == 0);  // 0 = success
    #else
    return false;  // Native test has no real RTC
    #endif
}

uint64_t TimingManager::readRTCTimestampUs() {
    #ifndef NATIVE_BUILD
    // TODO: Implement RTC3231 I2C read
    // For now, fall back to micros()
    return micros();
    #else
    return 0;
    #endif
}

void TimingManager::updateRTCHealth() {
    // Check if we've exceeded error threshold (10 errors)
    if (state_.i2c_error_count >= 10) {
        state_.rtc_healthy = false;
        enableFallback();
    }
}

void TimingManager::enableFallback() {
    state_.using_fallback = true;
    state_.rtc_healthy = false;
    state_.fallback_start_us = getTimestampUs();
}
