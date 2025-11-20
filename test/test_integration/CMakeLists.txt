# Integration Tests - BPM + Output
project(BPMOutputIntegrationTests CXX)

cmake_minimum_required(VERSION 3.14)

# Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Native build flag
add_definitions(-DNATIVE_BUILD)

# Test executable
add_executable(test_bpm_output
    test_bpm_output.cpp
    ../../src/bpm/BPMCalculation.cpp
    ../../src/output/OutputController.cpp
    ../../src/integration/BPMOutputBridge.cpp
)

target_include_directories(test_bpm_output PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
)

target_link_libraries(test_bpm_output
    GTest::gtest
    GTest::gtest_main
)

# Discover tests
include(GoogleTest)
gtest_discover_tests(test_bpm_output)
