add_library(intcompr STATIC)

target_sources(intcompr
  PRIVATE
    ic_impl.cpp
    ic_bitpack.cpp
    ic_forpack.cpp
    ic_deltapack.cpp
    ic_zigzag.cpp
    intcompr.cpp
  INTERFACE
    FILE_SET HEADERS
    FILES
      intcompr.hpp
)
target_link_libraries(intcompr
  fastlanes
  blib
)

add_executable(intcompr_test test.cpp)
target_link_libraries(intcompr_test
  intcompr 
  blib
  GTest::gtest_main
)
gtest_discover_tests(intcompr_test)

add_executable(intcompr_bench bench.cpp)
target_link_libraries(intcompr_bench
  intcompr
  blib
  benchmark::benchmark
)
