# TDD Cycle 6 - AC-AUDIO-006: Kick-Only Filtering Tests

cmake_minimum_required(VERSION 3.14)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include FetchContent module for GoogleTest
include(FetchContent)

# Fetch GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Source files for Audio Detection (AudioDetectionState is header-only)
set(AUDIO_SOURCES
    ../../src/audio/AudioDetection.cpp
)

# Include directories
set(AUDIO_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/audio
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/timing
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/events
    ${CMAKE_CURRENT_SOURCE_DIR}/../../test/mocks
)

# Test executable: Kick-Only Filtering
add_executable(test_kick_only_filtering
    test_kick_only_filtering.cpp
    ${AUDIO_SOURCES}
)

target_include_directories(test_kick_only_filtering PRIVATE
    ${AUDIO_INCLUDE_DIRS}
)

target_link_libraries(test_kick_only_filtering PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# Register test with CTest
add_test(NAME KickOnlyFilteringTests COMMAND test_kick_only_filtering)
