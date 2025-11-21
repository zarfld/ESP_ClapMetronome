# Master Test Suite Configuration
# ESP32 Clap Metronome - All Audio Tests
# Phase 05: Implementation & Verification

cmake_minimum_required(VERSION 3.14)
project(ESP_ClapMetronome_Tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GoogleTest setup (shared across all tests)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/release-1.12.1.zip
)

# Force shared CRT to match project
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Include GoogleTest module for test discovery
include(GoogleTest)

# Common include directories for all tests
set(COMMON_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Common source files
set(AUDIO_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/audio/AudioDetection.cpp
)

# Compiler options
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# AUDIO TEST SUITES (Wave 3.1)
# ============================================================================

# AUDIO-01: Adaptive Threshold (AC-AUDIO-001)
add_executable(test_adaptive_threshold
    test_audio/test_adaptive_threshold.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_adaptive_threshold PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_adaptive_threshold gtest gtest_main)
gtest_discover_tests(test_adaptive_threshold)

# State Machine Tests (AC-AUDIO-002)
add_executable(test_state_machine
    test_audio_state_machine/test_state_machine.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_state_machine PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_state_machine gtest gtest_main)
gtest_discover_tests(test_state_machine)

# Detection Accuracy Tests (AC-AUDIO-009)
add_executable(test_detection_accuracy
    test_audio_accuracy/test_detection_accuracy.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_detection_accuracy PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_detection_accuracy gtest gtest_main)
gtest_discover_tests(test_detection_accuracy)

# Beat Event Emission Tests (AC-AUDIO-004)
add_executable(test_beat_event_emission
    test_audio_beat_events/test_beat_event_emission.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_beat_event_emission PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_beat_event_emission gtest gtest_main)
gtest_discover_tests(test_beat_event_emission)

# AGC Transitions Tests (AC-AUDIO-003)
add_executable(test_agc_transitions
    test_audio_agc_transitions/test_agc_transitions.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_agc_transitions PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_agc_transitions gtest gtest_main)
gtest_discover_tests(test_agc_transitions)

# Debounce Tests (AC-AUDIO-005)
add_executable(test_debounce
    test_audio_debounce/test_debounce_period.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_debounce PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_debounce gtest gtest_main)
gtest_discover_tests(test_debounce)

# Kick Filtering Tests (AC-AUDIO-006)
add_executable(test_kick_filtering
    test_audio_kick_filtering/test_kick_only_filtering.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_kick_filtering PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_kick_filtering gtest gtest_main)
gtest_discover_tests(test_kick_filtering)

# Telemetry Tests (AC-AUDIO-007)
add_executable(test_telemetry
    test_audio_telemetry/test_telemetry_updates.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_telemetry PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_telemetry gtest gtest_main)
gtest_discover_tests(test_telemetry)

# Latency Tests (AC-AUDIO-008)
add_executable(test_latency
    test_audio_latency/test_audio_latency.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_latency PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_latency gtest gtest_main)
gtest_discover_tests(test_latency)

# CPU Usage Tests (AC-AUDIO-010)
add_executable(test_cpu_usage
    test_audio_cpu_usage/test_cpu_usage.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_cpu_usage PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_cpu_usage gtest gtest_main)
gtest_discover_tests(test_cpu_usage)

# Memory Tests (AC-AUDIO-011)
add_executable(test_memory
    test_audio_memory/test_memory_usage.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_memory PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_memory gtest gtest_main)
gtest_discover_tests(test_memory)

# ============================================================================
# INTEGRATION TESTS
# ============================================================================

# Clipping Integration Tests
add_executable(test_clipping
    test_audio_clipping/test_clipping_integration.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_clipping PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_clipping gtest gtest_main)
gtest_discover_tests(test_clipping)

# Noise Integration Tests
add_executable(test_noise
    test_audio_noise/test_noise_rejection.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_noise PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_noise gtest gtest_main)
gtest_discover_tests(test_noise)

# Window Synchronization Tests
add_executable(test_window_sync
    test_audio_window_sync/test_window_synchronization.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_window_sync PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_window_sync gtest gtest_main)
gtest_discover_tests(test_window_sync)

# Legacy Adaptive Threshold Tests (for comparison)
add_executable(test_adaptive_threshold_legacy
    test_audio_adaptive_threshold/test_adaptive_threshold.cpp
    ${AUDIO_SOURCES}
)
target_include_directories(test_adaptive_threshold_legacy PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_adaptive_threshold_legacy gtest gtest_main)
gtest_discover_tests(test_adaptive_threshold_legacy)

# ============================================================================
# BPM TEST SUITES (Wave 3.2)
# ============================================================================

# BPM sources
set(BPM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/bpm/BPMCalculation.cpp
)

# Test 1: Basic Tap Addition (AC-BPM-001, AC-BPM-002)
add_executable(test_bpm_basic_taps
    test_bpm/test_basic_taps.cpp
    ${BPM_SOURCES}
)
target_include_directories(test_bpm_basic_taps PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_bpm_basic_taps gtest gtest_main)
target_compile_definitions(test_bpm_basic_taps PRIVATE NATIVE_BUILD UNIT_TEST)
gtest_discover_tests(test_bpm_basic_taps)

# Test 2: Circular Buffer & Wrap-around (AC-BPM-001)
add_executable(test_bpm_circular_buffer
    test_bpm/test_circular_buffer.cpp
    ${BPM_SOURCES}
)
target_include_directories(test_bpm_circular_buffer PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_bpm_circular_buffer gtest gtest_main)
target_compile_definitions(test_bpm_circular_buffer PRIVATE NATIVE_BUILD UNIT_TEST)
gtest_discover_tests(test_bpm_circular_buffer)

# Test 3: Stability Detection (AC-BPM-003)
add_executable(test_bpm_stability
    test_bpm/test_stability.cpp
    ${BPM_SOURCES}
)
target_include_directories(test_bpm_stability PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_bpm_stability gtest gtest_main)
target_compile_definitions(test_bpm_stability PRIVATE NATIVE_BUILD UNIT_TEST)
gtest_discover_tests(test_bpm_stability)

# Test 4: Half/Double Tempo Correction (AC-BPM-004, AC-BPM-005)
add_executable(test_bpm_tempo_correction
    test_bpm/test_tempo_correction.cpp
    ${BPM_SOURCES}
)
target_include_directories(test_bpm_tempo_correction PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_bpm_tempo_correction gtest gtest_main)
target_compile_definitions(test_bpm_tempo_correction PRIVATE NATIVE_BUILD UNIT_TEST)
gtest_discover_tests(test_bpm_tempo_correction)

# Test 5: Invalid Interval Filtering (AC-BPM-012, AC-BPM-013)
add_executable(test_bpm_interval_validation
    test_bpm/test_interval_validation.cpp
    ${BPM_SOURCES}
)
target_include_directories(test_bpm_interval_validation PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_bpm_interval_validation gtest gtest_main)
target_compile_definitions(test_bpm_interval_validation PRIVATE NATIVE_BUILD UNIT_TEST)
gtest_discover_tests(test_bpm_interval_validation)

# Test 6: Clear/Reset (AC-BPM-007)
add_executable(test_bpm_clear_reset
    test_bpm/test_clear_reset.cpp
    ${BPM_SOURCES}
)
target_include_directories(test_bpm_clear_reset PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_bpm_clear_reset gtest gtest_main)
target_compile_definitions(test_bpm_clear_reset PRIVATE NATIVE_BUILD UNIT_TEST)
gtest_discover_tests(test_bpm_clear_reset)

# Test 7: Callback Notifications (AC-BPM-008)
add_executable(test_bpm_callback_notifications
    test_bpm/test_callback_notifications.cpp
    ${BPM_SOURCES}
)
target_include_directories(test_bpm_callback_notifications PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_bpm_callback_notifications gtest gtest_main)
target_compile_definitions(test_bpm_callback_notifications PRIVATE NATIVE_BUILD UNIT_TEST)
gtest_discover_tests(test_bpm_callback_notifications)

# ============================================================================
# CONFIGURATION MANAGER TEST SUITES (Wave 3.4)
# ============================================================================

# Config sources
set(CONFIG_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/config/ConfigurationManager.cpp
)

# Test 1: Factory Default Values (AC-CFG-002)
add_executable(test_config_default_values
    test_config/test_default_values.cpp
    ${CONFIG_SOURCES}
)
target_include_directories(test_config_default_values PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_config_default_values gtest gtest_main)
target_compile_definitions(test_config_default_values PRIVATE NATIVE_BUILD UNIT_TEST)
gtest_discover_tests(test_config_default_values)

# Test 2: Range Validation (AC-CFG-004)
add_executable(test_config_range_validation
    test_config/test_range_validation.cpp
    ${CONFIG_SOURCES}
)
target_include_directories(test_config_range_validation PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_config_range_validation gtest gtest_main)
target_compile_definitions(test_config_range_validation PRIVATE NATIVE_BUILD UNIT_TEST)
gtest_discover_tests(test_config_range_validation)

# Test 3: Change Notifications (AC-CFG-008)
add_executable(test_config_change_notifications
    test_config/test_change_notifications.cpp
    ${CONFIG_SOURCES}
)
target_include_directories(test_config_change_notifications PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_config_change_notifications gtest gtest_main)
target_compile_definitions(test_config_change_notifications PRIVATE NATIVE_BUILD UNIT_TEST)
gtest_discover_tests(test_config_change_notifications)

# Test 4: Persistence (AC-CFG-001, AC-CFG-003)
add_executable(test_config_persistence
    test_config/test_persistence.cpp
    ${CONFIG_SOURCES}
)
target_include_directories(test_config_persistence PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_config_persistence gtest gtest_main)
target_compile_definitions(test_config_persistence PRIVATE NATIVE_BUILD UNIT_TEST)
gtest_discover_tests(test_config_persistence)

# ============================================================================
# CUSTOM TARGETS FOR CONVENIENT EXECUTION
# ============================================================================

# Run all audio tests
add_custom_target(test_audio_all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "test_.*" -C $<CONFIG>
    DEPENDS 
        test_adaptive_threshold
        test_state_machine
        test_detection_accuracy
        test_beat_event_emission
        test_agc_transitions
        test_debounce
        test_kick_filtering
        test_telemetry
        test_latency
        test_cpu_usage
        test_memory
        test_clipping
        test_noise
        test_window_sync
        test_adaptive_threshold_legacy
    COMMENT "Running all audio test suites..."
)

# Run only verified tests (29 tests that we've fixed)
add_custom_target(test_audio_verified
    COMMAND test_adaptive_threshold
    COMMAND test_state_machine
    COMMAND test_detection_accuracy
    COMMAND test_beat_event_emission
    DEPENDS
        test_adaptive_threshold
        test_state_machine
        test_detection_accuracy
        test_beat_event_emission
    COMMENT "Running verified audio tests (29 tests)..."
)

# Run remaining untested suites
add_custom_target(test_audio_remaining
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R "test_(agc|debounce|kick|telemetry|latency|cpu|memory|clipping|noise|window)" -C $<CONFIG>
    DEPENDS
        test_agc_transitions
        test_debounce
        test_kick_filtering
        test_telemetry
        test_latency
        test_cpu_usage
        test_memory
        test_clipping
        test_noise
        test_window_sync
    COMMENT "Running remaining untested audio suites..."
)
