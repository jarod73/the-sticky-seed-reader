#include "ArticleSyncActivity.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

ArticleSyncActivity::ArticleSyncActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("ArticleSync", renderer, mappedInput) {}

void ArticleSyncActivity::onEnter() {
  Activity::onEnter();
  localArticleCount_ = ArticleSyncService::getLocalArticleCount();
  statusMessage_ = "Press Sync to pull unread articles from your queue.";
  requestUpdate();
}

void ArticleSyncActivity::onExit() {
  Activity::onExit();
}

void ArticleSyncActivity::startSync() {
  isSyncing_ = true;
  currentProgress_ = 0;
  totalProgress_ = 0;
  statusMessage_ = "Connecting to queue...";
  requestUpdate();

  ArticleSyncService::SyncResult result;
  // If no custom endpoint is set, use sample demo endpoint
  const std::string endpoint = "https://raw.githubusercontent.com/jarod73/the-sticky-seed-reader/develop/.crosspoint/sample_articles.json";

  ArticleSyncService::syncArticlesFromUrl(
      endpoint, "", result,
      [this](size_t current, size_t total, const std::string& title) {
        currentProgress_ = current;
        totalProgress_ = total;
        currentArticleTitle_ = title;
        requestUpdate();
      });

  isSyncing_ = false;
  localArticleCount_ = ArticleSyncService::getLocalArticleCount();
  statusMessage_ = result.message;
  requestUpdate();
}

void ArticleSyncActivity::loop() {
  if (isSyncing_) return;

  int tx = 0, ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    if (ty < 50) {
      onGoHome();
      return;
    }
    if (ty > renderer.getScreenHeight() - 60) {
      startSync();
      return;
    }
  }

  if (mappedInput.wasBackGesture() || mappedInput.wasHomeGesture() ||
      mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoHome();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    startSync();
  }
}

void ArticleSyncActivity::render(RenderLock&&) {
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();

  renderer.clearScreen(0xFF);
  GUI.drawHeader(renderer, Rect{0, 0, screenW, 40}, "Saved Web Articles (Pocket / Wallabag)");

  // 1. Article Count Card
  renderer.drawRoundedRect(16, 56, screenW - 32, 70, 2, 8, true);
  char countBuf[64];
  snprintf(countBuf, sizeof(countBuf), "%zu Saved Offline Articles", localArticleCount_);
  renderer.drawText(UI_12_FONT_ID, 32, 74, countBuf, true, EpdFontFamily::BOLD);
  renderer.drawText(SMALL_FONT_ID, 32, 98, "Stored in /.crosspoint/articles/ (readable in File Browser)", true);

  // 2. Sync Progress or Status
  if (isSyncing_) {
    char progBuf[64];
    snprintf(progBuf, sizeof(progBuf), "Syncing article %zu / %zu", currentProgress_, totalProgress_);
    Rect popupRect = GUI.drawPopup(renderer, progBuf);
    if (totalProgress_ > 0) {
      GUI.fillPopupProgress(renderer, popupRect, (currentProgress_ * 100) / totalProgress_);
    }
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  renderer.drawCenteredText(UI_10_FONT_ID, screenH / 2 + 10, statusMessage_.c_str(), true);

  const auto labels = mappedInput.mapLabels("Back", "Sync Now", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
