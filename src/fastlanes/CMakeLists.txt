add_library(fastlanes STATIC)

target_sources(fastlanes
  PRIVATE
    fl_bit_pack.cpp
    fl_bit_unpack.cpp
    fl_for_pack.cpp
    fl_for_unpack.cpp
    fl_delta.cpp
    fl_bit_unpack_delta_decode.cpp
    fl_rle.cpp
  INTERFACE
    FILE_SET HEADERS
    FILES
      fastlanes.hpp
)

add_executable(fastlanes_test test.cpp)
target_link_libraries(fastlanes_test 
  fastlanes
  GTest::gtest_main
)
gtest_discover_tests(fastlanes_test)
