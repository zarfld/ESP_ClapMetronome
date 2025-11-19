# CMakeLists.txt for DES-C-001 Audio Detection Engine Tests
# Implements: #45 (Audio Detection), #51 (TDD Plan)
# Standards: ISO/IEC/IEEE 12207:2017 (Build Configuration)
# TDD Status: RED phase (build configuration for test compilation)

cmake_minimum_required(VERSION 3.14)
project(AudioDetectionTests CXX)

# C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags - strict warnings
if(MSVC)
    add_compile_options(/W4 /WX)  # Warning level 4, warnings as errors
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# Enable testing
enable_testing()

# GoogleTest dependency (FetchContent like Wave 1)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
# For Windows: Prevent overriding parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Native build flag (not running on ESP32)
add_compile_definitions(NATIVE_BUILD)

# Production code under test
set(AUDIO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../src")

set(AUDIO_SOURCES
    ${AUDIO_SRC_DIR}/audio/AudioDetection.cpp
    # Note: Header-only files don't need to be listed
    # - AudioSampleBuffer.h (header-only)
    # - AudioDetectionState.h (header-only)
)

# Include directories
set(AUDIO_INCLUDE_DIRS
    ${AUDIO_SRC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
)

# ===== Test Executable 1: Adaptive Threshold Tests =====
add_executable(test_adaptive_threshold
    test_adaptive_threshold.cpp
    ${AUDIO_SOURCES}
)

target_include_directories(test_adaptive_threshold PRIVATE
    ${AUDIO_INCLUDE_DIRS}
)

target_link_libraries(test_adaptive_threshold PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# Register with CTest
add_test(NAME AdaptiveThresholdTests COMMAND test_adaptive_threshold)

# ===== Test Executable 2: State Machine Tests =====
add_executable(test_state_machine
    test_state_machine.cpp
    ${AUDIO_SOURCES}
)

target_include_directories(test_state_machine PRIVATE
    ${AUDIO_INCLUDE_DIRS}
)

target_link_libraries(test_state_machine PRIVATE
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME StateMachineTests COMMAND test_state_machine)

# ===== Test Executable 3: AGC Transitions Tests =====
add_executable(test_agc_transitions
    test_agc_transitions.cpp
    ${AUDIO_SOURCES}
)

target_include_directories(test_agc_transitions PRIVATE
    ${AUDIO_INCLUDE_DIRS}
)

target_link_libraries(test_agc_transitions PRIVATE
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME AGCTransitionsTests COMMAND test_agc_transitions)

# ===== Test Executable 4: Beat Event Emission Tests =====
add_executable(test_beat_event_emission
    test_beat_event_emission.cpp
    ${AUDIO_SOURCES}
)

target_include_directories(test_beat_event_emission PRIVATE
    ${AUDIO_INCLUDE_DIRS}
)

target_link_libraries(test_beat_event_emission PRIVATE
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME BeatEventEmissionTests COMMAND test_beat_event_emission)

# ===== Test Executable 5: Debounce Period Tests =====
add_executable(test_debounce_period
    test_debounce_period.cpp
    ${AUDIO_SOURCES}
)

target_include_directories(test_debounce_period PRIVATE
    ${AUDIO_INCLUDE_DIRS}
)

target_link_libraries(test_debounce_period PRIVATE
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME DebouncePeriodTests COMMAND test_debounce_period)

# ===== Test Executable 6: Telemetry Updates Tests =====
add_executable(test_telemetry_updates
    test_telemetry_updates.cpp
    ${AUDIO_SOURCES}
)

target_include_directories(test_telemetry_updates PRIVATE
    ${AUDIO_INCLUDE_DIRS}
)

target_link_libraries(test_telemetry_updates PRIVATE
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME TelemetryUpdatesTests COMMAND test_telemetry_updates)

# ===== Test Executable 7: Audio Latency Tests =====
add_executable(test_audio_latency
    test_audio_latency.cpp
    ${AUDIO_SOURCES}
)

target_include_directories(test_audio_latency PRIVATE
    ${AUDIO_INCLUDE_DIRS}
)

target_link_libraries(test_audio_latency PRIVATE
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME AudioLatencyTests COMMAND test_audio_latency)

# ===== Test Executable 8: Detection Accuracy Tests =====
add_executable(test_detection_accuracy
    test_detection_accuracy.cpp
    ${AUDIO_SOURCES}
)

target_include_directories(test_detection_accuracy PRIVATE
    ${AUDIO_INCLUDE_DIRS}
)

target_link_libraries(test_detection_accuracy PRIVATE
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME DetectionAccuracyTests COMMAND test_detection_accuracy)

# ===== Test Executable 9: CPU Usage Tests =====
add_executable(test_cpu_usage
    test_cpu_usage.cpp
    ${AUDIO_SOURCES}
)

target_include_directories(test_cpu_usage PRIVATE
    ${AUDIO_INCLUDE_DIRS}
)

target_link_libraries(test_cpu_usage PRIVATE
    GTest::gtest
    GTest::gtest_main
)

add_test(NAME CPUUsageTests COMMAND test_cpu_usage)

# ===== Future Test Executables (to be added in subsequent TDD cycles) =====
# Test Cycle 6: Kick Detection Tests
# add_executable(test_kick_detection ...)
# Test Cycle 11: Memory Usage Tests
# add_executable(test_memory_usage ...)

# Build output
message(STATUS "Audio Detection Tests Configuration:")
message(STATUS "  - C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  - Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  - Test Executables: 9 test suites (adaptive_threshold, state_machine, agc_transitions, beat_event_emission, debounce_period, telemetry_updates, audio_latency, detection_accuracy, cpu_usage)")
message(STATUS "  - GoogleTest: v1.14.0")
