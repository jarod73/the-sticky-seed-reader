#pragma once

#include <functional>
#include <string>
#include <vector>

/**
 * @brief Saved Web Article Sync Service (Pocket & Wallabag integration).
 *
 * Downloads unread saved web articles over Wi-Fi, extracts clean readable
 * text and headings via ReadabilityExtractor, and saves them locally as
 * offline e-book chapters.
 */
class ArticleSyncService {
 public:
  struct SyncResult {
    size_t articlesSynced = 0;
    size_t errors = 0;
    std::string message;
  };

  using ProgressCallback = std::function<void(size_t current, size_t total, const std::string& title)>;

  static bool syncArticlesFromUrl(const std::string& endpointUrl, const std::string& apiToken,
                                  SyncResult& result, ProgressCallback progress = nullptr);

  static size_t getLocalArticleCount();
};
