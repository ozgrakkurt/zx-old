add_library(blib STATIC)

target_sources(blib
  PRIVATE
    blib_stack_trace.cpp
    blib_err.cpp
  PUBLIC
    FILE_SET HEADERS
    FILES
      blib_rand.hpp
      blib_int.hpp
      blib_hashmap.hpp
      blib_err.hpp
      blib_stack_trace.hpp
      blib_alloc.hpp
)

add_executable(blib_test test.cpp)
target_link_libraries(blib_test
  blib 
  GTest::gtest_main
)
gtest_discover_tests(blib_test)

