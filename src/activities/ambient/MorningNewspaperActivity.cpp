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
  loadingProgress = 10;
  loadingMessage = "Connecting to Wi-Fi...";
  state = NewspaperState::CHECK_WIFI;
  requestUpdateAndWait();
}

void MorningNewspaperActivity::onExit() { Activity::onExit(); }

void MorningNewspaperActivity::checkAndConnectWifi() {
  if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
    loadNewspaperData();
  } else {
    state = NewspaperState::WIFI_CONNECTING;
    loadingProgress = 15;
    loadingMessage = "Connecting to Wi-Fi Network...";
    requestUpdateAndWait();
    startActivityForResult(std::make_unique<WifiSelectionActivity>(renderer, mappedInput, true),
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
  requestUpdateAndWait();
}

void MorningNewspaperActivity::loadNewspaperData() {
  state = NewspaperState::FETCHING_WEATHER;
  loadingProgress = 25;
  loadingMessage = "Fetching Weather Forecast...";
  requestUpdateAndWait();
}

void MorningNewspaperActivity::openStoryInBrowser(size_t index) {
  if (index < feed.items.size() && !feed.items[index].link.empty()) {
    const std::string& link = feed.items[index].link;
    if (link.rfind("http", 0) == 0) {
      activityManager.replaceActivity(std::make_unique<ReadabilityBrowserActivity>(renderer, mappedInput, link));
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

  // Handle tap during loading states to cancel
  if (state == NewspaperState::CHECK_WIFI || state == NewspaperState::WIFI_CONNECTING ||
      state == NewspaperState::FETCHING_WEATHER || state == NewspaperState::FETCHING_NEWS ||
      state == NewspaperState::PARSING_NEWS) {
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      onGoHome();
      return;
    }

    if (state == NewspaperState::CHECK_WIFI) {
      checkAndConnectWifi();
      return;
    }

    if (state == NewspaperState::FETCHING_WEATHER) {
      OpenMeteoClient::fetchForecast(CLOUD_CREDENTIALS.getWeatherLat(), CLOUD_CREDENTIALS.getWeatherLon(), forecast);
      state = NewspaperState::FETCHING_NEWS;
      loadingProgress = 55;
      loadingMessage = "Downloading Headlines & RSS Feeds...";
      requestUpdateAndWait();
      return;
    }

    if (state == NewspaperState::FETCHING_NEWS) {
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

      state = NewspaperState::PARSING_NEWS;
      loadingProgress = 85;
      loadingMessage = "Formatting Today's Gazette...";
      requestUpdateAndWait();
      return;
    }

    if (state == NewspaperState::PARSING_NEWS) {
      if (!feed.valid || feed.items.empty()) {
        loadOfflineDigest();
        return;
      }
      state = NewspaperState::DISPLAYING;
      requestUpdateAndWait();
      return;
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

    // Touch Taps: Header, Footer, Top Story, Wire Items, Reading Companion
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      const int screenW = renderer.getScreenWidth();
      const int screenH = renderer.getScreenHeight();
      const bool isPortrait = screenH > screenW;

      // 1. Top Masthead / Header Bar
      if (ty < 54) {
        if (tx < 120) {
          onGoHome();  // [ < Home ] button
        } else if (tx > screenW - 120) {
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

      if (isPortrait) {
        // Portrait Touch Zones
        // 3. Lead Story Zone (ty between 110 and 290)
        if (ty >= 110 && ty < 290) {
          selectedStoryIndex = 0;
          openStoryInBrowser(0);
          return;
        }

        // 4. News Wire Items Zone (ty between 290 and 540)
        if (ty >= 290 && ty < 540) {
          int itemIndex = (ty - 324) / 42 + 1;
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

        // 5. Reading Companion Zone (ty between 540 and 690)
        if (ty >= 540 && ty < 690) {
          const auto& recents = RECENT_BOOKS.getBooks();
          if (!recents.empty()) {
            activityManager.goToReader(recents[0].path, false);
          } else {
            activityManager.goToFileBrowser();
          }
          return;
        }
      } else {
        // Landscape Touch Zones
        const int midX = screenW / 2;
        // Left Column: Lead Story (top) and Reading Companion (bottom)
        if (tx < midX && ty >= 54 && ty < 280) {
          selectedStoryIndex = 0;
          openStoryInBrowser(0);
          return;
        } else if (tx < midX && ty >= 280 && ty < screenH - 48) {
          const auto& recents = RECENT_BOOKS.getBooks();
          if (!recents.empty()) {
            activityManager.goToReader(recents[0].path, false);
          } else {
            activityManager.goToFileBrowser();
          }
          return;
        }

        // Right Column: Headlines
        if (tx >= midX && ty >= 54 && ty < 320) {
          int itemIndex = (ty - 86) / 44 + 1;
          if (itemIndex >= 1 && itemIndex < storyCount) {
            if (selectedStoryIndex == static_cast<size_t>(itemIndex)) {
              openStoryInBrowser(itemIndex);
            } else {
              selectedStoryIndex = itemIndex;
              requestUpdate();
            }
            return;
          }
        } else if (tx >= midX && ty >= 320 && ty < screenH - 48) {
          loadNewspaperData();
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

  if (state == NewspaperState::CHECK_WIFI || state == NewspaperState::WIFI_CONNECTING ||
      state == NewspaperState::FETCHING_WEATHER || state == NewspaperState::FETCHING_NEWS ||
      state == NewspaperState::PARSING_NEWS) {
    Rect popupRect = GUI.drawPopup(renderer, loadingMessage.c_str());
    GUI.fillPopupProgress(renderer, popupRect, loadingProgress);
    renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - 30, "Tap anywhere or press Back to return home", true,
                              EpdFontFamily::ITALIC);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  if (pageHeight > pageWidth) {
    renderPortrait(pageWidth, pageHeight);
  } else {
    renderLandscape(pageWidth, pageHeight);
  }

  renderer.displayBuffer(HalDisplay::HALF_REFRESH);
}

void MorningNewspaperActivity::renderPortrait(int pageWidth, int pageHeight) {
  // --- 1. Top Navigation & Ear Details (collision-safe) ---
  renderer.drawRoundedRect(12, 6, 76, 26, 2, 6, true);
  renderer.drawText(SMALL_FONT_ID, 20, 11, "< HOME", true, EpdFontFamily::BOLD);

  renderer.drawRoundedRect(pageWidth - 88, 6, 76, 26, 2, 6, true);
  renderer.drawText(SMALL_FONT_ID, pageWidth - 80, 11, "REFRESH", true, EpdFontFamily::BOLD);

  // Masthead Title
  renderer.drawCenteredText(UI_12_FONT_ID, 12, "THE DAILY STICKY", true, EpdFontFamily::BOLD);

  // Status Ear Line under top buttons (y = 38)
  char leftInfo[48] = {0};
  char timeBuf[16] = {0};
  if (halClock.isAvailable() &&
      halClock.formatTime(timeBuf, sizeof(timeBuf), SETTINGS.clockUtcOffsetQ, SETTINGS.clockFormat == 1)) {
    snprintf(leftInfo, sizeof(leftInfo), "Edition: %s", timeBuf);
  } else {
    snprintf(leftInfo, sizeof(leftInfo), "Daily Edition");
  }
  renderer.drawText(SMALL_FONT_ID, 16, 38, leftInfo, true);

  char rightInfo[64] = {0};
  size_t rightOff = 0;
#if FREEINK_CAP_TEMP_HUMIDITY
  EnvironmentSensor env;
  float inTemp = 0.0f, inHum = 0.0f;
  if (env.begin() && env.read(inTemp, inHum)) {
    rightOff += snprintf(rightInfo + rightOff, sizeof(rightInfo) - rightOff, "Room: %.1f°C / %.0f%%  •  ", inTemp, inHum);
  }
#endif
  const uint16_t batt = powerManager.getBatteryPercentage();
  snprintf(rightInfo + rightOff, sizeof(rightInfo) - rightOff, "Bat: %u%%", batt);
  const int rightW = renderer.getTextWidth(SMALL_FONT_ID, rightInfo);
  renderer.drawText(SMALL_FONT_ID, pageWidth - 16 - rightW, 38, rightInfo, true);

  // Divider below top header
  renderer.drawLine(12, 56, pageWidth - 12, 56);

  // --- 2. Weather Capsule (y = 62..88) ---
  char weatherBuf[128] = {0};
  if (forecast.valid) {
    snprintf(weatherBuf, sizeof(weatherBuf), "Weather: %.1f°C, %s  •  H: %.0f°C / L: %.0f°C  •  Wind: %.0f km/h",
             forecast.currentTempC, forecast.conditionText.c_str(), forecast.tempMaxC, forecast.tempMinC,
             forecast.windSpeedKmh);
  } else {
    snprintf(weatherBuf, sizeof(weatherBuf), "The Sticky Gazette  •  Ambient Morning Intelligence & Daily Digest");
  }
  renderer.drawCenteredText(SMALL_FONT_ID, 64, weatherBuf, true);
  renderer.drawLine(12, 86, pageWidth - 12, 86);
  renderer.drawLine(12, 89, pageWidth - 12, 89);

  // --- 3. Section: Lead Story (y = 96..280) ---
  if (!feed.items.empty()) {
    const auto& top = feed.items[0];
    int y = 96;
    renderer.drawText(UI_10_FONT_ID, 16, y, "[ ★ LEAD STORY ]", true, EpdFontFamily::BOLD);
    y += 20;

    // Headline (up to 3 lines)
    y = TextWrapUtils::drawWrappedParagraph(renderer, UI_10_FONT_ID, 16, y, pageWidth - 32, 60, top.title,
                                            EpdFontFamily::BOLD, 3);
    y += 4;
    renderer.drawLine(16, y, pageWidth - 16, y);
    y += 8;

    // Summary (up to 4 lines)
    TextWrapUtils::drawWrappedParagraph(renderer, SMALL_FONT_ID, 16, y, pageWidth - 32, 70, top.description,
                                        EpdFontFamily::REGULAR, 4);

    // Read full story hint button
    renderer.drawRoundedRect(16, 258, 160, 22, 2, 4, true);
    renderer.drawText(SMALL_FONT_ID, 24, 262, "Read Full Article →", true, EpdFontFamily::BOLD);
  }
  renderer.drawLine(12, 288, pageWidth - 12, 288);

  // --- 4. Section: Top Headlines (y = 294..536) ---
  int yHeadlines = 294;
  renderer.drawText(UI_10_FONT_ID, 16, yHeadlines, "[ ⚡ TOP HEADLINES ]", true, EpdFontFamily::BOLD);
  yHeadlines += 22;

  const size_t maxHeadlines = std::min(feed.items.size(), size_t(6));
  for (size_t i = 1; i < maxHeadlines; i++) {
    const auto& item = feed.items[i];
    const bool isSelected = (i == selectedStoryIndex);

    if (isSelected) {
      renderer.drawRoundedRect(12, yHeadlines - 2, pageWidth - 24, 40, 1, 4, true);
    }

    std::string bullet = std::to_string(i) + ". " + item.title;
    yHeadlines = TextWrapUtils::drawWrappedParagraph(renderer, SMALL_FONT_ID, 18, yHeadlines, pageWidth - 36, 36, bullet,
                                                     isSelected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR, 2);
    yHeadlines += 4;
    if (i < maxHeadlines - 1 && !isSelected) {
      renderer.drawLine(18, yHeadlines, pageWidth - 18, yHeadlines);
    }
    yHeadlines += 4;
  }
  renderer.drawLine(12, 540, pageWidth - 12, 540);

  // --- 5. Section: Reading Companion Card (y = 546..682) ---
  renderer.drawText(UI_10_FONT_ID, 16, 548, "[ 📚 READING COMPANION ]", true, EpdFontFamily::BOLD);

  const auto& recents = RECENT_BOOKS.getBooks();
  if (!recents.empty()) {
    std::string bookLine = recents[0].title;
    if (!recents[0].author.empty()) {
      bookLine += " — " + recents[0].author;
    }
    TextWrapUtils::drawWrappedParagraph(renderer, SMALL_FONT_ID, 18, 572, pageWidth - 150, 36, bookLine,
                                        EpdFontFamily::BOLD, 2);

    // Resume button
    renderer.drawRoundedRect(pageWidth - 130, 568, 114, 24, 2, 4, true);
    renderer.drawText(SMALL_FONT_ID, pageWidth - 122, 573, "Resume Book →", true, EpdFontFamily::BOLD);

    // Reading statistics bar
    char statsBuf[96] = {0};
    snprintf(statsBuf, sizeof(statsBuf), "Streak: 🔥 %u Days   •   Speed: %u WPM   •   Total: %u mins",
             READING_STATS.getDailyStreak(), READING_STATS.getAverageWpm(), READING_STATS.getTotalReadingMinutes());
    renderer.drawText(SMALL_FONT_ID, 18, 614, statsBuf, true);
  } else {
    renderer.drawText(SMALL_FONT_ID, 18, 574, "Standalone 18MB Flash & MicroSD Card Ready", true, EpdFontFamily::BOLD);
    renderer.drawText(SMALL_FONT_ID, 18, 594, "Open the File Browser to select and read books.", true);

    char statsBuf[96] = {0};
    snprintf(statsBuf, sizeof(statsBuf), "Streak: 🔥 %u Days   •   Speed: %u WPM   •   Total: %u mins",
             READING_STATS.getDailyStreak(), READING_STATS.getAverageWpm(), READING_STATS.getTotalReadingMinutes());
    renderer.drawText(SMALL_FONT_ID, 18, 620, statsBuf, true);
  }
  renderer.drawLine(12, 650, pageWidth - 12, 650);

  // --- 6. Section: Thought / Literary Quote of the Day (y = 658..738) ---
  const time_t now = time(nullptr);
  const size_t quoteIdx = (now > 0) ? (now / 86400) % (sizeof(LITERARY_QUOTES) / sizeof(LITERARY_QUOTES[0])) : 0;
  TextWrapUtils::drawWrappedParagraph(renderer, SMALL_FONT_ID, 20, 660, pageWidth - 40, 64, LITERARY_QUOTES[quoteIdx],
                                      EpdFontFamily::ITALIC, 3);

  // --- 7. Footer Button Hints Bar (y = 752..800) ---
  const auto labels = mappedInput.mapLabels("Home", "Read Story", "Prev", "Next");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}

void MorningNewspaperActivity::renderLandscape(int pageWidth, int pageHeight) {
  // --- 1. Top Navigation & Ear Details ---
  renderer.drawRoundedRect(16, 6, 84, 28, 2, 6, true);
  renderer.drawText(SMALL_FONT_ID, 26, 13, "< HOME", true, EpdFontFamily::BOLD);

  renderer.drawRoundedRect(pageWidth - 100, 6, 84, 28, 2, 6, true);
  renderer.drawText(SMALL_FONT_ID, pageWidth - 90, 13, "REFRESH", true, EpdFontFamily::BOLD);

  char timeBuf[16] = {0};
  if (halClock.isAvailable() &&
      halClock.formatTime(timeBuf, sizeof(timeBuf), SETTINGS.clockUtcOffsetQ, SETTINGS.clockFormat == 1)) {
    char dateBuf[48];
    snprintf(dateBuf, sizeof(dateBuf), "EDITION: %s", timeBuf);
    renderer.drawText(SMALL_FONT_ID, 112, 13, dateBuf, true);
  }

  char rightInfo[64] = {0};
  size_t rightOff = 0;
#if FREEINK_CAP_TEMP_HUMIDITY
  EnvironmentSensor env;
  float inTemp = 0.0f, inHum = 0.0f;
  if (env.begin() && env.read(inTemp, inHum)) {
    rightOff += snprintf(rightInfo + rightOff, sizeof(rightInfo) - rightOff, "Room: %.1f°C / %.0f%%  •  ", inTemp, inHum);
  }
#endif
  const uint16_t batt = powerManager.getBatteryPercentage();
  snprintf(rightInfo + rightOff, sizeof(rightInfo) - rightOff, "Bat: %u%%", batt);
  const int rightW = renderer.getTextWidth(SMALL_FONT_ID, rightInfo);
  renderer.drawText(SMALL_FONT_ID, pageWidth - 112 - rightW, 13, rightInfo, true);

  renderer.drawLine(16, 38, pageWidth - 16, 38);

  // Masthead & Weather
  renderer.drawCenteredText(UI_12_FONT_ID, 42, "THE DAILY STICKY", true, EpdFontFamily::BOLD);
  renderer.drawLine(16, 64, pageWidth - 16, 64);

  char weatherBuf[128] = {0};
  if (forecast.valid) {
    snprintf(weatherBuf, sizeof(weatherBuf), "Weather: %.1f°C, %s  •  High: %.0f°C / Low: %.0f°C  •  Wind: %.1f km/h",
             forecast.currentTempC, forecast.conditionText.c_str(), forecast.tempMaxC, forecast.tempMinC,
             forecast.windSpeedKmh);
  } else {
    snprintf(weatherBuf, sizeof(weatherBuf), "The Sticky Gazette  •  Ambient Morning Intelligence & Daily Digest");
  }
  renderer.drawCenteredText(SMALL_FONT_ID, 68, weatherBuf, true);
  renderer.drawLine(16, 88, pageWidth - 16, 88);

  const int midX = pageWidth / 2;
  const int colW = midX - 32;

  // Vertical dividing rule
  renderer.drawLine(midX, 92, midX, pageHeight - 74);

  // Left Column: Lead Story + Reading Companion
  if (!feed.items.empty()) {
    const auto& top = feed.items[0];
    int y = 94;
    renderer.drawText(UI_10_FONT_ID, 20, y, "[ ★ LEAD STORY ]", true, EpdFontFamily::BOLD);
    y += 20;

    y = TextWrapUtils::drawWrappedParagraph(renderer, UI_10_FONT_ID, 20, y, colW, 46, top.title,
                                            EpdFontFamily::BOLD, 2);
    y += 4;
    renderer.drawLine(20, y, 20 + colW, y);
    y += 6;

    y = TextWrapUtils::drawWrappedParagraph(renderer, SMALL_FONT_ID, 20, y, colW, 80, top.description,
                                            EpdFontFamily::REGULAR, 4);

    renderer.drawRoundedRect(20, 260, 150, 22, 2, 4, true);
    renderer.drawText(SMALL_FONT_ID, 28, 264, "Read Full Article →", true, EpdFontFamily::BOLD);
  }

  // Reading Companion at bottom of Left Column
  renderer.drawLine(20, 292, midX - 20, 292);
  renderer.drawText(UI_10_FONT_ID, 20, 298, "[ 📚 READING COMPANION ]", true, EpdFontFamily::BOLD);

  const auto& recents = RECENT_BOOKS.getBooks();
  if (!recents.empty()) {
    std::string bookLine = recents[0].title;
    if (!recents[0].author.empty()) bookLine += " — " + recents[0].author;
    TextWrapUtils::drawWrappedParagraph(renderer, SMALL_FONT_ID, 20, 320, colW, 36, bookLine, EpdFontFamily::BOLD, 2);
    char statsBuf[64] = {0};
    snprintf(statsBuf, sizeof(statsBuf), "Streak: 🔥 %u Days  •  Speed: %u WPM", READING_STATS.getDailyStreak(),
             READING_STATS.getAverageWpm());
    renderer.drawText(SMALL_FONT_ID, 20, 362, statsBuf, true);
  } else {
    renderer.drawText(SMALL_FONT_ID, 20, 324, "Offline Library: 18MB Flash & SD Ready", true);
    char statsBuf[64] = {0};
    snprintf(statsBuf, sizeof(statsBuf), "Streak: 🔥 %u Days  •  Speed: %u WPM", READING_STATS.getDailyStreak(),
             READING_STATS.getAverageWpm());
    renderer.drawText(SMALL_FONT_ID, 20, 350, statsBuf, true);
  }

  // Right Column: Top Headlines (items 1..4)
  int yRight = 94;
  renderer.drawText(UI_10_FONT_ID, midX + 16, yRight, "[ ⚡ TOP HEADLINES ]", true, EpdFontFamily::BOLD);
  yRight += 22;

  const size_t maxItems = std::min(feed.items.size(), size_t(5));
  for (size_t i = 1; i < maxItems; i++) {
    const auto& item = feed.items[i];
    const bool isSelected = (i == selectedStoryIndex);

    if (isSelected) {
      renderer.drawRoundedRect(midX + 10, yRight - 2, colW, 44, 1, 4, true);
    }

    std::string bullet = std::to_string(i) + ". " + item.title;
    yRight = TextWrapUtils::drawWrappedParagraph(renderer, SMALL_FONT_ID, midX + 16, yRight, colW - 12, 38, bullet,
                                                 isSelected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR, 2);
    yRight += 4;
    if (i < maxItems - 1 && !isSelected) {
      renderer.drawLine(midX + 16, yRight, midX + 16 + colW - 12, yRight);
    }
    yRight += 6;
  }

  // Literary Quote & Button Hints
  renderer.drawLine(16, pageHeight - 74, pageWidth - 16, pageHeight - 74);
  const time_t now = time(nullptr);
  const size_t quoteIdx = (now > 0) ? (now / 86400) % (sizeof(LITERARY_QUOTES) / sizeof(LITERARY_QUOTES[0])) : 0;
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - 66, LITERARY_QUOTES[quoteIdx], true, EpdFontFamily::ITALIC);

  const auto labels = mappedInput.mapLabels("Home", "Read Story", "Prev", "Next");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
}
