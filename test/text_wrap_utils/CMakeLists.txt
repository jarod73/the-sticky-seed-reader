add_executable(TextWrapUtilsTest
  TextWrapUtilsTest.cpp
  ${REPO_ROOT}/src/util/TextWrapUtils.cpp
)

target_include_directories(TextWrapUtilsTest PRIVATE
  ${REPO_ROOT}/src
  ${REPO_ROOT}/src/util
  ${REPO_ROOT}/lib/GfxRenderer
  ${REPO_ROOT}/lib/EpdFont
)

target_link_libraries(TextWrapUtilsTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(TextWrapUtilsTest)
