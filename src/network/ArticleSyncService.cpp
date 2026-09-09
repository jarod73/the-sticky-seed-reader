#include "ArticleSyncService.h"

#include <ArduinoJson.h>
#include <FsHelpers.h>
#include <HalStorage.h>
#include <Logging.h>

#include "network/HttpDownloader.h"
#include "util/ReadabilityExtractor.h"

namespace {
constexpr char ARTICLE_DIR[] = "/.crosspoint/articles";
}

bool ArticleSyncService::syncArticlesFromUrl(const std::string& endpointUrl, const std::string& apiToken,
                                             SyncResult& result, ProgressCallback progress) {
  if (endpointUrl.empty()) {
    result.message = "Sync URL is empty";
    return false;
  }

  if (!Storage.exists(ARTICLE_DIR)) {
    Storage.mkdir(ARTICLE_DIR);
  }

  LOG_INF("SYNC", "Syncing saved articles from: %s", endpointUrl.c_str());

  std::string response;
  if (!HttpDownloader::fetchUrl(endpointUrl, response, apiToken.empty() ? "" : "bearer", apiToken) || response.empty()) {
    result.message = "Failed to reach article sync endpoint";
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, response);
  if (err) {
    result.message = "Invalid JSON response from server";
    return false;
  }

  JsonArray articles = doc["items"].as<JsonArray>();
  if (articles.isNull() || articles.size() == 0) {
    // If top-level is array
    articles = doc.as<JsonArray>();
  }

  size_t total = articles.size();
  size_t count = 0;

  for (JsonObject item : articles) {
    const std::string url = item["url"] | "";
    const std::string title = item["title"] | "Untitled Article";

    if (url.empty()) continue;

    if (progress) {
      progress(count + 1, total, title);
    }

    std::string html;
    if (HttpDownloader::fetchUrl(url, html) && !html.empty()) {
      auto article = ReadabilityExtractor::extract(html);
      if (article.valid) {
        // Save cleaned HTML file
        std::string filename;
        for (char c : (article.title.empty() ? title : article.title)) {
          if (isalnum(c) || c == ' ' || c == '_' || c == '-') filename += c;
        }
        if (filename.empty()) filename = "article_" + std::to_string(count);

        const std::string destPath = std::string(ARTICLE_DIR) + "/" + filename + ".html";
        HalFile outFile;
        if (Storage.openFileForWrite("SYNC", destPath.c_str(), outFile)) {
          std::string content = "<html><head><title>" + article.title + "</title></head><body><h1>" +
                                article.title + "</h1>";
          for (const auto& p : article.paragraphs) {
            content += "<p>" + p + "</p>";
          }
          content += "</body></html>";
          outFile.write(reinterpret_cast<const uint8_t*>(content.data()), content.size());
          outFile.close();
          count++;
        }
      }
    }
  }

  result.articlesSynced = count;
  result.message = "Successfully synced " + std::to_string(count) + " articles.";
  LOG_INF("SYNC", "%s", result.message.c_str());
  return true;
}

size_t ArticleSyncService::getLocalArticleCount() {
  size_t count = 0;
  auto dir = Storage.open(ARTICLE_DIR);
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return 0;
  }

  char name[256];
  for (auto file = dir.openNextFile(); file; file = dir.openNextFile()) {
    if (!file.isDirectory()) {
      file.getName(name, sizeof(name));
      if (FsHelpers::hasHtmlExtension(name)) {
        count++;
      }
    }
    file.close();
  }
  dir.close();
  return count;
}
