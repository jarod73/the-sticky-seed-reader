#pragma once

#include <ArduinoJson.h>
#include <PersistableStore.h>

#include <string>

/**
 * CloudCredentialStore
 *
 * Centralized, non-volatile configuration store for all connected cloud integrations
 * and ambient intelligence feeds on Seeed Studio reTerminal Sticky.
 *
 * Dual-Storage Resilience Architecture:
 * 1. Primary: JSON file stored on external SD card at `/.crosspoint/cloud_credentials.json`.
 * 2. Secondary: Non-Volatile Storage (NVS) flash partition (`cp_cloud` namespace).
 *
 * Whenever settings are saved, they are mirrored to onboard NVS. If the device boots
 * without an SD card, or if the SD card is swapped or reformatted, credentials
 * are automatically restored from NVS, ensuring uninterrupted cloud services.
 *
 * Integrations Managed:
 * - Notion Integration Token & Target Database ID
 * - Readwise Access Token
 * - Todoist API Token
 * - Google Gemini API Key
 * - Wallabag & WebDAV Sync Credentials
 * - RSS News Feed URL
 * - Weather Coordinates (Latitude, Longitude, City Name)
 */
class CloudCredentialStore : public PersistableStore<CloudCredentialStore> {
 private:
  std::string notionToken;
  std::string notionDatabaseId;
  std::string readwiseToken;
  std::string todoistToken;
  std::string geminiApiKey;
  std::string wallabagUrl;
  std::string wallabagToken;
  std::string webdavUrl;
  std::string webdavUser;
  std::string webdavPass;
  std::string rssFeedUrl = "https://feeds.bbci.co.uk/news/rss.xml";
  std::string weatherCity = "San Francisco";
  float weatherLat = 37.7749f;
  float weatherLon = -122.4194f;

  CloudCredentialStore() = default;
  ~CloudCredentialStore() = default;
  friend class PersistableStore<CloudCredentialStore>;

  void syncToNvs() const;
  bool loadFromNvs();

 public:
  static const char* getFilePath() { return "/.crosspoint/cloud_credentials.json"; }

  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);

  bool saveToFile() const;
  bool loadFromFile();

  // Getters & Setters
  const std::string& getNotionToken() const { return notionToken; }
  void setNotionToken(const std::string& val) { notionToken = val; }

  const std::string& getNotionDatabaseId() const { return notionDatabaseId; }
  void setNotionDatabaseId(const std::string& val) { notionDatabaseId = val; }

  const std::string& getReadwiseToken() const { return readwiseToken; }
  void setReadwiseToken(const std::string& val) { readwiseToken = val; }

  const std::string& getTodoistToken() const { return todoistToken; }
  void setTodoistToken(const std::string& val) { todoistToken = val; }

  const std::string& getGeminiApiKey() const { return geminiApiKey; }
  void setGeminiApiKey(const std::string& val) { geminiApiKey = val; }

  const std::string& getRssFeedUrl() const { return rssFeedUrl; }
  void setRssFeedUrl(const std::string& val) { rssFeedUrl = val; }

  const std::string& getWeatherCity() const { return weatherCity; }
  void setWeatherCity(const std::string& val) { weatherCity = val; }

  float getWeatherLat() const { return weatherLat; }
  float getWeatherLon() const { return weatherLon; }
  void setWeatherCoordinates(float lat, float lon, const std::string& city = "") {
    weatherLat = lat;
    weatherLon = lon;
    if (!city.empty()) weatherCity = city;
  }
};

#define CLOUD_CREDENTIALS CloudCredentialStore::getInstance()
