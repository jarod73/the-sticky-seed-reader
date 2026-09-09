#include "ReadabilityBrowserActivity.h"

#include <Arduino.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>
#include <WiFi.h>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "SilentRestart.h"
#include "activities/ActivityManager.h"
#include "activities/network/WifiSelectionActivity.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UIScale.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "network/HttpDownloader.h"
#include "util/TextWrapUtils.h"

namespace {
constexpr int MARGIN_X = 24;
constexpr int MARGIN_TOP = 50;
constexpr int MARGIN_BOTTOM = 40;
}  // namespace

ReadabilityBrowserActivity::ReadabilityBrowserActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                                       std::string initialUrl)
    : Activity("ReadabilityBrowser", renderer, mappedInput), currentUrl(std::move(initialUrl)) {}

void ReadabilityBrowserActivity::onEnter() {
  Activity::onEnter();
  currentPage = 0;
  pages.clear();
  errorMessage.clear();

  if (currentUrl.empty()) {
    currentUrl = "https://en.wikipedia.org/wiki/Special:Random";
  }

  checkAndConnectWifi();
}

void ReadabilityBrowserActivity::onExit() {
  Activity::onExit();
  pages.clear();
}

void ReadabilityBrowserActivity::checkAndConnectWifi() {
  if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
    fetchAndParseUrl(currentUrl);
  } else {
    state = WebReaderState::WIFI_CONNECTING;
    startActivityForResult(std::make_unique<WifiSelectionActivity>(renderer, mappedInput, true),
                           [this](const ActivityResult&) {
                             if (WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
                               fetchAndParseUrl(currentUrl);
                             } else {
                               state = WebReaderState::ERROR;
                               errorMessage = "WiFi Connection Failed";
                               requestUpdate();
                             }
                           });
  }
}

void ReadabilityBrowserActivity::promptUrlEntry() {
  state = WebReaderState::URL_INPUT;
  startActivityForResult(
      std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, "Enter Web URL", currentUrl, 256, InputType::Url),
      [this](const ActivityResult& result) {
        std::string text;
        if (const auto* kb = std::get_if<KeyboardResult>(&result.data)) {
          text = kb->text;
        }
        if (!text.empty()) {
          if (text.find("://") == std::string::npos) {
            text = "https://" + text;
          }
          currentUrl = text;
          fetchAndParseUrl(currentUrl);
        } else {
          if (article.valid) {
            state = WebReaderState::READING;
            requestUpdate();
          } else {
            onGoHome();
          }
        }
      });
}

void ReadabilityBrowserActivity::fetchAndParseUrl(const std::string& url) {
  state = WebReaderState::FETCHING;
  requestUpdate();

  std::string html;
  LOG_INF("WEB_READER", "Fetching article from: %s", url.c_str());

  if (!HttpDownloader::fetchUrl(url, html) || html.empty()) {
    state = WebReaderState::ERROR;
    errorMessage = "Failed to load URL";
    requestUpdate();
    return;
  }

  article = ReadabilityExtractor::extract(html);
  if (!article.valid) {
    state = WebReaderState::ERROR;
    errorMessage = "Could not extract readable article text";
    requestUpdate();
    return;
  }

  paginateContent();
  currentPage = 0;
  state = WebReaderState::READING;
  requestUpdate();
}

void ReadabilityBrowserActivity::paginateContent() {
  const auto scale = uiScaleSpec();
  const int screenW = renderer.getScreenWidth() - (MARGIN_X * 2);
  const int screenH = renderer.getScreenHeight() - (MARGIN_TOP + MARGIN_BOTTOM);

  TextWrapUtils::paginateParagraphs(renderer, scale.bodyFontId, article.paragraphs, screenW, screenH, pages);

  if (pages.empty() && !article.content.empty()) {
    pages.push_back(article.content);
  }
}

void ReadabilityBrowserActivity::saveArticleOffline() {
  if (!article.valid) return;

  if (!Storage.exists("/.crosspoint/web")) {
    Storage.mkdir("/.crosspoint/web");
  }

  std::string filename = "/.crosspoint/web/article_" + std::to_string(millis()) + ".txt";
  HalFile file;
  if (Storage.openFileForWrite("WEB", filename.c_str(), file)) {
    file.write(reinterpret_cast<const uint8_t*>(article.title.c_str()), article.title.length());
    file.write(reinterpret_cast<const uint8_t*>("\n\n"), 2);
    file.write(reinterpret_cast<const uint8_t*>(article.content.c_str()), article.content.length());
    file.close();
    LOG_INF("WEB_READER", "Saved article to %s", filename.c_str());
  }
}

void ReadabilityBrowserActivity::loop() {
  if (state == WebReaderState::WIFI_CONNECTING || state == WebReaderState::URL_INPUT) {
    return;
  }

  if (state == WebReaderState::ERROR) {
    if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      promptUrlEntry();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      onGoHome();
    }
    return;
  }

  if (state == WebReaderState::READING) {
    const int pageCount = std::max(1, static_cast<int>(pages.size()));

    // Edge and directional swipe gestures
    if (mappedInput.wasBackGesture() || mappedInput.wasHomeGesture()) {
      onGoHome();
      return;
    }

    const auto swipe = mappedInput.wasSwipe();
    if (swipe == MappedInputManager::SwipeDir::Right) {
      if (currentPage > 0) {
        currentPage--;
        requestUpdate();
      } else {
        onGoHome();
      }
      return;
    } else if (swipe == MappedInputManager::SwipeDir::Left || swipe == MappedInputManager::SwipeDir::Up) {
      if (currentPage + 1 < pageCount) {
        currentPage++;
        requestUpdate();
      }
      return;
    } else if (swipe == MappedInputManager::SwipeDir::Down) {
      if (currentPage > 0) {
        currentPage--;
        requestUpdate();
      }
      return;
    }

    buttonNavigator.onNext([this, pageCount] {
      if (currentPage + 1 < pageCount) {
        currentPage++;
        requestUpdate();
      }
    });

    buttonNavigator.onPrevious([this] {
      if (currentPage > 0) {
        currentPage--;
        requestUpdate();
      }
    });

    // Touch tap zones: Left 25% = Previous, Right 75% = Next
    int tx = 0, ty = 0;
    if (mappedInput.wasScreenTapped(tx, ty)) {
      const int screenW = renderer.getScreenWidth();
      if (ty < MARGIN_TOP) {
        // Tapped top header -> prompt new URL
        promptUrlEntry();
        return;
      } else if (ty > renderer.getScreenHeight() - MARGIN_BOTTOM) {
        // Tapped bottom bar -> Save
        saveArticleOffline();
        return;
      }

      if (tx < screenW / 3) {
        if (currentPage > 0) {
          currentPage--;
          requestUpdate();
        }
      } else {
        if (currentPage + 1 < static_cast<int>(pages.size())) {
          currentPage++;
          requestUpdate();
        }
      }
      return;
    }

    if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
      onGoHome();
    } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
      promptUrlEntry();
    }
  }
}

void ReadabilityBrowserActivity::render(RenderLock&&) {
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const auto scale = uiScaleSpec();

  renderer.clearScreen();

  if (state == WebReaderState::FETCHING) {
    Rect popupRect = GUI.drawPopup(renderer, tr(STR_LOADING));
    GUI.fillPopupProgress(renderer, popupRect, 65);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  if (state == WebReaderState::ERROR) {
    GUI.drawHeader(renderer, Rect{0, 0, pageWidth, 40}, "Error");
    renderer.drawCenteredText(scale.bodyFontId, pageHeight / 2 - 20, errorMessage.c_str(), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(scale.smallFontId, pageHeight / 2 + 20, "Press Confirm to enter URL, Back to Exit");
    renderer.displayBuffer(HalDisplay::HALF_REFRESH);
    return;
  }

  if (state == WebReaderState::READING) {
    // Header
    std::string headerTitle = article.title.empty() ? "Web Article" : article.title;
    if (headerTitle.length() > 40) {
      headerTitle.erase(37);
      headerTitle += "...";
    }
    GUI.drawHeader(renderer, Rect{0, 0, pageWidth, 40}, headerTitle.c_str());

    // Page text
    const int lineHeight = renderer.getLineHeight(scale.bodyFontId) + 6;
    if (currentPage < pages.size()) {
      int y = MARGIN_TOP + 10;
      std::string text = pages[currentPage];
      size_t pos = 0;
      while (pos < text.length()) {
        size_t next = text.find('\n', pos);
        std::string line = (next == std::string::npos) ? text.substr(pos) : text.substr(pos, next - pos);
        if (!line.empty()) {
          renderer.drawText(scale.bodyFontId, MARGIN_X, y, line.c_str(), true);
        }
        y += lineHeight;
        if (next == std::string::npos) break;
        pos = next + 1;
      }
    }

    // Footer page number
    char pageInfo[32];
    snprintf(pageInfo, sizeof(pageInfo), "%zu / %zu", currentPage + 1, std::max(size_t(1), pages.size()));
    renderer.drawCenteredText(scale.smallFontId, pageHeight - 20, pageInfo);

    const auto labels = mappedInput.mapLabels("Home", "Enter URL", "Prev", "Next");
    GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
  }
}
