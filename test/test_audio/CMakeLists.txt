# TDD Cycle AUDIO-01: Adaptive Threshold Tests
# Component: DES-C-001 Audio Detection Engine
# Build Configuration

cmake_minimum_required(VERSION 3.14)
project(test_audio)

# GoogleTest setup
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

# Test executable: Adaptive Threshold
add_executable(test_adaptive_threshold
    test_adaptive_threshold.cpp
    ../../src/audio/AudioDetection.cpp
)

# Include directories
target_include_directories(test_adaptive_threshold PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

# Link GoogleTest
target_link_libraries(test_adaptive_threshold
    gtest
    gtest_main
)

# Register test with CTest
include(GoogleTest)
gtest_discover_tests(test_adaptive_threshold)

# Compiler options
if(MSVC)
    target_compile_options(test_adaptive_threshold PRIVATE /W4)
else()
    target_compile_options(test_adaptive_threshold PRIVATE -Wall -Wextra -Wpedantic)
endif()
