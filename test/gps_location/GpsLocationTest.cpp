#include <gtest/gtest.h>

#include "network/services/GpsLocationService.h"

TEST(GpsLocationTest, NmeaChecksumValid) {
  // $GPRMC with checksum *47
  std::string_view valid = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
  EXPECT_TRUE(GpsLocationService::verifyNmeaChecksum(valid));
}

TEST(GpsLocationTest, NmeaChecksumInvalid) {
  std::string_view invalid = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*00";
  EXPECT_FALSE(GpsLocationService::verifyNmeaChecksum(invalid));
}

TEST(GpsLocationTest, ParseNmeaCoordinateLatitude) {
  // 4807.038 N -> 48 degrees + 07.038 / 60 minutes = 48.1173 degrees
  float deg = 0.0f;
  EXPECT_TRUE(GpsLocationService::parseNmeaCoordinate("4807.038", 'N', deg));
  EXPECT_NEAR(deg, 48.1173f, 0.001f);

  // South hemisphere -> negative
  EXPECT_TRUE(GpsLocationService::parseNmeaCoordinate("4807.038", 'S', deg));
  EXPECT_NEAR(deg, -48.1173f, 0.001f);
}

TEST(GpsLocationTest, ParseNmeaCoordinateLongitude) {
  // 01131.000 E -> 11 degrees + 31.000 / 60 minutes = 11.51667 degrees
  float deg = 0.0f;
  EXPECT_TRUE(GpsLocationService::parseNmeaCoordinate("01131.000", 'E', deg));
  EXPECT_NEAR(deg, 11.51667f, 0.001f);

  // West hemisphere -> negative
  EXPECT_TRUE(GpsLocationService::parseNmeaCoordinate("01131.000", 'W', deg));
  EXPECT_NEAR(deg, -11.51667f, 0.001f);
}

TEST(GpsLocationTest, ParseNmeaSentenceGPRMC) {
  GpsLocationService::LocationInfo info;
  std::string_view sentence = "$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A";
  EXPECT_TRUE(GpsLocationService::parseNmeaSentence(sentence, info));
  EXPECT_TRUE(info.hasCoordinates);
  EXPECT_NEAR(info.latitude, 48.1173f, 0.001f);
  EXPECT_NEAR(info.longitude, 11.51667f, 0.001f);
}
