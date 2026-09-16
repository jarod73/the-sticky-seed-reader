#include "CloudCredentialStore.h"

#include <Logging.h>
#include <ObfuscationUtils.h>
#include <Preferences.h>

namespace {
// Generous ceiling for a token/API key; rejects a corrupted or hostile
// decoded length before it gets allocated as a std::string.
constexpr size_t MAX_SECRET_LENGTH = 512;

// Reads an XOR+base64 obfuscated secret ("<key>_obf"), falling back to a
// legacy plaintext "<key>" field so credentials written before obfuscation
// was added still load; requests a resave (to upgrade them to obfuscated
// storage) whenever the legacy fallback is used. Mirrors the pattern
// WifiCredentialStore/extractPassword uses for the wifi password.
std::string extractSecret(JsonVariantConst doc, const char* obfKey, const char* legacyKey, bool& needsResave) {
  const char* obf = doc[obfKey] | "";
  if (obf[0] != '\0') {
    bool ok = false;
    bool tooLong = false;
    std::string value = obfuscation::deobfuscateFromBase64(obf, MAX_SECRET_LENGTH, &ok, &tooLong);
    if (ok && !tooLong) return value;
  }
  const char* legacy = doc[legacyKey] | "";
  if (legacy[0] != '\0') needsResave = true;
  return legacy;
}
}  // namespace

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
  // Tokens/keys/passwords are XOR+base64 obfuscated with the device's
  // hardware MAC before hitting the SD card or NVS, same as WifiCredentialStore
  // -- plaintext here would hand over every connected cloud account to anyone
  // who pulls the SD card.
  doc["notion_token_obf"] = obfuscation::obfuscateToBase64(notionToken);
  doc["notion_db"] = notionDatabaseId;
  doc["readwise_token_obf"] = obfuscation::obfuscateToBase64(readwiseToken);
  doc["todoist_token_obf"] = obfuscation::obfuscateToBase64(todoistToken);
  doc["gemini_key_obf"] = obfuscation::obfuscateToBase64(geminiApiKey);
  doc["wallabag_url"] = wallabagUrl;
  doc["wallabag_token_obf"] = obfuscation::obfuscateToBase64(wallabagToken);
  doc["webdav_url"] = webdavUrl;
  doc["webdav_user"] = webdavUser;
  doc["webdav_pass_obf"] = obfuscation::obfuscateToBase64(webdavPass);
  doc["rss_feed_url"] = rssFeedUrl;
  doc["weather_city"] = weatherCity;
  doc["weather_lat"] = weatherLat;
  doc["weather_lon"] = weatherLon;
}

bool CloudCredentialStore::fromJson(JsonVariantConst doc) {
  if (doc.isNull() || !doc.is<JsonObjectConst>()) return false;

  bool needsResave = false;
  notionToken = extractSecret(doc, "notion_token_obf", "notion_token", needsResave);
  notionDatabaseId = doc["notion_db"] | "";
  readwiseToken = extractSecret(doc, "readwise_token_obf", "readwise_token", needsResave);
  todoistToken = extractSecret(doc, "todoist_token_obf", "todoist_token", needsResave);
  geminiApiKey = extractSecret(doc, "gemini_key_obf", "gemini_key", needsResave);
  wallabagUrl = doc["wallabag_url"] | "";
  wallabagToken = extractSecret(doc, "wallabag_token_obf", "wallabag_token", needsResave);
  webdavUrl = doc["webdav_url"] | "";
  webdavUser = doc["webdav_user"] | "";
  webdavPass = extractSecret(doc, "webdav_pass_obf", "webdav_pass", needsResave);
  rssFeedUrl = doc["rss_feed_url"] | "https://feeds.bbci.co.uk/news/rss.xml";
  weatherCity = doc["weather_city"] | "San Francisco";
  weatherLat = doc["weather_lat"] | 37.7749f;
  weatherLon = doc["weather_lon"] | -122.4194f;

  if (needsResave) requestResave();

  return true;
}
