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
#include "ReadingStatsStore.h"
#include "RecentBooksStore.h"
#include "activities/ActivityManager.h"
#include "activities/browser/ReadabilityBrowserActivity.h"
#include "activities/network/WifiSelectionActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "network/CloudCredentialStore.h"
#include "network/HttpDownloader.h"
#include "util/TextWrapUtils.h"

namespace {
const char* const FALLBACK_FEEDS[] = {
    "https://feeds.npr.org/1001/rss.xml",
    "https://rss.nytimes.com/services/xml/rss/nyt/HomePage.xml",
    "https://news.ycombinator.com/rss",
    "https://feeds.bbci.co.uk/news/rss.xml",
};

const char* const LITERARY_QUOTES[] = {
    "\"A reader lives a thousand lives before he dies.\" — George R.R. Martin",
    "\"There is no friend as loyal as a book.\" — Ernest Hemingway",
    "\"Today a reader, tomorrow a leader.\" — Margaret Fuller",
    "\"Books are a uniquely portable magic.\" — Stephen King",
    "\"Reading is essential for those who seek to rise above the ordinary.\" — Jim Rohn",
    "\"The reading of all good books is like conversation with the finest minds.\" — Descartes",
    "\"I have always imagined that Paradise will be a kind of library.\" — Jorge Luis Borges",
};
}  // namespace

MorningNewspaperActivity::MorningNewspaperActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("MorningNewspaper", renderer, mappedInput) {}

void MorningNewspaperActivity::onEnter() {
  Activity::onEnter();
  selectedStoryIndex = 0;
  errorMessage.clear();
  checkAndConnectWifi();
}

void MorningNewspaperActivity::onExit() {
  Activity::onExit();
}

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
            // Load rich offline edition rather than halting on an error screen
            loadOfflineDigest();
          }
        });
  }
}

void MorningNewspaperActivity::loadOfflineDigest() {
  feed.items.clear();
  feed.title = "The Sticky Gazette (Offline Edition)";
  feed.valid = true;

  // Lead Story: Library summary or current reading book
  const auto& recents = RECENT_BOOKS.getBooks();
  RssItem top;
  if (!recents.empty()) {
    top.title = "Currently Reading: " + recents[0].title;
    top.description = "By " + (recents[0].author.empty() ? std::string("Unknown Author") : recents[0].author) +
                      ". Pick up where you left off or explore your local book collection in the File Browser.";
    top.link = recents[0].path;
  } else {
    top.title = "Welcome to The Daily Sticky";
    top.description =
        "Your ambient morning newspaper & e-reader station. Connect to Wi-Fi to pull live world news, weather, "
        "and encyclopedic digests, or read offline anywhere.";
    top.link = "";
  }
  feed.items.push_back(std::move(top));

  // News Wire Briefs from reading telemetry and device capabilities
  RssItem item1;
  item1.title = "Reading Streak: " + std::to_string(READING_STATS.getDailyStreak()) + " Days Active";
  item1.description = "Cumulative reading time: " + std::to_string(READING_STATS.getTotalReadingMinutes()) +
                      " minutes across " + std::to_string(READING_STATS.getTotalPagesRead()) + " pages.";
  feed.items.push_back(std::move(item1));

  RssItem item2;
  item2.title = "Pacing & Speed: " + std::to_string(READING_STATS.getAverageWpm()) + " Words Per Minute";
  item2.description = "Calculated rolling average speed across recent reading sessions.";
  feed.items.push_back(std::move(item2));

  RssItem item3;
  item3.title = "Offline Library: Standalone 18MB Flash & MicroSD Ready";
  item3.description = "Browse EPUB, PDF, MOBI, CBZ, FB2, and TXT documents directly from storage.";
  feed.items.push_back(std::move(item3));

  state = NewspaperState::DISPLAYING;
  requestUpdate();
}

void MorningNewspaperActivity::loadNewspaperData() {
  state = NewspaperState::FETCHING;
  requestUpdate();

  // 1. Fetch Weather Forecast from Open-Meteo
  OpenMeteoClient::fetchForecast(CLOUD_CREDENTIALS.getWeatherLat(), CLOUD_CREDENTIALS.getWeatherLon(), forecast);

  // 2. Fetch RSS Feed with multi-source fallback
  std::string rssXml;
  std::string feedUrl = CLOUD_CREDENTIALS.getRssFeedUrl();
  LOG_INF("NEWSPAPER", "Attempting primary RSS feed: %s", feedUrl.c_str());

  if (HttpDownloader::fetchUrl(feedUrl, rssXml) && !rssXml.empty()) {
    feed = RssFeedParser::parse(rssXml, 10);
  }

  // If primary feed failed or returned < 2 items, try fallbacks
  if (!feed.valid || feed.items.size() < 2) {
    for (const char* fallbackUrl : FALLBACK_FEEDS) {
      LOG_INF("NEWSPAPER", "Trying fallback RSS feed: %s", fallbackUrl);
      rssXml.clear();
      if (HttpDownloader::fetchUrl(fallbackUrl, rssXml) && !rssXml.empty()) {
        feed = RssFeedParser::parse(rssXml, 10);
        if (feed.valid && !feed.items.empty()) {
          break;
        }
      }
    }
  }

  if (!feed.valid || feed.items.empty()) {
    // If all network feeds fail, gracefully show the offline digest
    loadOfflineDigest();
    return;
  }

  state = NewspaperState::DISPLAYING;
  requestUpdate();
}

void MorningNewspaperActivity::openStoryInBrowser(size_t index) {
  if (index < feed.items.size() && !feed.items[index].link.empty()) {
    const std::string& link = feed.items[index].link;
    if (link.rfind("http", 0) == 0) {
      activityManager.replaceActivity(
          std::make_unique<ReadabilityBrowserActivity>(renderer, mappedInput, link));
    } else if (link.rfind("/", 0) == 0) {
      activityManager.goToReader(link, false);
    }
  }
}

void MorningNewspaperActivity::loop() {
  // Always allow instant exit via Back button, Home button, or touch gestures regardless of state
  if (mappedInput.wasBackGesture() || mappedInput.wasHomeGesture() ||
      mappedInput.wasReleased(MappedInputManager::Button::Back) ||
      mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    onGoHome();
    return;
  }

  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Right) {
    onGoHome();
    return;
  }

  if (state == NewspaperState::WIFI_CONNECTING || state == NewspaperState::FETCHING) {
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      onGoHome();
    }
    return;
  }

  if (state == NewspaperState::ERROR) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      loadNewspaperData();
    } else {
      int tx = 0, ty = 0;
      if (mappedInput.wasScreenTapped(tx, ty)) {
        onGoHome();
      }
    }
    return;
  }

  if (state == NewspaperState::DISPLAYING) {
    const int storyCount = static_cast<int>(feed.items.size());

    // Swipe navigation between stories
    if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Left) {
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

    // Physical Hardware Button Navigation
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

    // Touch Taps: Header, Footer, Top Story, Wire Items
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      const int screenW = renderer.getScreenWidth();
      const int screenH = renderer.getScreenHeight();

      // 1. Top Masthead / Header Bar
      if (ty < 50) {
        if (tx < 140) {
          onGoHome();  // [ < Home ] button
        } else if (tx > screenW - 140) {
          loadNewspaperData();  // [ ⟳ Refresh ] button
        } else {
          onGoHome();  // Tapping title bar exits home
        }
        return;
      }

      // 2. Bottom Footer Touch Bar
      if (ty >= screenH - 48) {
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

      // 3. Left Column: Lead Story
      const int midX = screenW / 2;
      if (tx < midX && ty >= 84 && ty < screenH - 50) {
        selectedStoryIndex = 0;
        openStoryInBrowser(0);
        return;
      }

      // 4. Right Column: News Wire Items
      if (tx >= midX && ty >= 84 && ty < screenH - 50) {
        int itemIndex = (ty - 110) / 54 + 1;
        if (itemIndex >= 1 && itemIndex < storyCount) {
          if (selectedStoryIndex == static_cast<size_t>(itemIndex)) {
            openStoryInBrowser(itemIndex);
          } else {
            selectedStoryIndex = itemIndex;
            requestUpdate();
          }
          return;
        }
      }
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      openStoryInBrowser(selectedStoryIndex);
    }
  }
}

void MorningNewspaperActivity::render(RenderLock&&) {
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  renderer.clearScreen(0xFF);

  if (state == NewspaperState::FETCHING || state == NewspaperState::WIFI_CONNECTING) {
    GUI.drawHeader(renderer, Rect{0, 0, pageWidth, 40}, "The Daily Sticky");
    renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2 - 20, "Loading Morning Edition...", true,
                              EpdFontFamily::BOLD);
    renderer.drawCenteredText(SMALL_FONT_ID, pageHeight / 2 + 15, "Fetching live news feeds & weather forecast...");
    renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - 30, "Tap anywhere or press Back to return home", true,
                              EpdFontFamily::ITALIC);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  // --- 1. Top Navigation & Ear Details ---
  // [ < Home ] Button Pill on Top Left
  renderer.drawRoundedRect(16, 6, 84, 28, 2, 6, true);
  renderer.drawText(SMALL_FONT_ID, 26, 13, "< HOME", true, EpdFontFamily::BOLD);

  // [ ⟳ Refresh ] Button Pill on Top Right
  renderer.drawRoundedRect(pageWidth - 100, 6, 84, 28, 2, 6, true);
  renderer.drawText(SMALL_FONT_ID, pageWidth - 90, 13, "REFRESH", true, EpdFontFamily::BOLD);

  // Date / Time in center-left ear
  char timeBuf[16] = {0};
  if (halClock.isAvailable() &&
      halClock.formatTime(timeBuf, sizeof(timeBuf), SETTINGS.clockUtcOffsetQ, SETTINGS.clockFormat == 1)) {
    char dateBuf[48];
    snprintf(dateBuf, sizeof(dateBuf), "EDITION: %s", timeBuf);
    renderer.drawText(SMALL_FONT_ID, 112, 13, dateBuf, true);
  }

  // Room Climate & Battery Status in center-right ear
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
  renderer.drawText(SMALL_FONT_ID, pageWidth - 112 - rightW, 13, rightInfo, true);

  // Sub-header divider
  renderer.drawLine(16, 38, pageWidth - 16, 38);

  // --- 2. Broadsheet Masthead ---
  renderer.drawCenteredText(UI_12_FONT_ID, 42, "THE DAILY STICKY", true, EpdFontFamily::BOLD);
  renderer.drawLine(16, 64, pageWidth - 16, 64);
  renderer.drawLine(16, 67, pageWidth - 16, 67);

  // --- 3. Weather & Almanac Capsule ---
  char weatherBuf[128] = {0};
  if (forecast.valid) {
    snprintf(weatherBuf, sizeof(weatherBuf), "Weather: %.1f°C, %s  •  High: %.0f°C / Low: %.0f°C  •  Wind: %.1f km/h",
             forecast.currentTempC, forecast.conditionText.c_str(), forecast.tempMaxC, forecast.tempMinC,
             forecast.windSpeedKmh);
  } else {
    snprintf(weatherBuf, sizeof(weatherBuf), "The Sticky Gazette  •  Ambient Morning Intelligence & Daily Digest");
  }
  renderer.drawCenteredText(SMALL_FONT_ID, 72, weatherBuf, true);
  renderer.drawLine(16, 88, pageWidth - 16, 88);

  // --- 4. Broadsheet Two-Column News Layout ---
  const int colW = (pageWidth - 52) / 2;
  const int midX = pageWidth / 2;

  // Vertical dividing rule
  renderer.drawLine(midX, 92, midX, pageHeight - 74);

  // Left Column: Lead Story
  if (!feed.items.empty()) {
    const auto& top = feed.items[0];
    int y = 94;
    renderer.drawText(UI_10_FONT_ID, 20, y, "[ ★ LEAD STORY ]", true, EpdFontFamily::BOLD);
    y += 22;

    // Headline
    y = TextWrapUtils::drawWrappedParagraph(renderer, UI_10_FONT_ID, 20, y, colW - 8, 62, top.title,
                                            EpdFontFamily::BOLD, 2);
    y += 4;
    renderer.drawLine(20, y, 20 + colW - 8, y);
    y += 10;

    // Description
    TextWrapUtils::drawWrappedParagraph(renderer, SMALL_FONT_ID, 20, y, colW - 8, (pageHeight - 110) - y,
                                        top.description, EpdFontFamily::REGULAR, 2);

    // Read full story hint button at column bottom
    renderer.drawRoundedRect(20, pageHeight - 104, 150, 24, 2, 4, true);
    renderer.drawText(SMALL_FONT_ID, 28, pageHeight - 99, "Read Full Article →", true, EpdFontFamily::BOLD);
  }

  // Right Column: News Wire Briefs
  int yRight = 94;
  renderer.drawText(UI_10_FONT_ID, midX + 16, yRight, "[ ⚡ NEWS WIRE ]", true, EpdFontFamily::BOLD);
  yRight += 22;

  for (size_t i = 1; i < std::min(feed.items.size(), size_t(5)); i++) {
    const auto& item = feed.items[i];
    const bool isSelected = (i == selectedStoryIndex);

    if (isSelected) {
      renderer.drawRoundedRect(midX + 10, yRight - 2, colW - 12, 48, 1, 4, true);
    }

    std::string bullet = std::to_string(i) + ". " + item.title;
    yRight = TextWrapUtils::drawWrappedParagraph(renderer, SMALL_FONT_ID, midX + 16, yRight, colW - 24, 42,
                                                 bullet, isSelected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR, 2);
    yRight += 4;
    if (i < 4 && !isSelected) {
      renderer.drawLine(midX + 16, yRight, midX + 16 + colW - 24, yRight);
    }
    yRight += 6;
  }

  // --- 5. Bottom Thought / Literary Quote of the Day Capsule ---
  renderer.drawLine(16, pageHeight - 74, pageWidth - 16, pageHeight - 74);
  const time_t now = time(nullptr);
  const size_t quoteIdx = (now > 0) ? (now / 86400) % (sizeof(LITERARY_QUOTES) / sizeof(LITERARY_QUOTES[0])) : 0;
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - 66, LITERARY_QUOTES[quoteIdx], true, EpdFontFamily::ITALIC);

  // --- 6. Footer Button Hints Bar ---
  const auto labels = mappedInput.mapLabels("Home", "Read Story", "Prev", "Next");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(HalDisplay::HALF_REFRESH);
}
