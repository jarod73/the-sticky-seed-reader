#include "OpenMeteoClient.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Logging.h>

#include <cstdio>

#include "network/HttpDownloader.h"

const char* OpenMeteoClient::getWeatherCodeDescription(int code) {
  switch (code) {
    case 0:
      return "Clear / Sunny";
    case 1:
      return "Mainly Clear";
    case 2:
      return "Partly Cloudy";
    case 3:
      return "Overcast";
    case 45:
    case 48:
      return "Foggy";
    case 51:
    case 53:
    case 55:
      return "Drizzle";
    case 61:
    case 63:
    case 65:
      return "Rain";
    case 71:
    case 73:
    case 75:
      return "Snow";
    case 80:
    case 81:
    case 82:
      return "Rain Showers";
    case 85:
    case 86:
      return "Snow Showers";
    case 95:
    case 96:
    case 99:
      return "Thunderstorm";
    default:
      return "Fair";
  }
}

bool OpenMeteoClient::fetchForecast(float lat, float lon, WeatherForecast& outForecast) {
  char urlBuf[256];
  snprintf(urlBuf, sizeof(urlBuf),
           "https://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f"
           "&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m"
           "&daily=weather_code,temperature_2m_max,temperature_2m_min&timezone=auto",
           lat, lon);

  LOG_INF("METEO", "Fetching weather from %s", urlBuf);

  std::string json;
  if (!HttpDownloader::fetchUrl(urlBuf, json) || json.empty()) {
    LOG_ERR("METEO", "Failed to fetch weather JSON");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    LOG_ERR("METEO", "JSON parse error: %s", err.c_str());
    return false;
  }

  JsonObject current = doc["current"];
  if (!current.isNull()) {
    outForecast.currentTempC = current["temperature_2m"] | 20.0f;
    outForecast.currentHumidity = current["relative_humidity_2m"] | 50;
    outForecast.windSpeedKmh = current["wind_speed_10m"] | 0.0f;
    outForecast.weatherCode = current["weather_code"] | 0;
    outForecast.conditionText = getWeatherCodeDescription(outForecast.weatherCode);
  }

  JsonObject daily = doc["daily"];
  if (!daily.isNull()) {
    JsonArray maxTemps = daily["temperature_2m_max"];
    JsonArray minTemps = daily["temperature_2m_min"];
    if (maxTemps.size() > 0) outForecast.tempMaxC = maxTemps[0] | outForecast.currentTempC;
    if (minTemps.size() > 0) outForecast.tempMinC = minTemps[0] | outForecast.currentTempC;
  }

  outForecast.valid = true;
  return true;
}
