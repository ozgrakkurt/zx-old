cmake_minimum_required(VERSION 3.14)

project(kit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(KIT_MARCH_NATIVE "compile for native architecture" ON)

if (KIT_MARCH_NATIVE)
  add_compile_options(-march=native)
endif()

add_compile_options(-Wall -Wextra)

include(FetchContent)

set(BUILD_GMOCK OFF)
set(INSTALL_GTEST OFF)
set(GTEST_HAS_ABSL OFF)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG 52eb8108c5bdec04579160ae17225d66034bd723
)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

add_subdirectory(./src/blib)
add_subdirectory(./src/fastlanes)
add_subdirectory(./src/intcompr)

add_executable(hello
  main.cpp
)

target_link_libraries(hello
  fastlanes
)
