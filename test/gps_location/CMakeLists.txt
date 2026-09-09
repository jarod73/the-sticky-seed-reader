add_executable(GpsLocationTest
  GpsLocationTest.cpp
  ${REPO_ROOT}/src/network/services/GpsLocationService.cpp
  ${REPO_ROOT}/src/CrossPointSettings.cpp
  ${REPO_ROOT}/src/ReaderFontSizes.cpp
  ${REPO_ROOT}/src/SettingsList.cpp
)

target_include_directories(GpsLocationTest PRIVATE
  ${REPO_ROOT}/src
  ${REPO_ROOT}/src/network/services
  ${REPO_ROOT}/lib/hal
  ${REPO_ROOT}/lib/I18n
  ${REPO_ROOT}/lib/Memory
  ${REPO_ROOT}/lib/EpdFont
  ${REPO_ROOT}/lib/Obfuscation
)

target_link_libraries(GpsLocationTest PRIVATE
  crosspoint_test_common
  GTest::gtest_main
)

gtest_discover_tests(GpsLocationTest)
