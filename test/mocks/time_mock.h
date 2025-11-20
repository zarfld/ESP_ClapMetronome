/**
 * @file time_mock.h
 * @brief Time function mocks for native unit testing
 * 
 * Provides Arduino-compatible time functions (micros(), millis())
 * for native builds (Windows/Linux/Mac).
 */

#ifndef TIME_MOCK_H
#define TIME_MOCK_H

#include <stdint.h>
#include <chrono>

#ifdef NATIVE_BUILD

// Mock time state - use inline to ensure single definition across translation units
namespace TimeMock {
    inline uint64_t& get_mock_micros() {
        static uint64_t mock_micros_value = 0;
        return mock_micros_value;
    }
    
    inline bool& get_use_real_time() {
        static bool use_real_time = false;
        return use_real_time;
    }
}

/**
 * @brief Get microseconds since program start (Arduino-compatible)
 * 
 * @return Microseconds (µs)
 */
inline uint64_t micros() {
    if (TimeMock::get_use_real_time()) {
        static auto start = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - start).count();
    }
    return TimeMock::get_mock_micros();
}

/**
 * @brief Get milliseconds since program start (Arduino-compatible)
 * 
 * @return Milliseconds (ms)
 */
inline uint32_t millis() {
    return static_cast<uint32_t>(micros() / 1000);
}

/**
 * @brief Set mock micros value (for testing)
 * 
 * @param value New micros value
 */
inline void set_mock_micros(uint64_t value) {
    TimeMock::get_mock_micros() = value;
}

/**
 * @brief Advance mock time by microseconds
 * 
 * @param us Microseconds to advance
 */
inline void advance_time_us(uint64_t us) {
    TimeMock::get_mock_micros() += us;
}

/**
 * @brief Advance mock time by milliseconds
 * 
 * @param ms Milliseconds to advance
 */
inline void advance_time_ms(uint32_t ms) {
    TimeMock::get_mock_micros() += static_cast<uint64_t>(ms) * 1000;
}

/**
 * @brief Reset mock time to zero
 */
inline void reset_mock_time() {
    TimeMock::get_mock_micros() = 0;
}

/**
 * @brief Enable/disable real-time mode
 * 
 * @param enable true to use real system time, false to use mock
 */
inline void use_real_time(bool enable) {
    TimeMock::get_use_real_time() = enable;
}

#else
// For ESP32 builds, use Arduino's time functions
// (already defined in Arduino.h)
#endif

#endif // TIME_MOCK_H
