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
#include "bpm/BPMCalculation.h"
#include "output/OutputController.h"
#include "config/ConfigurationManager.h"
#include "web/WebServer.h"
#include "mqtt/MQTTClient.h"

// Platform-specific includes
#if defined(ESP32)
  #include <WiFi.h>
  #include <esp_task_wdt.h>
  #include "soc/soc.h"
  #include "soc/rtc_cntl_reg.h"
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

// Library includes
#include <Wire.h>

// WiFi credentials (from credentials.h - gitignored)
#include "../test/credentials.h"

// Namespace
using namespace clap_metronome;

//==============================================================================
// Global Component Instances
//==============================================================================

TimingManager timing_manager;
AudioDetection audio_detection(&timing_manager);
BPMCalculation bpm_calculation(&timing_manager);  // Requires ITimingProvider
clap_metronome::ConfigurationManager config_manager;
clap_metronome::WebServer web_server(&config_manager, 80);
OutputController output_controller;
clap_metronome::MQTTClient* mqtt_client = nullptr;  // Created after config load

//==============================================================================
// Configuration
//==============================================================================

// Hardware Pin Assignments (Wave 3.8 - Final Configuration)
#if defined(ESP32)
  // DS3231 RTC (I2C)
  const int SDA_PIN = 21;          // GPIO21 - I2C Data
  const int SCL_PIN = 22;          // GPIO22 - I2C Clock
  
  // MAX9814 Microphone
  const int AUDIO_INPUT_PIN = 36;  // GPIO36 (ADC1_CH0) - Audio Signal
  const int GAIN_PIN = 32;         // GPIO32 - Gain Control (40/50/60dB)
  const int AR_PIN = 33;           // GPIO33 - Attack/Release (1:2000/1:4000)
  
  // Application Pins
  const int LED_PIN = 2;           // Built-in LED
  
#elif defined(ESP8266)
  const int AUDIO_INPUT_PIN = A0;  // Analog pin
  const int LED_PIN = 2;           // Built-in LED
#endif

// MAX9814 Gain Levels
enum class GainLevel : uint8_t {
    GAIN_40DB = 0,  // LOW  - Loud environments (concerts, rehearsals)
    GAIN_50DB = 1,  // FLOAT - Moderate environments (normal conversation)
    GAIN_60DB = 2   // HIGH - Quiet rooms (risk of saturation with loud input)
};

GainLevel current_gain = GainLevel::GAIN_40DB;  // Default: 40dB (recommended)

// Sample rate: 16kHz (62.5μs per sample)
const uint32_t SAMPLE_INTERVAL_US = 62;
uint64_t last_sample_time_us = 0;

// WiFi connection state
bool wifi_connected = false;
uint32_t last_wifi_check = 0;
const uint32_t WIFI_CHECK_INTERVAL_MS = 30000;  // Check every 30s

// Telemetry timing
uint32_t last_mqtt_publish = 0;
const uint32_t MQTT_PUBLISH_INTERVAL_MS = 5000;  // Publish every 5s

//==============================================================================
// MAX9814 Gain Control
//==============================================================================

/**
 * Set MAX9814 gain level via GPIO32
 * 
 * Hardware Control:
 * - 40dB: GPIO32 LOW
 * - 50dB: GPIO32 FLOAT (INPUT mode)
 * - 60dB: GPIO32 HIGH
 * 
 * Implements: Wave 3.8 MAX9814 Integration
 */
void setMAX9814Gain(GainLevel level) {
    #if defined(ESP32)
    switch (level) {
        case GainLevel::GAIN_40DB:
            pinMode(GAIN_PIN, OUTPUT);
            digitalWrite(GAIN_PIN, LOW);
            Serial.println("MAX9814 Gain: 40dB (LOW)");
            break;
        case GainLevel::GAIN_50DB:
            pinMode(GAIN_PIN, INPUT);  // Float
            Serial.println("MAX9814 Gain: 50dB (FLOAT)");
            break;
        case GainLevel::GAIN_60DB:
            pinMode(GAIN_PIN, OUTPUT);
            digitalWrite(GAIN_PIN, HIGH);
            Serial.println("MAX9814 Gain: 60dB (HIGH)");
            break;
    }
    current_gain = level;
    #endif
}

/**
 * Set MAX9814 attack/release time via GPIO33
 * 
 * Hardware Control:
 * - Fast (1:2000): GPIO33 LOW  - Recommended for beat detection
 * - Slow (1:4000): GPIO33 HIGH - For sustained signals
 * 
 * Implements: Wave 3.8 MAX9814 Integration
 */
void setMAX9814AttackRelease(bool fast) {
    #if defined(ESP32)
    pinMode(AR_PIN, OUTPUT);
    digitalWrite(AR_PIN, fast ? LOW : HIGH);
    Serial.print("MAX9814 Attack/Release: ");
    Serial.println(fast ? "Fast (1:2000)" : "Slow (1:4000)");
    #endif
}

/**
 * Auto-adjust MAX9814 gain based on signal levels
 * 
 * Algorithm:
 * - If peak > 3800: Reduce to 40dB (avoid saturation)
 * - If avg < 500: Increase to 60dB (boost weak signal)
 * - If 1000-3000: Maintain current gain (optimal range)
 * 
 * Implements: Test 009 Auto-Gain Algorithm
 */
void autoAdjustGain(uint16_t peak_value, uint16_t avg_value) {
    #if defined(ESP32)
    static uint32_t last_adjustment = 0;
    const uint32_t ADJUSTMENT_INTERVAL_MS = 5000;  // Don't adjust too frequently
    
    if (millis() - last_adjustment < ADJUSTMENT_INTERVAL_MS) {
        return;  // Too soon to adjust again
    }
    
    GainLevel new_gain = current_gain;
    
    if (peak_value > 3800) {
        // Signal too loud, reduce gain to prevent saturation
        if (current_gain != GainLevel::GAIN_40DB) {
            new_gain = GainLevel::GAIN_40DB;
            Serial.println("Auto-gain: Reducing to 40dB (high signal detected)");
        }
    } else if (avg_value < 500) {
        // Signal too weak, increase gain
        if (current_gain != GainLevel::GAIN_60DB) {
            new_gain = (current_gain == GainLevel::GAIN_40DB) ? 
                       GainLevel::GAIN_50DB : GainLevel::GAIN_60DB;
            Serial.println("Auto-gain: Increasing gain (weak signal detected)");
        }
    }
    
    if (new_gain != current_gain) {
        setMAX9814Gain(new_gain);
        last_adjustment = millis();
    }
    #endif
}

//==============================================================================
// WiFi Connection Management
//==============================================================================

void connectWiFi() {
  Serial.println("Connecting to WiFi...");
  
  // Enable WiFi power saving mode to reduce peak current draw
  WiFi.mode(WIFI_STA);
  delay(50);  // Let mode change stabilize
  
  WiFi.setSleep(true);  // Enable WiFi sleep mode to reduce power
  WiFi.setTxPower(WIFI_POWER_11dBm);  // Reduce TX power (11dBm instead of default 20dBm)
  
  delay(100);  // Let power stabilize before connecting
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  uint8_t attempts = 0;
  const uint8_t MAX_ATTEMPTS = 40;  // 20 seconds timeout (longer with power saving)
  
  while (WiFi.status() != WL_CONNECTED && attempts < MAX_ATTEMPTS) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Web UI: http://");
    Serial.println(WiFi.localIP());
  } else {
    wifi_connected = false;
    Serial.println("\nWiFi connection failed!");
  }
}

void checkWiFiConnection() {
  uint32_t now = millis();
  if (now - last_wifi_check < WIFI_CHECK_INTERVAL_MS) {
    return;
  }
  last_wifi_check = now;
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    wifi_connected = false;
    connectWiFi();
  }
}

//==============================================================================

/**
 * Callback invoked when a beat is detected
 * 
 * Implements: AC-AUDIO-004 Beat Event Emission
 * Wave 3.8: Includes RTC timestamp, BPM calculation, output triggering
 */
void onBeatDetected(const BeatEvent& event) {
    // Flash LED on beat
    digitalWrite(LED_PIN, LOW);
    
    // Add tap to BPM calculation ONLY for kick/snare beats (rise_time > 4ms)
    // This filters out hi-hats and cymbals which have faster attack
    // and prevents tempo inflation from 8th/16th note subdivisions
    if (event.kick_only) {
        bpm_calculation.addTap(event.timestamp_us);
    }
    
    // Get RTC timestamp if available
    if (timing_manager.rtcHealthy()) {
        // RTC timestamp available - convert to human-readable
        uint64_t timestamp_us = timing_manager.getTimestampUs();
        uint32_t timestamp_ms = timestamp_us / 1000;
        
        Serial.print("[RTC] Beat detected @ ");
        Serial.print(timestamp_ms);
        Serial.print("ms");
    } else {
        // Fallback to micros() timestamp
        Serial.print("Beat detected! timestamp=");
        Serial.print((uint32_t)(event.timestamp_us / 1000));
        Serial.print("ms");
    }
    
    // Log beat details
    Serial.print(", amplitude=");
    Serial.print(event.amplitude);
    Serial.print(", threshold=");
    Serial.print(event.threshold);
    Serial.print(", gain=");
    Serial.print(event.gain_level);
    Serial.print(", kick_only=");
    Serial.print(event.kick_only);
    
    // Indicate if beat was used for BPM calculation
    if (event.kick_only) {
        Serial.print(" [BPM]");
    }
    Serial.println();
    
    // LED will turn off after 50ms in main loop
}

//==============================================================================
// Telemetry Handler
//==============================================================================

/**
 * Callback invoked every 500ms for telemetry
 * 
 * Implements: AC-AUDIO-007 Telemetry Updates
 * Wave 3.8: Enables auto-gain adjustment, MQTT publishing
 */
void onTelemetry(const AudioTelemetry& telemetry) {
    // Check for BPM lock timeout (unlock if no beats for 3 seconds)
    bpm_calculation.checkTimeout(timing_manager.getTimestampUs());
    
    // Get locked and calculated BPM for display
    float locked_bpm = bpm_calculation.getLockedBPM();
    float calculated_bpm = bpm_calculation.getCalculatedBPM();
    bool is_stable = bpm_calculation.isStable();
    
    Serial.print("Telemetry: ");
    Serial.print("ADC=");
    Serial.print(telemetry.adc_value);
    Serial.print(", threshold=");
    Serial.print(telemetry.threshold);
    Serial.print(", beats=");
    Serial.print(telemetry.beat_count);
    Serial.print(", FP=");
    Serial.print(telemetry.false_positive_count);
    Serial.print(", gain=");
    Serial.print(telemetry.gain_level);
    
    // Add BPM information to telemetry output
    if (locked_bpm > 0) {
        // Show locked metronome tempo
        Serial.print(", BPM=");
        Serial.print(locked_bpm, 1);
        Serial.print(" [LOCKED]");
        
        // Show calculated BPM if significantly different (for debugging)
        if (calculated_bpm > 0) {
            float deviation = (calculated_bpm > locked_bpm) 
                             ? (calculated_bpm - locked_bpm) 
                             : (locked_bpm - calculated_bpm);
            float percent_dev = (deviation / locked_bpm) * 100.0f;
            
            if (percent_dev > 3.0f) {  // Show if >3% deviation
                Serial.print(" (calc=");
                Serial.print(calculated_bpm, 1);
                Serial.print(")");
            }
        }
    } else if (calculated_bpm > 0) {
        // No locked tempo yet - show calculated BPM
        Serial.print(", BPM=");
        Serial.print(calculated_bpm, 1);
        Serial.print(is_stable ? " [locking...]" : " [tracking]");
        
        // Show CV and stable count for debugging
        float cv = bpm_calculation.getCoefficientOfVariation();
        uint8_t stable_cnt = bpm_calculation.getStableCount();
        Serial.print(" CV=");
        Serial.print(cv, 1);
        Serial.print("% sc=");
        Serial.print(stable_cnt);
    }
    Serial.println();
    
    // Auto-adjust MAX9814 gain based on signal levels
    autoAdjustGain(telemetry.max_value, telemetry.adc_value);
    
    // Publish MQTT telemetry every 5 seconds
    uint32_t now = millis();
    if (mqtt_client && (now - last_mqtt_publish >= MQTT_PUBLISH_INTERVAL_MS)) {
        last_mqtt_publish = now;
        
        // Get current BPM and stability
        float current_bpm = bpm_calculation.getBPM();
        bool is_stable = bpm_calculation.isStable();
        uint8_t confidence = is_stable ? 95 : 50;
        const char* lock_status = is_stable ? "LOCKED" : "UNLOCKED";
        uint32_t timestamp = millis() / 1000;  // Convert to seconds
        
        // Publish BPM with full telemetry
        mqtt_client->publishBPM(current_bpm, confidence, lock_status, timestamp);
        
        // Publish audio levels (requires max/min tracking)
        static uint16_t max_level = 0;
        static uint16_t min_level = 4095;
        if (telemetry.adc_value > max_level) max_level = telemetry.adc_value;
        if (telemetry.adc_value < min_level) min_level = telemetry.adc_value;
        mqtt_client->publishAudioLevels(telemetry.adc_value, max_level, min_level, 
                                       telemetry.threshold, telemetry.gain_level);
        // Reset min/max for next interval
        max_level = 0;
        min_level = 4095;
    }
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
    Serial.println("Wave 3.8: Hardware Integration (DS3231 + MAX9814) INTEGRATED");
    
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
    
    // Initialize I2C for DS3231 RTC (Wave 3.8)
    #if defined(ESP32)
    Wire.begin(SDA_PIN, SCL_PIN);  // GPIO21=SDA, GPIO22=SCL
    Serial.print("I2C initialized: SDA=");
    Serial.print(SDA_PIN);
    Serial.print(", SCL=");
    Serial.println(SCL_PIN);
    #endif
    
    // Initialize MAX9814 hardware control (Wave 3.8)
    #if defined(ESP32)
    setMAX9814Gain(GainLevel::GAIN_40DB);  // Default: 40dB (recommended)
    setMAX9814AttackRelease(true);          // Fast attack (1:2000)
    Serial.println("MAX9814 hardware control initialized");
    #endif
    
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
    
    // Initialize configuration manager
    Serial.println("Initializing configuration manager...");
    if (config_manager.init()) {
        Serial.println("✓ Configuration manager initialized");
        if (config_manager.loadConfig()) {
            Serial.println("✓ Configuration loaded from NVS");
        } else {
            Serial.println("⚠ Using default configuration");
        }
    } else {
        Serial.println("✗ Configuration manager initialization FAILED");
    }
    
    // Initialize BPM calculation
    Serial.println("Initializing BPM calculation...");
    bpm_calculation.init();
    Serial.println("✓ BPM calculation initialized");
    
    // Initialize output controller
    Serial.println("Initializing output controller...");
    output_controller.init();
    
    // Get configuration and set output mode
    clap_metronome::OutputConfig config_output = config_manager.getOutputConfig();
    if (config_output.midi_enabled) {
        output_controller.setBPM(120);  // Default BPM
        output_controller.startSync();
    }
    Serial.println("✓ Output controller initialized");
    Serial.print("  - MIDI enabled: ");
    Serial.println(config_output.midi_enabled ? "yes" : "no");
    Serial.print("  - Relay enabled: ");
    Serial.println(config_output.relay_enabled ? "yes" : "no");
    
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
    
    // WiFi disabled - USB power insufficient for WiFi radio
    Serial.println("⚠ WiFi disabled (requires external 5V/1A power supply)");
    Serial.println("  Core features active: Beat detection, BPM, MIDI sync");
    // connectWiFi();
    
    // Skip web server and MQTT when WiFi is disabled
    /*
    // Initialize and start web server if WiFi connected
    if (wifi_connected) {
        Serial.println("Initializing web server...");
        if (web_server.init()) {
            web_server.start();
            Serial.println("✓ Web server started on port 80");
            Serial.print("  - Access Web UI at: http://");
            Serial.println(WiFi.localIP());
        } else {
            Serial.println("✗ Web server initialization FAILED");
        }
    }
    
    // Initialize and connect MQTT client if WiFi connected
    if (wifi_connected) {
        clap_metronome::NetworkConfig network_config = config_manager.getNetworkConfig();
        if (network_config.mqtt_enabled && strlen(network_config.mqtt_broker) > 0) {
            Serial.println("Initializing MQTT client...");
            
            // Build MQTT config from NetworkConfig
            static clap_metronome::MQTTConfig mqtt_config;
            mqtt_config.enabled = network_config.mqtt_enabled;
            mqtt_config.broker_host = network_config.mqtt_broker;
            mqtt_config.broker_port = network_config.mqtt_port;
            mqtt_config.username = network_config.mqtt_username;
            mqtt_config.password = network_config.mqtt_password;
            mqtt_config.device_id = "esp32-clap-metronome";  // TODO: Use chip ID
            
            mqtt_client = new clap_metronome::MQTTClient(&mqtt_config);
            if (mqtt_client->connect()) {
                Serial.println("✓ MQTT client connected");
                // publishOnlineStatus is private, handled internally by connect()
            } else {
                Serial.println("✗ MQTT connection FAILED");
            }
        }
    }
    */
    
    Serial.println("\n=== System Ready ===");
    Serial.println("Listening for claps at 16kHz sample rate...");
    Serial.println("Ready for beat detection testing!");
    Serial.println();
    
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
    
    // Update output controller with stable BPM
    float current_bpm = bpm_calculation.getBPM();
    bool is_stable = bpm_calculation.isStable();
    if (is_stable && current_bpm > 0.0f) {
        static float last_bpm_sent = 0.0f;
        if (current_bpm != last_bpm_sent) {
            output_controller.updateBPM((uint16_t)current_bpm);
            last_bpm_sent = current_bpm;
        }
    }
    
    // Service MQTT client
    if (mqtt_client) {
        mqtt_client->loop();
    }
    
    // WiFi disabled - skip periodic check
    // checkWiFiConnection();
    
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
