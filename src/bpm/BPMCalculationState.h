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
    // ===== Tap Circular Buffer (DES-D-003) =====
    static constexpr uint8_t MAX_TAPS = 64;          ///< Buffer size (power of 2 for fast modulo)
    uint64_t tap_buffer[MAX_TAPS];                   ///< Circular buffer of tap timestamps (µs)
    uint8_t tap_count;                                ///< Number of taps in buffer (0-64)
    uint8_t write_index;                              ///< Next write position (0-63)
    
    // ===== BPM Calculation Results =====
    float current_bpm;                                ///< Current BPM (0.0 if <2 taps)
    bool is_stable;                                   ///< Stability flag (CV < 5%)
    
    // ===== Statistics =====
    uint64_t last_tap_us;                             ///< Timestamp of last tap (for interval calc)
    uint64_t average_interval_us;                     ///< Average tap interval (µs)
    float coefficient_of_variation;                   ///< CV = stddev / mean (0-100%)
    
    // ===== Tempo Correction State =====
    uint8_t half_tempo_count;                         ///< Consecutive intervals ~2× average (0-5)
    uint8_t double_tempo_count;                       ///< Consecutive intervals ~0.5× average (0-5)
    bool tempo_correction_applied;                    ///< Flag to prevent re-applying correction
    
    // ===== Constants =====
    static constexpr uint64_t MIN_INTERVAL_US = 100000;   ///< 100ms = 600 BPM max
    static constexpr uint64_t MAX_INTERVAL_US = 2000000;  ///< 2000ms = 30 BPM min
    static constexpr float STABILITY_CV_THRESHOLD = 5.0f; ///< CV < 5% = stable
    static constexpr float HALF_TEMPO_RATIO = 1.8f;       ///< Interval > 1.8× avg = half tempo
    static constexpr float DOUBLE_TEMPO_RATIO = 0.6f;     ///< Interval < 0.6× avg = double tempo
    static constexpr uint8_t TEMPO_CORRECTION_THRESHOLD = 5; ///< 5 consecutive = apply correction
    
    /**
     * Initialize state to defaults
     */
    void init() {
        tap_count = 0;
        write_index = 0;
        current_bpm = 0.0f;
        is_stable = false;
        last_tap_us = 0;
        average_interval_us = 0;
        coefficient_of_variation = 0.0f;
        half_tempo_count = 0;
        double_tempo_count = 0;
        tempo_correction_applied = false;
        
        // Initialize buffer to zeros
        for (uint8_t i = 0; i < MAX_TAPS; ++i) {
            tap_buffer[i] = 0;
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
