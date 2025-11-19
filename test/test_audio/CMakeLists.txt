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

# ===== Future Test Executables (to be added in subsequent TDD cycles) =====
# Test Cycle 2: State Machine Tests
# add_executable(test_state_machine ...)
# Test Cycle 3: AGC Tests
# add_executable(test_agc ...)
# Test Cycle 4: Beat Event Tests
# add_executable(test_beat_events ...)
# Test Cycle 5: Debounce Tests
# add_executable(test_debounce ...)
# Test Cycle 6: Kick Detection Tests
# add_executable(test_kick_detection ...)

# Build output
message(STATUS "Audio Detection Tests Configuration:")
message(STATUS "  - C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  - Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  - Test Executables: test_adaptive_threshold")
message(STATUS "  - GoogleTest: v1.14.0")
