#pragma once

#include <cstdint>
#include <string>
#include <string_view>

/**
 * @brief Geolocation and GNSS synchronization service for reTerminal Sticky.
 *
 * Provides dual location acquisition:
 * 1. IP Geolocation over Wi-Fi (zero hardware needed): queries geolocation API
 *    for city name, decimal coordinates, and IANA time zone / UTC offset.
 * 2. NMEA 0183 GNSS Stream (hardware GPS module): parses $GPRMC and $GPGGA sentences
 *    from an external GNSS receiver over UART for live latitude, longitude, and UTC time.
 */
class GpsLocationService {
 public:
  struct LocationInfo {
    float latitude = 0.0f;
    float longitude = 0.0f;
    std::string city;
    std::string country;
    std::string timezone;
    int32_t utcOffsetSeconds = 0;
    uint32_t unixTime = 0;
    bool hasCoordinates = false;
    bool hasTime = false;
  };

  /**
   * @brief Queries IP geolocation service over Wi-Fi.
   * @param outInfo Populated with resolved location, coordinates, and timezone offset.
   * @return true on successful network fetch and JSON parsing.
   */
  static bool fetchIpLocation(LocationInfo& outInfo);

  /**
   * @brief Parses a single NMEA 0183 sentence ($GPRMC or $GPGGA).
   * @param sentence Raw NMEA sentence string view.
   * @param outInfo LocationInfo structure updated with extracted coordinates/time.
   * @return true if sentence was valid, checksum matched, and data was extracted.
   */
  static bool parseNmeaSentence(std::string_view sentence, LocationInfo& outInfo);

  /**
   * @brief Automatically updates CrossPointSettings coordinates and synchronizes
   *        the PCF8563 RTC / system clock with current location timezone.
   * @return true if location was acquired and settings updated.
   */
  static bool autoSyncLocationAndTime();

  /**
   * @brief Converts NMEA coordinate format (ddmm.mmmm or dddmm.mmmm) to decimal degrees.
   * @param raw Raw NMEA coordinate string (e.g. "4807.038").
   * @param hemisphere Cardinal direction indicator ('N', 'S', 'E', 'W').
   * @param outDegrees Decimal degrees output.
   * @return true on valid conversion.
   */
  static bool parseNmeaCoordinate(std::string_view raw, char hemisphere, float& outDegrees);

  /**
   * @brief Verifies the XOR checksum of an NMEA 0183 sentence.
   * @param sentence Sentence beginning with '$' and containing '*hh'.
   * @return true if checksum is valid.
   */
  static bool verifyNmeaChecksum(std::string_view sentence);
};
