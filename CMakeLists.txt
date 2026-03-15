cmake_minimum_required(VERSION 3.16)
project(CodebookRuntimeCpp-Config CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Header-only library target
add_library(CodebookRuntimeConfig INTERFACE)
target_include_directories(CodebookRuntimeConfig INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Tests
enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(test_Value tests/test_Value.cpp)
target_link_libraries(test_Value
    PRIVATE CodebookRuntimeConfig GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_Value)
