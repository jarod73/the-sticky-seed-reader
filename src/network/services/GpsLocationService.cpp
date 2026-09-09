#include "GpsLocationService.h"

#include <ArduinoJson.h>
#include <Logging.h>

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>

#include "CrossPointSettings.h"
#include "network/HttpDownloader.h"

namespace {
constexpr const char* IP_GEO_URL = "http://ip-api.com/json/?fields=status,country,city,lat,lon,timezone,offset";

// Fast hex char to nibble conversion
int8_t hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
}  // namespace

bool GpsLocationService::verifyNmeaChecksum(std::string_view sentence) {
  if (sentence.size() < 4) return false;
  if (sentence.front() != '$') return false;

  const size_t starPos = sentence.rfind('*');
  if (starPos == std::string_view::npos || starPos + 2 >= sentence.size()) return false;

  uint8_t calculated = 0;
  for (size_t i = 1; i < starPos; ++i) {
    calculated ^= static_cast<uint8_t>(sentence[i]);
  }

  const int8_t hi = hexNibble(sentence[starPos + 1]);
  const int8_t lo = hexNibble(sentence[starPos + 2]);
  if (hi < 0 || lo < 0) return false;

  const uint8_t expected = static_cast<uint8_t>((hi << 4) | lo);
  return calculated == expected;
}

bool GpsLocationService::parseNmeaCoordinate(std::string_view raw, char hemisphere, float& outDegrees) {
  if (raw.empty()) return false;

  // Find decimal point
  const size_t dotPos = raw.find('.');
  if (dotPos == std::string_view::npos || dotPos < 2) return false;

  // The 2 digits immediately preceding the dot are minutes
  const size_t degreeDigits = dotPos - 2;
  char degBuf[8] = {0};
  if (degreeDigits >= sizeof(degBuf)) return false;

  for (size_t i = 0; i < degreeDigits; ++i) degBuf[i] = raw[i];
  degBuf[degreeDigits] = '\0';

  char minBuf[16] = {0};
  const size_t minLen = raw.size() - degreeDigits;
  if (minLen >= sizeof(minBuf)) return false;

  for (size_t i = 0; i < minLen; ++i) minBuf[i] = raw[degreeDigits + i];
  minBuf[minLen] = '\0';

  const float degrees = std::strtof(degBuf, nullptr);
  const float minutes = std::strtof(minBuf, nullptr);
  float result = degrees + (minutes / 60.0f);

  if (hemisphere == 'S' || hemisphere == 's' || hemisphere == 'W' || hemisphere == 'w') {
    result = -result;
  }

  outDegrees = result;
  return true;
}

bool GpsLocationService::parseNmeaSentence(std::string_view sentence, LocationInfo& outInfo) {
  if (!verifyNmeaChecksum(sentence)) return false;

  // Extract tokens delimited by comma
  std::vector<std::string_view> tokens;
  tokens.reserve(16);

  size_t start = 0;
  while (start < sentence.size()) {
    size_t end = sentence.find(',', start);
    if (end == std::string_view::npos) {
      // Last token stops at '*' if present
      const size_t starPos = sentence.find('*', start);
      if (starPos != std::string_view::npos) {
        tokens.push_back(sentence.substr(start, starPos - start));
      } else {
        tokens.push_back(sentence.substr(start));
      }
      break;
    }
    tokens.push_back(sentence.substr(start, end - start));
    start = end + 1;
  }

  if (tokens.empty()) return false;

  const std::string_view tag = tokens[0];

  // Parse $GPRMC / $GNRMC: Recommended Minimum Navigation Information
  // Tokens: [0] $GPRMC, [1] hhmmss.ss, [2] Status (A=Active, V=Void),
  //         [3] Lat (ddmm.mm), [4] N/S, [5] Lon (dddmm.mm), [6] E/W, ...
  if (tag == "$GPRMC" || tag == "$GNRMC") {
    if (tokens.size() < 7) return false;
    if (tokens[2] != "A") return false;  // Void/no-fix

    float lat = 0.0f, lon = 0.0f;
    const char latHem = tokens[4].empty() ? 'N' : tokens[4][0];
    const char lonHem = tokens[6].empty() ? 'E' : tokens[6][0];

    if (parseNmeaCoordinate(tokens[3], latHem, lat) && parseNmeaCoordinate(tokens[5], lonHem, lon)) {
      outInfo.latitude = lat;
      outInfo.longitude = lon;
      outInfo.hasCoordinates = true;
      return true;
    }
  }

  // Parse $GPGGA / $GNGGA: Global Positioning System Fix Data
  // Tokens: [0] $GPGGA, [1] hhmmss.ss, [2] Lat, [3] N/S, [4] Lon, [5] E/W, [6] Fix Quality (0=invalid)
  if (tag == "$GPGGA" || tag == "$GNGGA") {
    if (tokens.size() < 7) return false;
    if (tokens[6] == "0") return false;  // No fix

    float lat = 0.0f, lon = 0.0f;
    const char latHem = tokens[3].empty() ? 'N' : tokens[3][0];
    const char lonHem = tokens[5].empty() ? 'E' : tokens[5][0];

    if (parseNmeaCoordinate(tokens[2], latHem, lat) && parseNmeaCoordinate(tokens[4], lonHem, lon)) {
      outInfo.latitude = lat;
      outInfo.longitude = lon;
      outInfo.hasCoordinates = true;
      return true;
    }
  }

  return false;
}

bool GpsLocationService::fetchIpLocation(LocationInfo& outInfo) {
  std::string responseJson;
  if (!HttpDownloader::fetchUrl(IP_GEO_URL, responseJson) || responseJson.empty()) {
    LOG_ERR("GPS", "Failed to fetch IP geolocation from %s", IP_GEO_URL);
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, responseJson);
  if (err) {
    LOG_ERR("GPS", "Failed to parse IP geolocation JSON: %s", err.c_str());
    return false;
  }

  const char* status = doc["status"] | "fail";
  if (std::strcmp(status, "success") != 0) {
    LOG_ERR("GPS", "IP geolocation returned error status");
    return false;
  }

  outInfo.latitude = doc["lat"] | 0.0f;
  outInfo.longitude = doc["lon"] | 0.0f;
  outInfo.city = doc["city"] | "";
  outInfo.country = doc["country"] | "";
  outInfo.timezone = doc["timezone"] | "";
  outInfo.utcOffsetSeconds = doc["offset"] | 0;
  outInfo.hasCoordinates = (outInfo.latitude != 0.0f || outInfo.longitude != 0.0f);

  LOG_INF("GPS", "Resolved location: %s, %s (%.4f, %.4f), TimeZone: %s (UTC%+d)", outInfo.city.c_str(),
          outInfo.country.c_str(), outInfo.latitude, outInfo.longitude, outInfo.timezone.c_str(),
          outInfo.utcOffsetSeconds / 3600);

  return true;
}

#include "network/CloudCredentialStore.h"

bool GpsLocationService::autoSyncLocationAndTime() {
  LocationInfo info;
  if (!fetchIpLocation(info) || !info.hasCoordinates) {
    return false;
  }

  // Update CLOUD_CREDENTIALS with resolved coordinates and city name
  CLOUD_CREDENTIALS.setWeatherCoordinates(info.latitude, info.longitude, info.city);
  CLOUD_CREDENTIALS.saveToFile();

  LOG_INF("GPS", "Updated location to %s (%.4f, %.4f)", CLOUD_CREDENTIALS.getWeatherCity().c_str(),
          CLOUD_CREDENTIALS.getWeatherLat(), CLOUD_CREDENTIALS.getWeatherLon());
  return true;
}
