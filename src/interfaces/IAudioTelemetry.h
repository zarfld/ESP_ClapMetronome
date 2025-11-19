/**
 * DES-I-005: Audio Telemetry Interface
 * 
 * Interface for components that publish periodic audio diagnostics/telemetry.
 * Used by Audio Detection Engine (DES-C-001) to report state to Web/MQTT.
 * 
 * Implements: #45 (DES-C-001 Audio Detection Engine)
 * Interface: DES-I-005 (Audio Telemetry)
 * Requirement: REQ-F-001 (Audio monitoring), REQ-NF-004 (Telemetry)
 * Standards: ISO/IEC/IEEE 12207:2017 (Interface Design)
 * 
 * TDD Status: RED ⏳ (Interface definition)
 * 
 * See: https://github.com/zarfld/ESP_ClapMetronome/issues/45
 */

#ifndef I_AUDIO_TELEMETRY_H
#define I_AUDIO_TELEMETRY_H

#include <cstdint>
#include <functional>

namespace clap_metronome {

/**
 * Audio telemetry data structure
 * Published every 500ms (AC-AUDIO-007)
 */
struct AudioTelemetry {
    uint64_t timestamp_us;       ///< Timestamp of telemetry snapshot
    uint16_t adc_value;          ///< Current ADC reading (0-4095)
    uint16_t min_value;          ///< Minimum in current window
    uint16_t max_value;          ///< Maximum in current window
    uint16_t threshold;          ///< Current adaptive threshold
    uint8_t  gain_level;         ///< MAX9814 gain (0=40dB, 1=50dB, 2=60dB)
    uint8_t  state;              ///< Detection state (see DetectionState enum)
    uint32_t beat_count;         ///< Total beats detected since boot
    uint32_t false_positive_count; ///< Noise rejections (below threshold)
};

/**
 * Audio telemetry callback signature
 * Consumers register this callback to receive periodic telemetry
 */
using AudioTelemetryCallback = std::function<void(const AudioTelemetry&)>;

/**
 * Interface: Audio Telemetry Publisher
 * 
 * Pure virtual interface for components that publish audio diagnostics.
 * Audio Detection Engine implements this to report state.
 */
class IAudioTelemetry {
public:
    virtual ~IAudioTelemetry() = default;

    /**
     * Register callback for telemetry updates
     * 
     * @param callback Function to call every 500ms with telemetry data
     * 
     * Example:
     *   audio.onTelemetry([](const AudioTelemetry& telem) {
     *       Serial.printf("ADC: %d, Threshold: %d, Beats: %lu\n",
     *                     telem.adc_value, telem.threshold, telem.beat_count);
     *   });
     */
    virtual void onTelemetry(AudioTelemetryCallback callback) = 0;

    /**
     * Check if a telemetry callback is currently registered
     * 
     * @return true if callback registered, false otherwise
     */
    virtual bool hasTelemetryCallback() const = 0;
};

} // namespace clap_metronome

#endif // I_AUDIO_TELEMETRY_H
