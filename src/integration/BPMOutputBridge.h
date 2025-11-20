/**
 * @file BPMOutputBridge.h
 * @brief Integration layer between BPM Calculation Engine and Output Controller
 * 
 * Component: Integration Layer (BPM ↔ Output)
 * Implements: AC-OUT-014, AC-OUT-015, AC-OUT-016, AC-OUT-017
 * 
 * @description
 * Coordinates BPM calculation engine with output controller:
 * - Forwards BPM updates to output timer
 * - Triggers relay pulses on beat detection
 * - Manages stability-based auto-sync
 * - Provides end-to-end integration
 * 
 * @standards ISO/IEC/IEEE 12207:2017 (Integration Process)
 * @phase Phase 05 - Implementation
 * @date 2025-11-20
 */

#ifndef BPM_OUTPUT_BRIDGE_H
#define BPM_OUTPUT_BRIDGE_H

#include "../bpm/BPMCalculation.h"
#include "../output/OutputController.h"

/**
 * @brief BPM-Output Integration Bridge
 * 
 * Coordinates BPM calculation and output synchronization.
 * Acts as mediator between detection/BPM subsystem and output subsystem.
 */
class BPMOutputBridge {
public:
    /**
     * @brief Constructor
     * @param bpm BPM calculation engine
     * @param output Output controller
     */
    BPMOutputBridge(BPMCalculation* bpm, OutputController* output);
    
    /**
     * @brief Destructor
     */
    ~BPMOutputBridge() = default;
    
    /**
     * @brief Initialize bridge
     * 
     * Registers callbacks with BPM engine.
     */
    void init();
    
    /**
     * @brief Handle BPM update event
     * 
     * Called by BPM engine when BPM changes.
     * Forwards update to output controller.
     * Manages auto-sync if enabled.
     * 
     * @param event BPM update event
     */
    void onBPMUpdate(const BPMUpdateEvent& event);
    
    /**
     * @brief Handle beat detection event
     * 
     * Called when beat detected.
     * Triggers relay pulse if relay enabled.
     * 
     * @param timestamp_us Detection timestamp
     */
    void onBeatDetected(uint64_t timestamp_us);
    
    /**
     * @brief Enable/disable auto-sync on stability
     * 
     * When enabled, MIDI sync starts automatically when
     * BPM becomes stable (CV < 5%, ≥4 taps).
     * 
     * @param enabled true to enable auto-sync
     */
    void setAutoSyncEnabled(bool enabled);
    
    /**
     * @brief Check if auto-sync enabled
     * @return true if auto-sync enabled
     */
    bool isAutoSyncEnabled() const;
    
private:
    BPMCalculation* bpm_engine_;           ///< BPM calculation engine
    OutputController* output_controller_;  ///< Output controller
    bool auto_sync_enabled_;               ///< Auto-sync on stability flag
    bool is_syncing_;                      ///< Currently syncing flag
    float last_bpm_;                       ///< Last BPM value
    
    /**
     * @brief Start sync if conditions met
     * 
     * Starts MIDI sync if:
     * - Auto-sync enabled
     * - BPM is stable
     * - Not already syncing
     * 
     * @param bpm BPM value to sync at
     */
    void startSyncIfReady(float bpm);
};

#endif // BPM_OUTPUT_BRIDGE_H
