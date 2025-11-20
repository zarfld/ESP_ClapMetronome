cmake_minimum_required(VERSION 3.14)
project(OutputControllerTests CXX)

# ===== Compiler Configuration =====
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -O0 -ggdb)
endif()

# ===== Build Definitions =====
add_compile_definitions(NATIVE_BUILD UNIT_TEST)

# ===== Testing Framework =====
enable_testing()

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ===== Source Files =====
set(OUTPUT_SOURCES
    ../../src/output/OutputController.cpp
)

# ===== Test Executables =====

# Test 1: MIDI Beat Clock (AC-OUT-001, AC-OUT-002, AC-OUT-003, AC-OUT-004, AC-OUT-008, AC-OUT-010, AC-OUT-011)
add_executable(test_midi_beat_clock
    test_midi_beat_clock.cpp
    ${OUTPUT_SOURCES}
)

target_include_directories(test_midi_beat_clock PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_midi_beat_clock PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# Test 2: Network Transport (AC-OUT-003, AC-OUT-008) - OUT-02
add_executable(test_network_transport
    test_network_transport.cpp
    ${OUTPUT_SOURCES}
)

target_include_directories(test_network_transport PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_network_transport PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# Test 3: Timer-based Clock Sending (AC-OUT-007, AC-OUT-008, AC-OUT-009, AC-OUT-010) - OUT-03
add_executable(test_timer_clock
    test_timer_clock.cpp
    ${OUTPUT_SOURCES}
)

target_include_directories(test_timer_clock PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_timer_clock PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# Test 4: Relay Output (AC-OUT-005, AC-OUT-006, AC-OUT-012) - OUT-04
add_executable(test_relay_output
    test_relay_output.cpp
    ${OUTPUT_SOURCES}
)

target_include_directories(test_relay_output PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_relay_output PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# Test 5: Performance Validation (AC-OUT-007, AC-OUT-008, AC-OUT-009) - OUT-05
add_executable(test_performance
    test_performance.cpp
    ${OUTPUT_SOURCES}
)

target_include_directories(test_performance PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_performance PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# NOTE: test_midi_output.cpp is temporarily disabled (uses old MIDI note API)
# Will be updated or replaced in future cycles

# ===== Test Discovery =====
include(GoogleTest)
gtest_discover_tests(test_midi_beat_clock)
gtest_discover_tests(test_network_transport)
gtest_discover_tests(test_timer_clock)
gtest_discover_tests(test_relay_output)
gtest_discover_tests(test_performance)
