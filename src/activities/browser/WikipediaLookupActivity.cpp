#include "WikipediaLookupActivity.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <GfxRenderer.h>
#include <HalDisplay.h>
#include <I18n.h>
#include <Logging.h>
#include <WiFi.h>

#include "MappedInputManager.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UIScale.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "network/HttpDownloader.h"
#include "util/TextWrapUtils.h"
#include "util/UrlUtils.h"

WikipediaLookupActivity::WikipediaLookupActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                                 std::string term)
    : Activity("WikipediaLookup", renderer, mappedInput), searchTerm(std::move(term)) {}

void WikipediaLookupActivity::onEnter() {
  Activity::onEnter();
  loading = true;
  failed = false;
  errorMessage.clear();

  if (searchTerm.empty()) {
    startActivityForResult(
        std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, "Search Wikipedia", "", 64),
        [this](const ActivityResult& result) {
          std::string text;
          if (const auto* kb = std::get_if<KeyboardResult>(&result.data)) {
            text = kb->text;
          }
          if (!text.empty()) {
            searchTerm = text;
            fetchSummary(searchTerm);
          } else {
            onGoHome();
          }
        });
  } else {
    fetchSummary(searchTerm);
  }
}

void WikipediaLookupActivity::onExit() { Activity::onExit(); }

void WikipediaLookupActivity::fetchSummary(const std::string& term) {
  loading = true;
  failed = false;
  requestUpdate();

  // Replace spaces with underscores
  std::string encodedTerm = term;
  for (char& c : encodedTerm) {
    if (c == ' ') c = '_';
  }

  std::string url = "https://en.wikipedia.org/api/rest_v1/page/summary/" + encodedTerm;
  LOG_INF("WIKI", "Fetching Wikipedia summary: %s", url.c_str());

  std::string json;
  if (!HttpDownloader::fetchUrl(url, json) || json.empty()) {
    loading = false;
    failed = true;
    errorMessage = "Article not found on Wikipedia";
    requestUpdate();
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    loading = false;
    failed = true;
    errorMessage = "Failed to parse Wikipedia response";
    requestUpdate();
    return;
  }

  articleTitle = doc["title"] | term;
  description = doc["description"] | "";
  extract = doc["extract"] | "No summary extract available.";

  loading = false;
  failed = false;
  requestUpdate();
}

void WikipediaLookupActivity::loop() {
  if (loading) return;

  // Edge & directional swipe gestures
  if (mappedInput.wasBackGesture() || mappedInput.wasHomeGesture()) {
    onGoHome();
    return;
  }

  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Right || swipe == MappedInputManager::SwipeDir::Down) {
    onGoHome();
    return;
  }

  // Touch taps
  int tx = 0, ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    if (ty < 50) {
      // Tap header -> Exit Home
      onGoHome();
      return;
    } else if (ty >= renderer.getScreenHeight() - 50) {
      // Tap footer -> search new term
      startActivityForResult(
          std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, "Search Wikipedia", "", 64),
          [this](const ActivityResult& result) {
            std::string text;
            if (const auto* kb = std::get_if<KeyboardResult>(&result.data)) {
              text = kb->text;
            }
            if (!text.empty()) {
              searchTerm = text;
              fetchSummary(searchTerm);
            }
          });
      return;
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoHome();
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    startActivityForResult(
        std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, "Search Wikipedia", "", 64),
        [this](const ActivityResult& result) {
          std::string text;
          if (const auto* kb = std::get_if<KeyboardResult>(&result.data)) {
            text = kb->text;
          }
          if (!text.empty()) {
            searchTerm = text;
            fetchSummary(searchTerm);
          }
        });
  }
}

void WikipediaLookupActivity::render(RenderLock&&) {
  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();
  const auto scale = uiScaleSpec();

  renderer.clearScreen();

  GUI.drawHeader(renderer, Rect{0, 0, pageWidth, 40}, "Wikipedia Encyclopedia");

  if (loading) {
    Rect popupRect = GUI.drawPopup(renderer, tr(STR_LOADING));
    GUI.fillPopupProgress(renderer, popupRect, 60);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  if (failed) {
    renderer.drawCenteredText(scale.bodyFontId, pageHeight / 2 - 20, errorMessage.c_str(), true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(scale.smallFontId, pageHeight / 2 + 20, "Press Confirm to search again, Back to Exit");
    renderer.displayBuffer(HalDisplay::HALF_REFRESH);
    return;
  }

  // Draw encyclopedic card
  int y = 60;
  renderer.drawText(scale.titleFontId, 24, y, articleTitle.c_str(), true, EpdFontFamily::BOLD);
  y += 28;

  if (!description.empty()) {
    renderer.drawText(scale.smallFontId, 24, y, description.c_str(), true, EpdFontFamily::ITALIC);
    y += 24;
  }

  // Draw separator line
  renderer.drawLine(24, y, pageWidth - 24, y);
  y += 16;

  // Render extract with wrapping
  TextWrapUtils::drawWrappedParagraph(renderer, scale.bodyFontId, 24, y, pageWidth - 48,
                                      (pageHeight - 50) - y, extract, EpdFontFamily::REGULAR, 6);

  const auto labels = mappedInput.mapLabels("Exit", "New Search", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(HalDisplay::HALF_REFRESH);
}
