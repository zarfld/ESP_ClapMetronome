/**
 * DES-I-004: Beat Event Publisher Interface
 * 
 * Interface for components that detect and publish beat/clap events.
 * Used by Audio Detection Engine (DES-C-001) to notify BPM Calculator and other consumers.
 * 
 * Implements: #45 (DES-C-001 Audio Detection Engine)
 * Interface: DES-I-004 (Beat Event)
 * Requirement: REQ-F-001 (Acoustic clap/kick detection)
 * Standards: ISO/IEC/IEEE 12207:2017 (Interface Design)
 * 
 * TDD Status: RED ⏳ (Interface definition)
 * 
 * See: https://github.com/zarfld/ESP_ClapMetronome/issues/45
 */

#ifndef I_BEAT_EVENT_PUBLISHER_H
#define I_BEAT_EVENT_PUBLISHER_H

#include <cstdint>
#include <functional>

namespace clap_metronome {

/**
 * Beat event data structure
 * Contains all relevant information about a detected beat/clap
 */
struct BeatEvent {
    uint64_t timestamp_us;    ///< Microsecond timestamp from DES-I-001 (Timing Manager)
    uint16_t amplitude;       ///< Peak amplitude of detected beat (ADC units 0-4095)
    uint16_t threshold;       ///< Current adaptive threshold at detection time
    uint8_t  gain_level;      ///< MAX9814 gain level (0=40dB, 1=50dB, 2=60dB)
    bool     kick_only;       ///< True if rise time >4ms (kick drum characteristic)
};

/**
 * Beat event callback signature
 * Consumers register this callback to receive beat notifications
 */
using BeatEventCallback = std::function<void(const BeatEvent&)>;

/**
 * Interface: Beat Event Publisher
 * 
 * Pure virtual interface for components that detect and publish beat events.
 * Audio Detection Engine implements this to notify consumers.
 */
class IBeatEventPublisher {
public:
    virtual ~IBeatEventPublisher() = default;

    /**
     * Register callback for beat event notifications
     * 
     * @param callback Function to call when beat detected
     * 
     * Example:
     *   audio.onBeat([](const BeatEvent& event) {
     *       Serial.printf("Beat at %llu us, amplitude %d\n", 
     *                     event.timestamp_us, event.amplitude);
     *   });
     */
    virtual void onBeat(BeatEventCallback callback) = 0;

    /**
     * Check if a beat callback is currently registered
     * 
     * @return true if callback registered, false otherwise
     */
    virtual bool hasBeatCallback() const = 0;
};

} // namespace clap_metronome

#endif // I_BEAT_EVENT_PUBLISHER_H
