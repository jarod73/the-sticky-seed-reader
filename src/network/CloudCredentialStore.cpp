#include "CloudCredentialStore.h"

#include <Logging.h>
#include <Preferences.h>

void CloudCredentialStore::syncToNvs() const {
  Preferences prefs;
  if (!prefs.begin("cp_cloud", false)) {
    LOG_ERR("CLOUD", "Failed to open NVS for cloud credentials persistence");
    return;
  }
  JsonDocument doc;
  toJson(doc);
  String jsonStr;
  serializeJson(doc, jsonStr);
  prefs.putString("cloud_data", jsonStr);
  prefs.end();
  LOG_DBG("CLOUD", "Synced cloud credentials to NVS");
}

bool CloudCredentialStore::loadFromNvs() {
  Preferences prefs;
  if (!prefs.begin("cp_cloud", true)) {
    return false;
  }
  String jsonStr = prefs.getString("cloud_data", "");
  prefs.end();
  if (jsonStr.isEmpty()) {
    return false;
  }
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, jsonStr);
  if (err) {
    LOG_ERR("CLOUD", "Failed to deserialize NVS cloud data: %s", err.c_str());
    return false;
  }
  bool ok = fromJson(doc.as<JsonVariantConst>());
  if (ok) {
    LOG_INF("CLOUD", "Restored cloud credentials from NVS backup");
    PersistableStore<CloudCredentialStore>::saveToFile();
  }
  return ok;
}

bool CloudCredentialStore::saveToFile() const {
  bool ok = PersistableStore<CloudCredentialStore>::saveToFile();
  syncToNvs();
  return ok;
}

bool CloudCredentialStore::loadFromFile() {
  bool ok = PersistableStore<CloudCredentialStore>::loadFromFile();
  if (!ok) {
    if (loadFromNvs()) {
      ok = true;
    }
  } else {
    syncToNvs();
  }
  return ok;
}

void CloudCredentialStore::toJson(JsonDocument& doc) const {
  doc["notion_token"] = notionToken;
  doc["notion_db"] = notionDatabaseId;
  doc["readwise_token"] = readwiseToken;
  doc["todoist_token"] = todoistToken;
  doc["gemini_key"] = geminiApiKey;
  doc["wallabag_url"] = wallabagUrl;
  doc["wallabag_token"] = wallabagToken;
  doc["webdav_url"] = webdavUrl;
  doc["webdav_user"] = webdavUser;
  doc["webdav_pass"] = webdavPass;
  doc["rss_feed_url"] = rssFeedUrl;
  doc["weather_city"] = weatherCity;
  doc["weather_lat"] = weatherLat;
  doc["weather_lon"] = weatherLon;
}

bool CloudCredentialStore::fromJson(JsonVariantConst doc) {
  if (doc.isNull() || !doc.is<JsonObjectConst>()) return false;

  notionToken = doc["notion_token"] | "";
  notionDatabaseId = doc["notion_db"] | "";
  readwiseToken = doc["readwise_token"] | "";
  todoistToken = doc["todoist_token"] | "";
  geminiApiKey = doc["gemini_key"] | "";
  wallabagUrl = doc["wallabag_url"] | "";
  wallabagToken = doc["wallabag_token"] | "";
  webdavUrl = doc["webdav_url"] | "";
  webdavUser = doc["webdav_user"] | "";
  webdavPass = doc["webdav_pass"] | "";
  rssFeedUrl = doc["rss_feed_url"] | "https://feeds.bbci.co.uk/news/rss.xml";
  weatherCity = doc["weather_city"] | "San Francisco";
  weatherLat = doc["weather_lat"] | 37.7749f;
  weatherLon = doc["weather_lon"] | -122.4194f;

  return true;
}
