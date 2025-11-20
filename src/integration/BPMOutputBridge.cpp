/**
 * @file BPMOutputBridge.cpp
 * @brief Integration layer implementation
 */

#include "BPMOutputBridge.h"

BPMOutputBridge::BPMOutputBridge(BPMCalculation* bpm, OutputController* output)
    : bpm_engine_(bpm)
    , output_controller_(output)
    , auto_sync_enabled_(false)
    , is_syncing_(false)
    , last_bpm_(0.0f)
{
}

void BPMOutputBridge::init() {
    // Register as BPM update listener
    bpm_engine_->onBPMUpdate([this](const BPMUpdateEvent& event) {
        this->onBPMUpdate(event);
    });
    
    // Initialize state
    is_syncing_ = false;
    last_bpm_ = 0.0f;
}

void BPMOutputBridge::onBPMUpdate(const BPMUpdateEvent& event) {
    // Update output controller with new BPM
    if (event.bpm >= 40.0f && event.bpm <= 240.0f) {
        uint16_t bpm_int = static_cast<uint16_t>(event.bpm + 0.5f);
        output_controller_->updateBPM(bpm_int);
        last_bpm_ = event.bpm;
    }
    
    // Reset syncing flag if output stopped
    if (output_controller_->getState() == OutputState::STOPPED) {
        is_syncing_ = false;
    }
    
    // Handle auto-sync on stability
    if (auto_sync_enabled_ && event.is_stable && !is_syncing_) {
        startSyncIfReady(event.bpm);
    }
}

void BPMOutputBridge::onBeatDetected(uint64_t timestamp_us) {
    // Trigger relay pulse if relay enabled
    OutputConfig config = output_controller_->getConfig();
    if (config.mode == OutputMode::RELAY_ONLY || config.mode == OutputMode::BOTH) {
        output_controller_->pulseRelay();
    }
}

void BPMOutputBridge::setAutoSyncEnabled(bool enabled) {
    auto_sync_enabled_ = enabled;
}

bool BPMOutputBridge::isAutoSyncEnabled() const {
    return auto_sync_enabled_;
}

void BPMOutputBridge::startSyncIfReady(float bpm) {
    // Verify BPM in valid range
    if (bpm < 40.0f || bpm > 240.0f) {
        return;
    }
    
    // Check if output is in MIDI mode
    OutputConfig config = output_controller_->getConfig();
    if (config.mode != OutputMode::MIDI_ONLY && config.mode != OutputMode::BOTH) {
        return;
    }
    
    // Start sync
    uint16_t bpm_int = static_cast<uint16_t>(bpm + 0.5f);
    bool started = output_controller_->startSync(bpm_int);
    
    if (started) {
        is_syncing_ = true;
    }
}
