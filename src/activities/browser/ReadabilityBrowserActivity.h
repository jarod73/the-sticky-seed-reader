#pragma once

#include <string>
#include <vector>

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"
#include "util/ReadabilityExtractor.h"

enum class WebReaderState {
  CHECK_WIFI,
  WIFI_CONNECTING,
  URL_INPUT,
  FETCHING,
  READING,
  ERROR
};

class ReadabilityBrowserActivity final : public Activity {
 private:
  ButtonNavigator buttonNavigator;
  WebReaderState state = WebReaderState::CHECK_WIFI;
  std::string currentUrl;
  std::string errorMessage;
  ReadabilityArticle article;

  // Pagination
  std::vector<std::string> pages;
  size_t currentPage = 0;

  void checkAndConnectWifi();
  void promptUrlEntry();
  void fetchAndParseUrl(const std::string& url);
  void paginateContent();
  void saveArticleOffline();

 public:
  explicit ReadabilityBrowserActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                      std::string initialUrl = "");
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
