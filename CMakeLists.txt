cmake_minimum_required(VERSION 3.14)

project(kit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wconversion -Wsign-conversion -march=native)

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
set(GTEST_HAS_ABSL OFF)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG "v1.17.0"
)
FetchContent_MakeAvailable(googletest)

set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_Declare(
  benchmark 
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG "v1.9.4"
)
FetchContent_MakeAvailable(benchmark)

enable_testing()
include(GoogleTest)

add_subdirectory(./src/blib)
add_subdirectory(./src/fastlanes)
add_subdirectory(./src/intcompr)
