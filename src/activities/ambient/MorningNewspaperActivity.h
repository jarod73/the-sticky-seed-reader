#pragma once

#include <string>
#include <vector>

#include "activities/Activity.h"
#include "network/services/OpenMeteoClient.h"
#include "util/ButtonNavigator.h"
#include "util/RssFeedParser.h"

enum class NewspaperState {
  CHECK_WIFI,
  WIFI_CONNECTING,
  FETCHING_WEATHER,
  FETCHING_NEWS,
  PARSING_NEWS,
  DISPLAYING,
  ERROR
};

class MorningNewspaperActivity final : public Activity {
 private:
  ButtonNavigator buttonNavigator;
  NewspaperState state = NewspaperState::CHECK_WIFI;
  int loadingProgress = 0;
  std::string loadingMessage = "Connecting to Wi-Fi...";
  std::string errorMessage;
  RssFeed feed;
  WeatherForecast forecast;
  size_t selectedStoryIndex = 0;

  void checkAndConnectWifi();
  void loadNewspaperData();
  void loadOfflineDigest();
  void openStoryInBrowser(size_t index);

  void renderPortrait(int pageWidth, int pageHeight);
  void renderLandscape(int pageWidth, int pageHeight);

 public:
  explicit MorningNewspaperActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
