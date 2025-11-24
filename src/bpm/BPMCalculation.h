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
     * Returns locked BPM when stable, otherwise calculated BPM
     * @return BPM (0.0 if <2 taps)
     */
    float getBPM() const;
    
    /**
     * Get locked/stable BPM (metronome tempo)
     * @return Locked BPM (0.0 if not yet locked)
     */
    float getLockedBPM() const;
    
    /**
     * Get calculated BPM (may vary)
     * @return Current calculated BPM (0.0 if <2 taps)
     */
    float getCalculatedBPM() const;
    
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
     * Get relative deviation from locked tempo
     * Shows timing accuracy when tempo is locked
     * @return Deviation percentage (0-100%, 0 when not locked)
     */
    float getRelativeDeviation() const;
    
    /**
     * Get stable count for debugging
     * When not locked: counts consecutive stable readings (increments to 3+)
     * When locked: counts remaining deviation tolerance (decrements from 5 to 0)
     * @return Current stable_count value
     */
    uint8_t getStableCount() const;
    
    /**
     * Check for lock timeout and unlock if no beats for 3 seconds
     * MUST be called periodically (e.g., every 500ms) to detect stale locks
     * @param current_time_us Current timestamp in microseconds
     */
    void checkTimeout(uint64_t current_time_us);
    
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
     * Apply half-tempo correction (calculate BPM from baseline tempo)
     * @param baseline_interval_us Average interval from baseline (pre-tempo-change)
     */
    void applyHalfTempoCorrection(uint64_t baseline_interval_us);
    
    /**
     * Apply double-tempo correction (calculate BPM from baseline tempo)
     * @param baseline_interval_us Average interval from baseline (pre-tempo-change)
     */
    void applyDoubleTempoCorrection(uint64_t baseline_interval_us);
    
    /**
     * Fire BPM update callback
     */
    void fireBPMUpdateCallback();
    
    // ===== Shadow Tracker (Parallel Tempo Detection) =====
    
    /**
     * Update shadow tracker with new tap
     * Runs in parallel with primary tracker for tempo change detection
     * @param timestamp_us New tap timestamp
     */
    void updateShadowTracker(uint64_t timestamp_us);
    
    /**
     * Calculate shadow BPM from shadow buffer
     * Uses simplified algorithm for faster response
     */
    void calculateShadowBPM();
    
    /**
     * Check if shadow tracker should replace primary locked tempo
     * @return true if shadow tracker is more confident and tempo differs
     */
    bool shouldSwitchTempo();
    
    /**
     * Switch to shadow tracker's tempo
     * Replaces locked tempo with shadow tempo and resets confidence
     */
    void switchToShadowTempo();
};

#endif // BPM_CALCULATION_H
