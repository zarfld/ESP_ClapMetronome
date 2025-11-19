/**
 * DES-D-002: Audio Detection State Data Model
 * 
 * State machine and metrics for audio beat detection.
 * Implements rising edge detection (AC-AUDIO-002) and AGC (AC-AUDIO-003).
 * 
 * Implements: #45 (DES-C-001 Audio Detection Engine)
 * Data Model: DES-D-002 (Audio Detection State)
 * Requirement: REQ-F-001 (Beat detection state machine)
 * Standards: IEEE 1016-2009 (Data Design), ISO/IEC/IEEE 12207:2017
 * 
 * TDD Status: RED ⏳ (Data model definition)
 * 
 * Memory Budget: ~160 bytes (AC-AUDIO-011)
 * 
 * See: https://github.com/zarfld/ESP_ClapMetronome/issues/45
 */

#ifndef AUDIO_DETECTION_STATE_H
#define AUDIO_DETECTION_STATE_H

#include <cstdint>
#include <cstring>  // for memset

namespace clap_metronome {

/**
 * Detection State Machine (AC-AUDIO-002)
 * 
 * State transitions:
 * IDLE → RISING → TRIGGERED → DEBOUNCE → IDLE
 */
enum class DetectionState : uint8_t {
    IDLE = 0,        ///< No beat detected, monitoring for threshold crossing
    RISING = 1,      ///< Signal crossed threshold, measuring rise time
    TRIGGERED = 2,   ///< Beat confirmed, event emitted
    DEBOUNCE = 3     ///< Ignoring signals for 50ms after beat (AC-AUDIO-005)
};

/**
 * AGC (Automatic Gain Control) Level (AC-AUDIO-003)
 * 
 * MAX9814 gain settings:
 * - GAIN_40DB: Low gain, prevents clipping in loud environments
 * - GAIN_50DB: Medium gain, balanced sensitivity
 * - GAIN_60DB: High gain, maximum sensitivity for quiet environments
 */
enum class AGCLevel : uint8_t {
    GAIN_40DB = 0,  ///< 40dB gain (lowest sensitivity, clipping prevention)
    GAIN_50DB = 1,  ///< 50dB gain (medium sensitivity)
    GAIN_60DB = 2   ///< 60dB gain (highest sensitivity)
};

/**
 * Audio Detection State
 * 
 * Maintains all state for beat detection algorithm.
 * Size: ~160 bytes total
 */
struct AudioDetectionState {
    // State Machine (AC-AUDIO-002)
    DetectionState state;               ///< Current detection state
    
    // Adaptive Threshold (AC-AUDIO-001)
    uint16_t threshold;                 ///< Current adaptive threshold (ADC units)
    uint16_t min_value;                 ///< Minimum in current window
    uint16_t max_value;                 ///< Maximum in current window
    
    // AGC State (AC-AUDIO-003, AC-AUDIO-012)
    AGCLevel gain_level;                ///< Current MAX9814 gain level
    bool     clipping_detected;         ///< True if recent sample >4000 (clipping)
    
    // Beat Detection Metrics
    uint32_t beat_count;                ///< Total beats detected since boot
    uint32_t false_positive_count;      ///< Noise rejections below threshold
    uint64_t last_beat_timestamp_us;    ///< Timestamp of most recent beat (for debounce)
    
    // Rise Time Detection (AC-AUDIO-006 kick-only filtering)
    uint64_t rising_edge_start_us;      ///< Timestamp when signal crossed threshold
    uint16_t rising_edge_start_value;   ///< ADC value at start of rise
    uint16_t rising_edge_peak_value;    ///< Peak ADC value during rise
    
    // Telemetry Timing (AC-AUDIO-007)
    uint64_t last_telemetry_us;         ///< Timestamp of last telemetry publish
    
    // Window tracking (for adaptive threshold calculation)
    static constexpr size_t WINDOW_SIZE = 64;  ///< Samples for min/max tracking (from legacy code)
    uint16_t window_samples[WINDOW_SIZE];      ///< Rolling window of recent samples
    uint8_t  window_index;                     ///< Current write position in window
    
    // Constants for detection algorithm
    static constexpr uint16_t CLIPPING_THRESHOLD = 4000;  ///< ADC value indicating clipping (12-bit max = 4095)
    static constexpr uint64_t DEBOUNCE_PERIOD_US = 50000; ///< 50ms debounce (AC-AUDIO-005)
    static constexpr uint64_t TELEMETRY_INTERVAL_US = 500000; ///< 500ms telemetry (AC-AUDIO-007)
    static constexpr uint64_t KICK_RISE_TIME_US = 4000;   ///< 4ms rise time for kick detection (AC-AUDIO-006)
    static constexpr float    THRESHOLD_FACTOR = 0.8f;    ///< Adaptive threshold factor (AC-AUDIO-001)
    
    /**
     * Initialize state to defaults
     */
    void init() {
        state = DetectionState::IDLE;
        threshold = 50;  // Initial threshold from legacy code
        min_value = 0;
        max_value = 4095;
        gain_level = AGCLevel::GAIN_50DB;  // Start with medium gain
        clipping_detected = false;
        beat_count = 0;
        false_positive_count = 0;
        last_beat_timestamp_us = 0;
        rising_edge_start_us = 0;
        rising_edge_start_value = 0;
        rising_edge_peak_value = 0;
        last_telemetry_us = 0;
        window_index = 0;
        // Initialize window with midpoint value to avoid zero-induced threshold issues
        // With zeros, first samples cause incorrect min/max calculations
        // Using 2000 (midpoint of 0-4095 12-bit ADC range) provides stable baseline
        for (size_t i = 0; i < WINDOW_SIZE; i++) {
            window_samples[i] = 2000;
        }
    }
    
    /**
     * Update adaptive threshold (AC-AUDIO-001)
     * Formula: threshold = 0.8 × (max - min) + min
     */
    void updateThreshold() {
        if (max_value > min_value) {
            float range = static_cast<float>(max_value - min_value);
            threshold = static_cast<uint16_t>(THRESHOLD_FACTOR * range + min_value);
        }
    }
    
    /**
     * Add sample to rolling window for min/max tracking
     * 
     * @param sample ADC reading to add
     */
    void addToWindow(uint16_t sample) {
        window_samples[window_index] = sample;
        window_index = (window_index + 1) % WINDOW_SIZE;
        
        // Recalculate min/max from window
        min_value = window_samples[0];
        max_value = window_samples[0];
        for (size_t i = 1; i < WINDOW_SIZE; i++) {
            if (window_samples[i] < min_value) min_value = window_samples[i];
            if (window_samples[i] > max_value) max_value = window_samples[i];
        }
        
        updateThreshold();
    }
    
    /**
     * Check if in debounce period (AC-AUDIO-005)
     * 
     * @param current_timestamp_us Current time
     * @return true if still in 50ms debounce period
     */
    bool inDebouncePeriod(uint64_t current_timestamp_us) const {
        if (last_beat_timestamp_us == 0) return false;
        return (current_timestamp_us - last_beat_timestamp_us) < DEBOUNCE_PERIOD_US;
    }
    
    /**
     * Check if telemetry should be published (AC-AUDIO-007)
     * 
     * @param current_timestamp_us Current time
     * @return true if 500ms elapsed since last telemetry
     */
    bool shouldPublishTelemetry(uint64_t current_timestamp_us) const {
        if (last_telemetry_us == 0) return true;
        return (current_timestamp_us - last_telemetry_us) >= TELEMETRY_INTERVAL_US;
    }
};

} // namespace clap_metronome

#endif // AUDIO_DETECTION_STATE_H
