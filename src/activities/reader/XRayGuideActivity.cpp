#include "XRayGuideActivity.h"

#include <ArduinoJson.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>

#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "network/HttpDownloader.h"
#include "util/TextWrapUtils.h"

XRayGuideActivity::XRayGuideActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string term)
    : Activity("XRayGuide", renderer, mappedInput), targetTerm_(std::move(term)) {}

void XRayGuideActivity::onEnter() {
  Activity::onEnter();
  if (!targetTerm_.empty()) {
    fetchConceptDetails(targetTerm_);
  } else {
    isFailed_ = true;
    errorMessage_ = "No concept or character specified";
    requestUpdate();
  }
}

void XRayGuideActivity::onExit() {
  Activity::onExit();
}

void XRayGuideActivity::fetchConceptDetails(const std::string& term) {
  isLoading_ = true;
  isFailed_ = false;
  requestUpdate();

  std::string encoded = term;
  for (char& c : encoded) {
    if (c == ' ') c = '_';
  }

  const std::string url = "https://en.wikipedia.org/api/rest_v1/page/summary/" + encoded;
  LOG_INF("XRAY", "Fetching X-Ray dossier: %s", url.c_str());

  std::string response;
  if (!HttpDownloader::fetchUrl(url, response) || response.empty()) {
    isLoading_ = false;
    isFailed_ = true;
    conceptTitle_ = term;
    conceptSubtitle_ = "Character / Concept Guide";
    conceptSummary_ = "Detailed online summary unavailable. Connect to Wi-Fi for full encyclopedic character background.";
    requestUpdate();
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, response);
  if (err) {
    isLoading_ = false;
    isFailed_ = true;
    errorMessage_ = "Failed to parse character dossier";
    requestUpdate();
    return;
  }

  conceptTitle_ = doc["title"] | term;
  conceptSubtitle_ = doc["description"] | "Character / Concept";
  conceptSummary_ = doc["extract"] | "No background summary extract available.";

  isLoading_ = false;
  requestUpdate();
}

void XRayGuideActivity::loop() {
  if (isLoading_) return;

  int tx = 0, ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty) || mappedInput.wasBackGesture() || mappedInput.wasHomeGesture() ||
      mappedInput.wasReleased(MappedInputManager::Button::Back) ||
      mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    finish();
  }
}

void XRayGuideActivity::render(RenderLock&&) {
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();

  renderer.clearScreen(0xFF);

  // 1. Header Bar
  GUI.drawHeader(renderer, Rect{0, 0, screenW, 40}, "X-Ray: Character & Concept Guide");

  if (isLoading_) {
    renderer.drawCenteredText(UI_12_FONT_ID, screenH / 2 - 20, "Analyzing Context...", true, EpdFontFamily::BOLD);
    renderer.drawCenteredText(UI_10_FONT_ID, screenH / 2 + 15, targetTerm_.c_str(), true);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  // 2. Dossier Title & Subtitle Card
  int y = 56;
  renderer.drawText(UI_12_FONT_ID, 24, y, conceptTitle_.c_str(), true, EpdFontFamily::BOLD);
  y += 28;

  if (!conceptSubtitle_.empty()) {
    renderer.drawText(SMALL_FONT_ID, 24, y, conceptSubtitle_.c_str(), true, EpdFontFamily::ITALIC);
    y += 20;
  }

  renderer.drawLine(24, y, screenW - 24, y, true);
  y += 16;

  // 3. Render Formatted Extract
  TextWrapUtils::drawWrappedParagraph(renderer, UI_10_FONT_ID, 24, y, screenW - 48, (screenH - 50) - y,
                                      conceptSummary_, EpdFontFamily::REGULAR, 6);

  // 4. Footer Hint
  const auto labels = mappedInput.mapLabels("Return", "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(HalDisplay::HALF_REFRESH);
}
