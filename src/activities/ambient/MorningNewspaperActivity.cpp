#include "MorningNewspaperActivity.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <HalClock.h>
#include <HalDisplay.h>
#include <HalPowerManager.h>
#include <I18n.h>
#include <Logging.h>
#include <WiFi.h>

#if FREEINK_CAP_TEMP_HUMIDITY
#include <EnvironmentSensor.h>
#endif

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "activities/ActivityManager.h"
#include "activities/browser/ReadabilityBrowserActivity.h"
#include "activities/network/WifiSelectionActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "network/CloudCredentialStore.h"
#include "network/HttpDownloader.h"
#include "util/TextWrapUtils.h"

MorningNewspaperActivity::MorningNewspaperActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("MorningNewspaper", renderer, mappedInput) {}

void MorningNewspaperActivity::onEnter() {
  Activity::onEnter();
  selectedStoryIndex = 0;
  errorMessage.clear();
  checkAndConnectWifi();
}

void MorningNewspaperActivity::onExit() { Activity::onExit(); }

void MorningNewspaperActivity::checkAndConnectWifi() {
  if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
    loadNewspaperData();
  } else {
    state = NewspaperState::WIFI_CONNECTING;
    startActivityForResult(
        std::make_unique<WifiSelectionActivity>(renderer, mappedInput, true),
        [this](const ActivityResult&) {
          if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
            loadNewspaperData();
          } else {
            state = NewspaperState::ERROR;
            errorMessage = "WiFi Connection Failed";
            requestUpdate();
          }
        });
  }
}

void MorningNewspaperActivity::loadNewspaperData() {
  state = NewspaperState::FETCHING;
  requestUpdate();

  // 1. Fetch Weather
  OpenMeteoClient::fetchForecast(CLOUD_CREDENTIALS.getWeatherLat(), CLOUD_CREDENTIALS.getWeatherLon(), forecast);

  // 2. Fetch RSS Feed
  std::string rssXml;
  std::string feedUrl = CLOUD_CREDENTIALS.getRssFeedUrl();
  LOG_INF("NEWSPAPER", "Fetching RSS feed from: %s", feedUrl.c_str());

  if (HttpDownloader::fetchUrl(feedUrl, rssXml) && !rssXml.empty()) {
    feed = RssFeedParser::parse(rssXml, 8);
  }

  if (!feed.valid) {
    state = NewspaperState::ERROR;
    errorMessage = "Failed to load news feed";
    requestUpdate();
    return;
  }

  state = NewspaperState::DISPLAYING;
  requestUpdate();
}

void MorningNewspaperActivity::openStoryInBrowser(size_t index) {
  if (index < feed.items.size() && !feed.items[index].link.empty()) {
    activityManager.replaceActivity(
        std::make_unique<ReadabilityBrowserActivity>(renderer, mappedInput, feed.items[index].link));
  }
}

void MorningNewspaperActivity::loop() {
  if (state == NewspaperState::WIFI_CONNECTING || state == NewspaperState::FETCHING) {
    return;
  }

  if (state == NewspaperState::ERROR) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      loadNewspaperData();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      onGoHome();
    }
    return;
  }

  if (state == NewspaperState::DISPLAYING) {
    const int storyCount = static_cast<int>(feed.items.size());

    // 1. Edge & Directional Swipes for fluid navigation
    if (mappedInput.wasBackGesture() || mappedInput.wasHomeGesture()) {
      onGoHome();
      return;
    }

    const auto swipe = mappedInput.wasSwipe();
    if (swipe == MappedInputManager::SwipeDir::Right) {
      onGoHome();
      return;
    } else if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Left) {
      if (selectedStoryIndex + 1 < storyCount) {
        selectedStoryIndex++;
        requestUpdate();
      }
      return;
    } else if (swipe == MappedInputManager::SwipeDir::Down) {
      if (selectedStoryIndex > 0) {
        selectedStoryIndex--;
        requestUpdate();
      }
      return;
    }

    // 2. Physical / Logical Hardware Button Navigation
    buttonNavigator.onNext([this, storyCount] {
      if (selectedStoryIndex + 1 < storyCount) {
        selectedStoryIndex++;
        requestUpdate();
      }
    });

    buttonNavigator.onPrevious([this] {
      if (selectedStoryIndex > 0) {
        selectedStoryIndex--;
        requestUpdate();
      }
    });

    // 3. Full-Screen Touch Taps (Top Masthead, Bottom Hints, Story Cards)
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      const int screenW = renderer.getScreenWidth();
      const int screenH = renderer.getScreenHeight();

      // Top Masthead: Tap left to exit Home, tap right to refresh
      if (ty < 55) {
        if (tx < screenW / 2) {
          onGoHome();
        } else {
          loadNewspaperData();
        }
        return;
      }

      // Bottom Footer Bar Button Hints
      if (ty >= screenH - 45) {
        if (tx < screenW / 4) {
          onGoHome();
        } else if (tx < screenW / 2) {
          openStoryInBrowser(selectedStoryIndex);
        } else if (tx < (3 * screenW) / 4) {
          if (selectedStoryIndex > 0) {
            selectedStoryIndex--;
            requestUpdate();
          }
        } else {
          if (selectedStoryIndex + 1 < storyCount) {
            selectedStoryIndex++;
            requestUpdate();
          }
        }
        return;
      }

      // Content Area: Tap Top Story or News Wire rows
      if (ty >= 86 && ty < screenH - 45) {
        if (tx < screenW / 2) {
          openStoryInBrowser(0);
          return;
        } else {
          int row = (ty - 110) / 60 + 1;
          if (row < storyCount) {
            openStoryInBrowser(row);
            return;
          }
        }
      }
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      openStoryInBrowser(selectedStoryIndex);
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      onGoHome();
    }
  }
}

void MorningNewspaperActivity::render(RenderLock&&) {
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();

  if (state == NewspaperState::FETCHING) {
    GUI.drawHeader(renderer, Rect{0, 0, pageWidth, 40}, "The Daily Sticky");
    renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2 - 20, tr(STR_LOADING), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(SMALL_FONT_ID, pageHeight / 2 + 15, "Fetching morning news & weather...");
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  if (state == NewspaperState::ERROR) {
    GUI.drawHeader(renderer, Rect{0, 0, pageWidth, 40}, "The Daily Sticky");
    renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2 - 20, errorMessage.c_str(), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(SMALL_FONT_ID, pageHeight / 2 + 20, "Press Confirm to retry, Back to exit");
    renderer.displayBuffer(HalDisplay::HALF_REFRESH);
    return;
  }

  // --- 1. Top Sub-Header Ears (Date/Time & Room Climate/Battery) ---
  char timeBuf[16] = {0};
  if (halClock.isAvailable() &&
      halClock.formatTime(timeBuf, sizeof(timeBuf), SETTINGS.clockUtcOffsetQ, SETTINGS.clockFormat == 1)) {
    char dateBuf[64];
    snprintf(dateBuf, sizeof(dateBuf), "THE STICKY GAZETTE  •  %s", timeBuf);
    renderer.drawText(SMALL_FONT_ID, 24, 8, dateBuf, true);
  } else {
    renderer.drawText(SMALL_FONT_ID, 24, 8, "THE STICKY GAZETTE  •  MORNING EDITION", true);
  }

  char rightInfo[64] = {0};
  size_t rightOff = 0;
#if FREEINK_CAP_TEMP_HUMIDITY
  EnvironmentSensor env;
  float inTemp = 0.0f, inHum = 0.0f;
  if (env.begin() && env.read(inTemp, inHum)) {
    rightOff += snprintf(rightInfo + rightOff, sizeof(rightInfo) - rightOff, "Room: %.1f°C / %.0f%% RH  •  ", inTemp,
                         inHum);
  }
#endif
  const uint16_t batt = powerManager.getBatteryPercentage();
  snprintf(rightInfo + rightOff, sizeof(rightInfo) - rightOff, "Bat: %u%%", batt);
  const int rightW = renderer.getTextWidth(SMALL_FONT_ID, rightInfo);
  renderer.drawText(SMALL_FONT_ID, pageWidth - 24 - rightW, 8, rightInfo, true);

  // Sub-header divider rule
  renderer.drawLine(20, 24, pageWidth - 20, 24);

  // --- 2. Authentic Broadsheet Masthead ---
  renderer.drawCenteredText(UI_12_FONT_ID, 30, "THE DAILY STICKY", true, EpdFontFamily::BOLD);

  // Double vintage horizontal rules
  renderer.drawLine(20, 52, pageWidth - 20, 52);
  renderer.drawLine(20, 55, pageWidth - 20, 55);

  // --- 3. Weather Forecast Capsule ---
  char weatherBuf[128] = {0};
  if (forecast.valid) {
    snprintf(weatherBuf, sizeof(weatherBuf), "Weather: %.1f°C, %s  •  High: %.0f°C  /  Low: %.0f°C",
             forecast.currentTempC, forecast.conditionText.c_str(), forecast.tempMaxC, forecast.tempMinC);
  } else {
    snprintf(weatherBuf, sizeof(weatherBuf), "Ambient Intelligence  •  Live RSS News Digest");
  }
  renderer.drawCenteredText(SMALL_FONT_ID, 63, weatherBuf, true);

  // Forecast divider rule
  renderer.drawLine(20, 80, pageWidth - 20, 80);

  // --- 4. Broadsheet Two-Column News Layout ---
  const int colW = (pageWidth - 60) / 2;
  const int midX = pageWidth / 2;

  // Vertical dividing rule between columns
  renderer.drawLine(midX, 84, midX, pageHeight - 44);

  // Left Column: Lead Top Story
  if (!feed.items.empty()) {
    const auto& top = feed.items[0];
    int y = 88;
    renderer.drawText(UI_10_FONT_ID, 24, y, "[ TOP STORY ]", true, EpdFontFamily::BOLD);
    y += 22;

    // Wrap Lead Headline
    y = TextWrapUtils::drawWrappedParagraph(renderer, UI_10_FONT_ID, 24, y, colW - 10, 60,
                                            top.title, EpdFontFamily::BOLD, 2);

    y += 4;
    renderer.drawLine(24, y, 24 + colW - 10, y);
    y += 10;

    // Lead Story Description Body
    TextWrapUtils::drawWrappedParagraph(renderer, SMALL_FONT_ID, 24, y, colW - 10,
                                        (pageHeight - 55) - y, top.description,
                                        EpdFontFamily::REGULAR, 2);
  }

  // Right Column: News Wire Briefs
  int yRight = 88;
  renderer.drawText(UI_10_FONT_ID, midX + 16, yRight, "[ NEWS WIRE ]", true, EpdFontFamily::BOLD);
  yRight += 22;

  for (size_t i = 1; i < std::min(feed.items.size(), size_t(5)); i++) {
    const auto& item = feed.items[i];
    std::string bullet = "• " + item.title;
    const auto style = (i == selectedStoryIndex) ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR;

    yRight = TextWrapUtils::drawWrappedParagraph(renderer, SMALL_FONT_ID, midX + 16, yRight,
                                                 colW - 10, 40, bullet, style, 2);
    yRight += 4;
    if (i < 4) {
      renderer.drawLine(midX + 16, yRight, midX + 16 + colW - 10, yRight);
      yRight += 8;
    }
  }

  // Footer Hints
  const auto labels = mappedInput.mapLabels("Home", "Read Story", "Prev", "Next");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(HalDisplay::HALF_REFRESH);
}
