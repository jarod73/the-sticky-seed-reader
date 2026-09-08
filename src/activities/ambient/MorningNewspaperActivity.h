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
  FETCHING,
  DISPLAYING,
  ERROR
};

class MorningNewspaperActivity final : public Activity {
 private:
  ButtonNavigator buttonNavigator;
  NewspaperState state = NewspaperState::CHECK_WIFI;
  std::string errorMessage;
  RssFeed feed;
  WeatherForecast forecast;
  size_t selectedStoryIndex = 0;

  void checkAndConnectWifi();
  void loadNewspaperData();
  void openStoryInBrowser(size_t index);

 public:
  explicit MorningNewspaperActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
