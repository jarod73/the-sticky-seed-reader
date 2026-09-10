add_executable(ReaderUtilsTest
  ReaderUtilsTest.cpp
)

target_include_directories(ReaderUtilsTest PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${REPO_ROOT}/src
  ${REPO_ROOT}/src/activities/reader
)

target_link_libraries(ReaderUtilsTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(ReaderUtilsTest)
