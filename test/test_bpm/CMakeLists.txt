cmake_minimum_required(VERSION 3.14)
project(BPMCalculationBasicTests CXX)

# ===== Compiler Configuration =====
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX)  # Warning level 4, treat warnings as errors
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
set(BPM_SOURCES
    ../../src/bpm/BPMCalculation.cpp
)

# ===== Test Executable =====
add_executable(test_basic_taps
    test_basic_taps.cpp
    ${BPM_SOURCES}
)

# ===== Include Directories =====
target_include_directories(test_basic_taps PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..       # Root (for absolute includes)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src   # Source root
    ${CMAKE_CURRENT_SOURCE_DIR}/../mocks    # Mock interfaces
)

# ===== Link Libraries =====
target_link_libraries(test_basic_taps PRIVATE
    GTest::gtest
    GTest::gtest_main
)

# ===== Test Discovery =====
include(GoogleTest)
gtest_discover_tests(test_basic_taps)
