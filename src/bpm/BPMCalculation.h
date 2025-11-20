/**
 * BPM Calculation Engine - Component Header
 * 
 * @file BPMCalculation.h
 * @component DES-C-002: BPM Calculation Engine
 * @implements #46 REQ-F-002: Calculate BPM from tap timestamps
 * @architecture #22 ARC-C-002: BPM Calculation (tap buffer)
 * @interfaces DES-I-006 (BPM Update), DES-I-007 (Tap Addition)
 * 
 * @description
 * Calculates beats per minute (BPM) from beat event timestamps using
 * circular buffer averaging with tempo validation and stability detection.
 * 
 * @algorithm
 * - BPM = 60,000,000 µs / average_interval_µs
 * - Stability: Coefficient of Variation (CV) < 5%
 * - Tempo correction: Detect half/double tempo (5 consecutive anomalies)
 * - Invalid interval filtering: Reject <100ms or >2000ms
 * 
 * @performance
 * - Calculation latency: <5ms
 * - Memory: 572B RAM
 * - CPU: <2% average
 * 
 * @see https://github.com/zarfld/ESP_ClapMetronome/issues/46
 * 
 * @standards ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * @phase Phase 05 - Implementation
 * @date 2025-11-20
 */

#ifndef BPM_CALCULATION_H
#define BPM_CALCULATION_H

#include <cstdint>
#include <functional>
#include "BPMCalculationState.h"
#include "../interfaces/ITimingProvider.h"

/**
 * BPM Update Event (DES-I-006)
 * 
 * Callback payload when BPM changes
 */
struct BPMUpdateEvent {
    float bpm;                  ///< Current BPM value
    bool is_stable;             ///< Stability flag (CV < 5%)
    uint64_t timestamp_us;      ///< Timestamp when BPM updated
    uint8_t tap_count;          ///< Number of taps in calculation
};

/**
 * BPM Calculation Engine
 * 
 * Implements DES-C-002: BPM Calculation Engine
 * Memory: 572B RAM (544B state + 28B vtable/padding)
 */
class BPMCalculation {
public:
    /**
     * Constructor
     * @param timing_provider Timing service for timestamps (DES-I-001)
     */
    explicit BPMCalculation(ITimingProvider* timing_provider);
    
    /**
     * Destructor
     */
    ~BPMCalculation() = default;
    
    /**
     * Initialize BPM calculation engine
     * Resets all state to defaults
     */
    void init();
    
    // ===== DES-I-007: Tap Addition Interface =====
    
    /**
     * Add a tap timestamp to the buffer
     * 
     * @param timestamp_us Tap timestamp in microseconds
     * 
     * @behavior
     * - Calculates interval from last tap
     * - Validates interval (100ms - 2000ms)
     * - Adds to circular buffer
     * - Recalculates BPM if ≥2 taps
     * - Detects half/double tempo
     * - Updates stability flag
     * - Fires onBPMUpdate callback if BPM changed
     * 
     * @performance <5ms execution time
     */
    void addTap(uint64_t timestamp_us);
    
    /**
     * Clear all taps and reset to initial state
     */
    void clear();
    
    // ===== DES-I-006: BPM Update Interface =====
    
    /**
     * Get current BPM value
     * @return BPM (0.0 if <2 taps)
     */
    float getBPM() const;
    
    /**
     * Check if BPM is stable
     * @return true if CV < 5% and ≥4 taps
     */
    bool isStable() const;
    
    /**
     * Get number of taps in buffer
     * @return Tap count (0-64)
     */
    uint8_t getTapCount() const;
    
    /**
     * Get coefficient of variation (CV)
     * @return CV percentage (0-100%)
     */
    float getCoefficientOfVariation() const;
    
    /**
     * Register callback for BPM updates
     * @param callback Function called when BPM changes
     */
    void onBPMUpdate(std::function<void(const BPMUpdateEvent&)> callback);
    
private:
    ITimingProvider* timing_;               ///< Timing service
    BPMCalculationState state_;             ///< Component state (544B)
    std::function<void(const BPMUpdateEvent&)> bpm_update_callback_; ///< BPM change callback
    
    /**
     * Calculate BPM from current tap buffer
     * 
     * @algorithm
     * 1. Calculate average interval
     * 2. Convert to BPM: 60,000,000 / avg_interval_us
     * 3. Calculate standard deviation and CV
     * 4. Update stability flag
     * 5. Fire callback if BPM changed
     */
    void calculateBPM();
    
    /**
     * Calculate average interval from tap buffer
     * @return Average interval in microseconds (0 if <2 taps)
     */
    uint64_t calculateAverageInterval();
    
    /**
     * Calculate standard deviation of intervals
     * @param avg_interval Average interval in microseconds
     * @return Standard deviation in microseconds
     */
    float calculateStandardDeviation(uint64_t avg_interval);
    
    /**
     * Detect half-tempo pattern (5 consecutive 2× intervals)
     * @param current_interval Current tap interval
     */
    void detectHalfTempo(uint64_t current_interval);
    
    /**
     * Detect double-tempo pattern (5 consecutive 0.5× intervals)
     * @param current_interval Current tap interval
     */
    void detectDoubleTempo(uint64_t current_interval);
    
    /**
     * Apply half-tempo correction (divide BPM by 2)
     */
    void applyHalfTempoCorrection();
    
    /**
     * Apply double-tempo correction (multiply BPM by 2)
     */
    void applyDoubleTempoCorrection();
    
    /**
     * Fire BPM update callback
     */
    void fireBPMUpdateCallback();
};

#endif // BPM_CALCULATION_H
