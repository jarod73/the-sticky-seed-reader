#pragma once

#include <string>

/**
 * Live weather forecast dataset fetched from Open-Meteo (zero API key required).
 */
struct WeatherForecast {
  float currentTempC = 0.0f;  ///< Instantaneous 2-meter air temperature in Celsius
  int currentHumidity = 0;    ///< Relative humidity percentage (0-100%)
  float windSpeedKmh = 0.0f;  ///< Wind speed in kilometers per hour
  int weatherCode = 0;        ///< WMO Weather interpretation code (0-99)
  std::string conditionText;  ///< Human-readable weather condition text (e.g. "Partly Cloudy")
  float tempMaxC = 0.0f;      ///< Daily maximum temperature in Celsius
  float tempMinC = 0.0f;      ///< Daily minimum temperature in Celsius
  bool valid = false;         ///< True if data was successfully fetched and decoded
};

/**
 * OpenMeteoClient
 *
 * Open-Meteo weather integration for ambient intelligence screens.
 * Features:
 * - Free, public weather API (no registration or API key required).
 * - Automatic WMO (World Meteorological Organization) weather condition decoding.
 * - Daily high/low and current temperature & humidity telemetry.
 */
class OpenMeteoClient {
 public:
  /**
   * Fetches weather forecast for given latitude and longitude coordinates.
   *
   * @param lat Latitude in decimal degrees (e.g. 37.7749).
   * @param lon Longitude in decimal degrees (e.g. -122.4194).
   * @param outForecast Output forecast structure.
   * @return True if fetch and JSON parsing succeeded.
   */
  static bool fetchForecast(float lat, float lon, WeatherForecast& outForecast);

  /**
   * Translates a WMO weather code integer into a human-readable condition description.
   */
  static const char* getWeatherCodeDescription(int code);
};
