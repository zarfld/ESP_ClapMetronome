cmake_minimum_required(VERSION 3.14)
project(AdaptiveThresholdTests CXX)

# C++17 Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -O0 -ggdb)
endif()

# Native build flag
add_compile_definitions(NATIVE_BUILD UNIT_TEST)

# Enable testing
enable_testing()

# GoogleTest dependency
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Source files under test
set(AUDIO_SOURCES
    ../../src/audio/AudioDetection.cpp
)

# Test executable
add_executable(test_adaptive_threshold
    test_adaptive_threshold.cpp
    ${AUDIO_SOURCES}
)

# Include directories
target_include_directories(test_adaptive_threshold PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
)

# Link GoogleTest
target_link_libraries(test_adaptive_threshold PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# Add test to CTest
include(GoogleTest)
gtest_discover_tests(test_adaptive_threshold)
