cmake_minimum_required(VERSION 3.14)
project(TimingManagerTests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -O0 -ggdb)
endif()

# Define NATIVE_BUILD for test compilation
add_definitions(-DNATIVE_BUILD -DUNIT_TEST)

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.14.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Source files under test
set(SOURCE_FILES
    ../../src/timing/TimingManager.cpp
)

# Test files
set(TEST_FILES
    test_timestamp_query.cpp
)

# Create test executable
add_executable(test_timestamp_query
    ${TEST_FILES}
    ${SOURCE_FILES}
)

# Include directories
target_include_directories(test_timestamp_query PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

# Link GoogleTest
target_link_libraries(test_timestamp_query
    gtest
    gtest_main
)

# Add test to CTest
include(GoogleTest)
gtest_discover_tests(test_timestamp_query)

# Test 2: RTC Health Status (TDD Cycle 2)
add_executable(test_rtc_health
    test_rtc_health.cpp
    ${SOURCE_FILES}
)

target_include_directories(test_rtc_health PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

target_link_libraries(test_rtc_health
    gtest
    gtest_main
)

gtest_discover_tests(test_rtc_health)

# Test 3: Time Synchronization (TDD Cycle 3)
add_executable(test_time_sync
    test_time_sync.cpp
    ${SOURCE_FILES}
)

target_include_directories(test_time_sync PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

target_link_libraries(test_time_sync
    gtest
    gtest_main
)

gtest_discover_tests(test_time_sync)

# Note: Combined executable not feasible due to GoogleTest's internal main() registration
# Each test file uses TEST_F which registers with gtest_main's main() function
# Solution: Use run_all_tests.ps1 script to execute all three test executables sequentially
