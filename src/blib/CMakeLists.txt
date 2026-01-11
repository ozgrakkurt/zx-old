add_library(blib INTERFACE)

target_sources(blib
  INTERFACE
    FILE_SET HEADERS
    FILES
      blib_rand.hpp
      blib_int.hpp
)

# add_executable(fastlanes_test test.cpp)
# target_link_libraries(fastlanes_test 
#   fastlanes
#   GTest::gtest_main
# )
# gtest_discover_tests(fastlanes_test)

