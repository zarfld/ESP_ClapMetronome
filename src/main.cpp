/**
 * @file main.cpp
 * @brief ESP_ClapMetronome - Main Application Entry Point
 * 
 * Phase 05: Implementation (ISO/IEC/IEEE 12207:2017)
 * Implements: Requirements from Phase 02
 * Architecture: Decisions from Phase 03 (ADRs)
 * Design: Component specifications from Phase 04
 * 
 * TDD Implementation Status:
 * - Wave 1: Core Audio Detection (Cycles 1-14) ✅ COMPLETE
 * - Wave 2: BPM Tracking & Timing - IN PROGRESS
 * 
 * Standards: ISO/IEC/IEEE 12207:2017 (Implementation Process)
 * XP Practices: Test-Driven Development, Simple Design, Continuous Integration
 * 
 * See: Project architecture in 03-architecture/views/
 * See: Component designs in 04-design/components/
 */

#include <Arduino.h>

// Core component headers (TDD-implemented modules)
#include "timing/TimingManager.h"
#include "audio/AudioDetection.h"

// Platform-specific includes
#if defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <esp_task_wdt.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
#endif

// Library includes
#include <Wire.h>

// Namespace
using namespace clap_metronome;

//==============================================================================
// Global Component Instances
//==============================================================================

TimingManager timing_manager;
AudioDetection audio_detection(&timing_manager);

//==============================================================================
// Configuration
//==============================================================================

// Audio input pin
#if defined(ESP32)
  const int AUDIO_INPUT_PIN = 36;  // GPIO36 (ADC1_CH0)
  const int LED_PIN = 2;           // Built-in LED
#elif defined(ESP8266)
  const int AUDIO_INPUT_PIN = A0;  // Analog pin
  const int LED_PIN = 2;           // Built-in LED
#endif

// Sample rate: 16kHz (62.5μs per sample)
const uint32_t SAMPLE_INTERVAL_US = 62;
uint64_t last_sample_time_us = 0;

//==============================================================================
// Beat Event Handler
//==============================================================================

/**
 * Callback invoked when a beat is detected
 * 
 * Implements: AC-AUDIO-004 Beat Event Emission
 */
void onBeatDetected(const BeatEvent& event) {
    // Flash LED on beat
    digitalWrite(LED_PIN, LOW);
    
    // Log beat to serial
    Serial.print("Beat detected! timestamp=");
    Serial.print((uint32_t)(event.timestamp_us / 1000));
    Serial.print("ms, amplitude=");
    Serial.print(event.amplitude);
    Serial.print(", threshold=");
    Serial.print(event.threshold);
    Serial.print(", gain=");
    Serial.print(event.gain_level);
    Serial.print(", kick_only=");
    Serial.println(event.kick_only);
    
    // LED will turn off after 50ms in main loop
}

//==============================================================================
// Telemetry Handler
//==============================================================================

/**
 * Callback invoked every 500ms for telemetry
 * 
 * Implements: AC-AUDIO-007 Telemetry Updates
 */
void onTelemetry(const AudioTelemetry& telemetry) {
    Serial.print("Telemetry: ");
    Serial.print("ADC=");
    Serial.print(telemetry.adc_value);
    Serial.print(", threshold=");
    Serial.print(telemetry.threshold);
    Serial.print(", beats=");
    Serial.print(telemetry.beat_count);
    Serial.print(", FP=");
    Serial.println(telemetry.false_positive_count);
}

//==============================================================================
// Arduino Setup
//==============================================================================

#ifndef UNIT_TEST  // Don't define setup() when running unit tests
void setup() {
    // Initialize serial
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
        ; // Wait for serial (up to 3 seconds)
    }
    Serial.println("\n\n=== ESP_ClapMetronome Starting ===");
    Serial.println("Phase 05: TDD Implementation");
    Serial.println("Wave 1: Audio Detection (Cycles 1-14) COMPLETE");
    
    // Configure watchdog
    #if defined(ESP32)
    esp_task_wdt_init(40000, true);  // 40s timeout
    esp_task_wdt_add(NULL);
    Serial.println("ESP32 watchdog configured (40s)");
    #elif defined(ESP8266)
    ESP.wdtDisable();
    ESP.wdtEnable(40000);
    Serial.println("ESP8266 watchdog configured (40s)");
    #endif
    
    // Initialize GPIO
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);  // LED off (active low)
    pinMode(AUDIO_INPUT_PIN, INPUT);
    Serial.print("Audio input configured on pin ");
    Serial.println(AUDIO_INPUT_PIN);
    
    // Initialize timing manager
    Serial.println("Initializing timing manager...");
    if (timing_manager.init()) {
        Serial.println("✓ Timing manager initialized");
        if (timing_manager.rtcHealthy()) {
            Serial.println("✓ RTC3231 detected and healthy");
        } else {
            Serial.println("⚠ Using fallback timing (micros)");
        }
    } else {
        Serial.println("✗ Timing manager initialization FAILED");
    }
    
    // Initialize audio detection
    Serial.println("Initializing audio detection...");
    if (audio_detection.init()) {
        Serial.println("✓ Audio detection initialized");
        Serial.println("  - Adaptive threshold: enabled");
        Serial.println("  - State machine: 4-state FSM");
        Serial.println("  - AGC: 60dB/50dB/40dB levels");
        Serial.println("  - False positive rejection: 3-layer");
    } else {
        Serial.println("✗ Audio detection initialization FAILED");
    }
    
    // Register callbacks
    audio_detection.onBeat(onBeatDetected);
    audio_detection.onTelemetry(onTelemetry);
    Serial.println("✓ Callbacks registered");
    
    Serial.println("\n=== System Ready ===");
    Serial.println("Listening for claps at 16kHz sample rate...\n");
    
    // Initialize sample timing
    last_sample_time_us = timing_manager.getTimestampUs();
}

//==============================================================================
// Arduino Main Loop
//==============================================================================

void loop() {
    // Get current timestamp
    uint64_t current_time_us = timing_manager.getTimestampUs();
    
    // Check if it's time to sample (16kHz = 62.5μs intervals)
    if ((current_time_us - last_sample_time_us) >= SAMPLE_INTERVAL_US) {
        last_sample_time_us = current_time_us;
        
        // Read ADC
        uint16_t adc_value = analogRead(AUDIO_INPUT_PIN);
        
        // Process sample through audio detection engine
        audio_detection.processSample(adc_value);
    }
    
    // Turn off LED after beat (50ms delay)
    static uint64_t led_off_time = 0;
    if (digitalRead(LED_PIN) == LOW) {
        if (led_off_time == 0) {
            led_off_time = current_time_us + 50000;  // 50ms from now
        } else if (current_time_us >= led_off_time) {
            digitalWrite(LED_PIN, HIGH);  // LED off
            led_off_time = 0;
        }
    }
    
    // Feed watchdog
    #if defined(ESP32)
    esp_task_wdt_reset();
    #elif defined(ESP8266)
    ESP.wdtFeed();
    #endif
}
#endif  // UNIT_TEST
