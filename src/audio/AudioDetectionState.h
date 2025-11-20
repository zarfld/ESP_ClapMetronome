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
 * IDLE → RISING_EDGE → TRIGGERED → DEBOUNCE → IDLE
 * 
 * Note: RISING renamed to RISING_EDGE to avoid conflict with Arduino.h RISING macro
 */
enum class DetectionState : uint8_t {
    IDLE = 0,        ///< No beat detected, monitoring for threshold crossing
    RISING_EDGE = 1, ///< Signal crossed threshold, measuring rise time
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
    static constexpr size_t WINDOW_SIZE = 100;  ///< Samples for min/max tracking (AC-AUDIO-001)
    uint16_t window_samples[WINDOW_SIZE];       ///< Rolling window of recent samples
    uint8_t  window_index;                      ///< Current write position in window
    uint8_t  window_count;                      ///< Number of valid samples in window (0-100)
    uint16_t noise_floor;                       ///< Estimated noise floor (for false positive rejection)
    uint8_t  samples_since_noise_update;        ///< Counter for noise floor recalculation
    static constexpr uint8_t NOISE_UPDATE_INTERVAL = 16; ///< Recalculate noise floor every N samples
    
    // Constants for detection algorithm
    static constexpr uint16_t CLIPPING_THRESHOLD = 4000;  ///< ADC value indicating clipping (12-bit max = 4095)
    static constexpr uint64_t DEBOUNCE_PERIOD_US = 50000; ///< 50ms debounce (AC-AUDIO-005)
    static constexpr uint64_t TELEMETRY_INTERVAL_US = 500000; ///< 500ms telemetry (AC-AUDIO-007)
    static constexpr uint64_t KICK_RISE_TIME_US = 4000;   ///< 4ms rise time for kick detection (AC-AUDIO-006)
    static constexpr float    THRESHOLD_FACTOR = 0.8f;    ///< Adaptive threshold factor (AC-AUDIO-001)
    static constexpr uint16_t THRESHOLD_MARGIN = 80;      ///< Hysteresis margin above threshold (AC-AUDIO-009)
    static constexpr uint16_t MIN_SIGNAL_AMPLITUDE = 200; ///< Minimum signal amplitude for valid beat (AC-AUDIO-009)
    
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
        window_count = 0;  // No samples yet
        noise_floor = 100;  // Initial noise floor estimate
        samples_since_noise_update = 0;
        // Initialize window with zeros for clean initial state
        // Min/max will be calculated from actual samples as they arrive
        for (size_t i = 0; i < WINDOW_SIZE; i++) {
            window_samples[i] = 0;
        }
    }
    
    /**
     * Update adaptive threshold (AC-AUDIO-001)
     * Formula: threshold = 0.8 × (max - min) + min
     * 
     * Note: Enhanced with AC-AUDIO-009 false positive rejection in future cycles
     */
    void updateThreshold() {
        // Calculate adaptive threshold (works even when max == min)
        float range = static_cast<float>(max_value - min_value);
        threshold = static_cast<uint16_t>(THRESHOLD_FACTOR * range + min_value);
        
        // Update noise floor periodically for AC-AUDIO-009 (future use)
        samples_since_noise_update++;
        if (samples_since_noise_update >= NOISE_UPDATE_INTERVAL) {
            noise_floor = calculateNoiseFloor();
            samples_since_noise_update = 0;
        }
    }
    
    /**
     * Calculate noise floor as 20th percentile of window samples
     * Uses partial selection sort for efficiency (only sorts up to target percentile)
     * 
     * @return Estimated noise floor (ADC units)
     */
    uint16_t calculateNoiseFloor() const {
        // Create working copy of window
        uint16_t working[WINDOW_SIZE];
        for (size_t i = 0; i < WINDOW_SIZE; i++) {
            working[i] = window_samples[i];
        }
        
        // Find 20th percentile using partial selection sort
        // Only sort up to index 12 (20% of 64), much faster than full sort
        constexpr size_t target_index = WINDOW_SIZE / 5;  // Index 12 for 20th percentile
        
        for (size_t i = 0; i <= target_index; i++) {
            // Find minimum in remaining unsorted portion
            size_t min_idx = i;
            for (size_t j = i + 1; j < WINDOW_SIZE; j++) {
                if (working[j] < working[min_idx]) {
                    min_idx = j;
                }
            }
            // Swap to position
            if (min_idx != i) {
                uint16_t temp = working[i];
                working[i] = working[min_idx];
                working[min_idx] = temp;
            }
        }
        
        return working[target_index];
    }
    
    /**
     * Add sample to rolling window for min/max tracking
     * 
     * @param sample ADC reading to add
     */
    void addToWindow(uint16_t sample) {
        window_samples[window_index] = sample;
        window_index = (window_index + 1) % WINDOW_SIZE;
        
        // Track how many valid samples we have (up to WINDOW_SIZE)
        if (window_count < WINDOW_SIZE) {
            window_count++;
        }
        
        // Recalculate min/max from valid samples only
        size_t valid_count = window_count;
        min_value = window_samples[0];
        max_value = window_samples[0];
        for (size_t i = 1; i < valid_count; i++) {
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
        // Always wait full 500ms from start or last telemetry
        if (last_telemetry_us == 0) {
            // First telemetry: only publish if 500ms has passed from boot
            return current_timestamp_us >= TELEMETRY_INTERVAL_US;
        }
        // Subsequent telemetry: 500ms since last publish
        return (current_timestamp_us - last_telemetry_us) >= TELEMETRY_INTERVAL_US;
    }
};

} // namespace clap_metronome

#endif // AUDIO_DETECTION_STATE_H
