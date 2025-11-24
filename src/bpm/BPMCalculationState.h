/**
 * BPM Calculation State - Data Model
 * 
 * @file BPMCalculationState.h
 * @component DES-C-002: BPM Calculation Engine
 * @data_model DES-D-003: Tap Circular Buffer
 * @implements #46 REQ-F-002: Calculate BPM from tap timestamps
 * @architecture #22 ARC-C-002: BPM Calculation (tap buffer)
 * 
 * @description
 * State data for BPM calculation using circular buffer averaging.
 * Stores up to 64 tap timestamps for accurate BPM computation with
 * half/double tempo correction and stability detection.
 * 
 * @memory_layout
 * - Tap buffer: 64 × 8B = 512B (uint64_t timestamps)
 * - State vars: 7 × 4B + 2 × 1B = 30B
 * - Alignment: 2B padding
 * - Total: 544B (within 572B budget from design)
 * 
 * @algorithm
 * - BPM = 60,000,000 µs / average_interval_µs
 * - Stability: CV (Coefficient of Variation) < 5%
 * - Tempo correction: 5 consecutive 2× or 0.5× intervals
 * 
 * @see https://github.com/zarfld/ESP_ClapMetronome/issues/46
 * 
 * @standards ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * @phase Phase 05 - Implementation
 * @date 2025-11-20
 */

#ifndef BPM_CALCULATION_STATE_H
#define BPM_CALCULATION_STATE_H

#include <cstdint>

/**
 * BPM Calculation State Structure
 * 
 * Implements DES-D-003: Tap Circular Buffer
 * Memory: 544 bytes (512B buffer + 32B state)
 */
struct BPMCalculationState {
    // ===== Primary Tap Circular Buffer (DES-D-003) =====
    static constexpr uint8_t MAX_TAPS = 64;          ///< Buffer size (8 measures at 80-200 BPM for stable averaging)
    static constexpr uint8_t MIN_TAPS_EVEN = 4;      ///< Minimum taps for evenly-spaced rhythm (simple quarter notes)
    static constexpr uint8_t MIN_TAPS_SUBDIVISION = 8;  ///< Minimum taps for subdivision detection (mixed eighth/quarter notes)
    static constexpr uint8_t MIN_TAPS_BASIC = 2;     ///< Absolute minimum for any BPM calculation (1 interval)
    // At 80 BPM: 64 beats = 48 seconds (16 measures) - extensive history for subdivisions
    // At 200 BPM: 64 beats = 19.2 seconds (8 measures) - stable tempo detection
    uint64_t tap_buffer[MAX_TAPS];                   ///< Circular buffer of tap timestamps (µs)
    uint8_t tap_count;                                ///< Number of taps in buffer (0-64)
    uint8_t write_index;                              ///< Next write position (0-63)
    
    // ===== Shadow Tracker (Parallel Tempo Detection) =====
    static constexpr uint8_t SHADOW_TAPS = 16;       ///< Shadow buffer size (recent beats only, faster response)
    uint64_t shadow_buffer[SHADOW_TAPS];             ///< Shadow circular buffer (µs)
    uint8_t shadow_count;                            ///< Taps in shadow buffer (0-16)
    uint8_t shadow_write_index;                      ///< Shadow write position (0-15)
    float shadow_bpm;                                ///< Shadow tracker BPM
    float shadow_cv;                                 ///< Shadow tracker CV%
    uint8_t shadow_confidence;                       ///< Shadow confidence score (0-10, increments when stable)
    
    // ===== BPM Calculation Results =====
    float current_bpm;                                ///< Current calculated BPM (0.0 if <2 taps)
    float locked_bpm;                                 ///< Locked/stable BPM (metronome tempo)
    bool is_stable;                                   ///< Stability flag (CV < 5%)
    uint8_t stable_count;                             ///< Consecutive stable calculations
    
    // ===== Statistics =====
    uint64_t last_tap_us;                             ///< Timestamp of last tap (for interval calc)
    uint64_t average_interval_us;                     ///< Average tap interval (µs)
    float coefficient_of_variation;                   ///< CV = stddev / mean (0-100%)
    float relative_deviation_percent;                 ///< Deviation from locked tempo (0-100%, 0 when unlocked)
    
    // ===== Tempo Correction State =====
    uint8_t half_tempo_count;                         ///< Consecutive intervals ~2× average (0-5)
    uint8_t double_tempo_count;                       ///< Consecutive intervals ~0.5× average (0-5)
    bool tempo_correction_applied;                    ///< Flag to prevent re-applying correction
    uint8_t pattern_mismatch_count;                   ///< Consecutive beats not matching locked pattern (0-5)
    
    // ===== Constants =====
    static constexpr uint64_t MIN_INTERVAL_US = 100000;   ///< 100ms = 600 BPM max
    static constexpr uint64_t MAX_INTERVAL_US = 2000000;  ///< 2000ms = 30 BPM min
    static constexpr float MIN_BPM = 70.0f;               ///< Minimum valid tempo (prevents false positives)
    static constexpr float MAX_BPM = 200.0f;              ///< Maximum valid tempo (prevents false positives)
    static constexpr float STABILITY_CV_THRESHOLD = 12.0f; ///< CV < 12% = stable (relaxed for real human drumming - was 8%)
    static constexpr float TEMPO_TOLERANCE_PERCENT = 4.0f; ///< ±4% tolerance for tempo change (more sensitive - was 6%)
    static constexpr uint8_t STABLE_COUNT_LOCK = 3;      ///< 3 consecutive stable = lock tempo
    static constexpr uint8_t DEVIATION_COUNT_UNLOCK = 3;  ///< 3 consecutive deviations = unlock tempo (was 5)
    static constexpr float HALF_TEMPO_RATIO = 1.8f;       ///< Interval > 1.8× avg = half tempo
    static constexpr float DOUBLE_TEMPO_RATIO = 0.6f;     ///< Interval < 0.6× avg = double tempo
    static constexpr uint8_t TEMPO_CORRECTION_THRESHOLD = 5; ///< 5 consecutive = apply correction
    static constexpr uint64_t LOCK_TIMEOUT_US = 3000000;  ///< 3s timeout - unlock if no beats (covers 2 bars at 40 BPM)
    static constexpr uint8_t SHADOW_CONFIDENCE_THRESHOLD = 5; ///< Shadow confidence needed to replace locked tempo
    static constexpr float SHADOW_SWITCH_MIN_BPM_DIFF = 5.0f; ///< Min BPM difference to consider switch (avoids noise)
    static constexpr uint8_t PATTERN_MISMATCH_UNLOCK = 5;    ///< 5 consecutive pattern mismatches = force unlock
    
    /**
     * Initialize state to defaults
     */
    void init() {
        tap_count = 0;
        write_index = 0;
        current_bpm = 0.0f;
        locked_bpm = 0.0f;
        is_stable = false;
        stable_count = 0;
        last_tap_us = 0;
        average_interval_us = 0;
        coefficient_of_variation = 0.0f;
        relative_deviation_percent = 0.0f;
        half_tempo_count = 0;
        double_tempo_count = 0;
        tempo_correction_applied = false;
        pattern_mismatch_count = 0;
        
        // Initialize shadow tracker
        shadow_count = 0;
        shadow_write_index = 0;
        shadow_bpm = 0.0f;
        shadow_cv = 0.0f;
        shadow_confidence = 0;
        
        // Initialize buffers to zeros
        for (uint8_t i = 0; i < MAX_TAPS; ++i) {
            tap_buffer[i] = 0;
        }
        for (uint8_t i = 0; i < SHADOW_TAPS; ++i) {
            shadow_buffer[i] = 0;
        }
    }
    
    /**
     * Reset all state (equivalent to init)
     */
    void reset() {
        init();
    }
};

#endif // BPM_CALCULATION_STATE_H
