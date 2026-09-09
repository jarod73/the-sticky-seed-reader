#pragma once

#include "activities/Activity.h"
#include "network/ArticleSyncService.h"

/**
 * @brief Saved Web Article Sync Activity on E-Ink (Pocket / Wallabag / Self-hosted).
 */
class ArticleSyncActivity final : public Activity {
 public:
  explicit ArticleSyncActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~ArticleSyncActivity() override = default;

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void startSync();

  std::string statusMessage_;
  std::string currentArticleTitle_;
  bool isSyncing_ = false;
  size_t currentProgress_ = 0;
  size_t totalProgress_ = 0;
  size_t localArticleCount_ = 0;
};
