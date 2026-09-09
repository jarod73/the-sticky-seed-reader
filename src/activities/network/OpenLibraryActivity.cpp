#include "OpenLibraryActivity.h"

#include <ArduinoJson.h>
#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>

#include <algorithm>

#include "MappedInputManager.h"
#include "activities/util/KeyboardEntryActivity.h"
#include "components/UITheme.h"
#include "fontIds.h"
#include "network/HttpDownloader.h"
#include "util/UrlUtils.h"

namespace {
constexpr int ITEMS_PER_PAGE = 5;
}

OpenLibraryActivity::OpenLibraryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                         std::string initialQuery)
    : Activity("OpenLibrary", renderer, mappedInput), currentQuery_(std::move(initialQuery)) {}

void OpenLibraryActivity::onEnter() {
  Activity::onEnter();
  if (currentQuery_.empty()) {
    promptSearch();
  } else {
    performSearch(currentQuery_);
  }
}

void OpenLibraryActivity::onExit() {
  Activity::onExit();
}

void OpenLibraryActivity::promptSearch() {
  startActivityForResult(
      std::make_unique<KeyboardEntryActivity>(renderer, mappedInput, "Search Public Library (Open Library)", "", 64),
      [this](const ActivityResult& result) {
        std::string text;
        if (const auto* kb = std::get_if<KeyboardResult>(&result.data)) {
          text = kb->text;
        }
        if (!text.empty()) {
          currentQuery_ = text;
          performSearch(currentQuery_);
        } else if (results_.empty()) {
          onGoHome();
        }
      });
}

void OpenLibraryActivity::performSearch(const std::string& query) {
  isLoading_ = true;
  isDownloading_ = false;
  statusMessage_ = "Searching public library catalog...";
  results_.clear();
  selectedIndex_ = 0;
  requestUpdate();

  std::string encodedQuery = query;
  for (char& c : encodedQuery) {
    if (c == ' ') c = '+';
  }

  const std::string url = "https://openlibrary.org/search.json?q=" + encodedQuery +
                          "&limit=10&fields=title,author_name,first_publish_year,ia,key";
  LOG_INF("OPENLIB", "Querying: %s", url.c_str());

  std::string response;
  if (!HttpDownloader::fetchUrl(url, response) || response.empty()) {
    isLoading_ = false;
    statusMessage_ = "Connection failed. Please check Wi-Fi.";
    requestUpdate();
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, response);
  if (err) {
    isLoading_ = false;
    statusMessage_ = "Failed to parse catalog response.";
    requestUpdate();
    return;
  }

  JsonArray docs = doc["docs"].as<JsonArray>();
  for (JsonObject item : docs) {
    OpenLibraryBook book;
    book.title = item["title"] | "Untitled";
    JsonArray authors = item["author_name"].as<JsonArray>();
    if (!authors.isNull() && authors.size() > 0) {
      book.author = authors[0].as<std::string>();
    } else {
      book.author = "Unknown Author";
    }

    if (item["first_publish_year"].is<int>()) {
      book.publishYear = std::to_string(item["first_publish_year"].as<int>());
    }

    book.openLibKey = item["key"] | "";

    JsonArray iaList = item["ia"].as<JsonArray>();
    if (!iaList.isNull() && iaList.size() > 0) {
      book.iaId = iaList[0].as<std::string>();
    }

    results_.push_back(std::move(book));
  }

  isLoading_ = false;
  if (results_.empty()) {
    statusMessage_ = "No public library books found for: " + query;
  } else {
    statusMessage_.clear();
  }

  requestUpdate();
}

void OpenLibraryActivity::downloadSelectedBook(size_t index) {
  if (index >= results_.size()) return;
  const auto& book = results_[index];

  if (book.iaId.empty()) {
    statusMessage_ = "Digital loan not available for this item";
    requestUpdate();
    return;
  }

  isDownloading_ = true;
  downloadProgress_ = 0;
  statusMessage_ = "Downloading EPUB from Internet Archive...";
  requestUpdate();

  // Destination path: /books/<clean_title>.epub
  if (!Storage.exists("/books")) {
    Storage.mkdir("/books");
  }

  std::string sanitizedTitle;
  for (char c : book.title) {
    if (isalnum(c) || c == ' ' || c == '_' || c == '-') {
      sanitizedTitle += c;
    }
  }
  if (sanitizedTitle.empty()) sanitizedTitle = book.iaId;

  const std::string destPath = "/books/" + sanitizedTitle + ".epub";
  const std::string downloadUrl = "https://archive.org/download/" + book.iaId + "/" + book.iaId + ".epub";

  LOG_INF("OPENLIB", "Downloading: %s -> %s", downloadUrl.c_str(), destPath.c_str());

  auto err = HttpDownloader::downloadToFile(
      downloadUrl, destPath,
      [this](size_t downloaded, size_t total) {
        if (total > 0) {
          downloadProgress_ = static_cast<int>((downloaded * 100) / total);
          requestUpdate();
        }
      });

  isDownloading_ = false;
  if (err == HttpDownloader::OK) {
    statusMessage_ = "Download complete! Opening book...";
    requestUpdate();
    delay(500);
    activityManager.goToReader(destPath, false);
  } else {
    statusMessage_ = "Download failed. Check connection.";
    requestUpdate();
  }
}

void OpenLibraryActivity::loop() {
  if (isLoading_ || isDownloading_) return;

  if (mappedInput.wasBackGesture() || mappedInput.wasHomeGesture() ||
      mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoHome();
    return;
  }

  int tx = 0, ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    if (ty < 50) {
      onGoHome();
      return;
    }
    if (ty >= renderer.getScreenHeight() - 50) {
      promptSearch();
      return;
    }

    // Item selection tap
    const int itemH = 68;
    const int startY = 60;
    for (size_t i = 0; i < results_.size() && i < ITEMS_PER_PAGE; ++i) {
      const int rowY = startY + i * itemH;
      if (ty >= rowY && ty < rowY + itemH) {
        selectedIndex_ = static_cast<int>(i);
        downloadSelectedBook(selectedIndex_);
        return;
      }
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    if (selectedIndex_ > 0) {
      selectedIndex_--;
      requestUpdate();
    }
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    if (selectedIndex_ + 1 < static_cast<int>(results_.size())) {
      selectedIndex_++;
      requestUpdate();
    }
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (!results_.empty()) {
      downloadSelectedBook(selectedIndex_);
    } else {
      promptSearch();
    }
  }
}

void OpenLibraryActivity::render(RenderLock&&) {
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();

  renderer.clearScreen(0xFF);
  GUI.drawHeader(renderer, Rect{0, 0, screenW, 40}, "Open Library Public Books");

  if (isLoading_) {
    Rect popupRect = GUI.drawPopup(renderer, "Searching Public Catalog...");
    GUI.fillPopupProgress(renderer, popupRect, 50);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  if (isDownloading_) {
    char progBuf[64];
    snprintf(progBuf, sizeof(progBuf), "Downloading: %d%%", downloadProgress_);
    Rect popupRect = GUI.drawPopup(renderer, progBuf);
    GUI.fillPopupProgress(renderer, popupRect, downloadProgress_);
    renderer.displayBuffer(HalDisplay::FAST_REFRESH);
    return;
  }

  if (!statusMessage_.empty()) {
    renderer.drawCenteredText(UI_10_FONT_ID, screenH / 2, statusMessage_.c_str(), true);
  }

  // Draw Book List Rows
  const int startY = 56;
  const int itemH = 68;
  for (size_t i = 0; i < results_.size() && i < ITEMS_PER_PAGE; ++i) {
    const auto& book = results_[i];
    const int rowY = startY + i * itemH;
    const bool isSelected = (static_cast<int>(i) == selectedIndex_);

    if (isSelected) {
      renderer.drawRoundedRect(16, rowY, screenW - 32, itemH - 8, 2, 8, true);
    }

    renderer.drawText(UI_12_FONT_ID, 28, rowY + 16, book.title.c_str(), true,
                      isSelected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);

    std::string sub = book.author;
    if (!book.publishYear.empty()) sub += " (" + book.publishYear + ")";
    if (!book.iaId.empty()) sub += " • [Direct EPUB]";
    renderer.drawText(SMALL_FONT_ID, 28, rowY + 38, sub.c_str(), true);
  }

  const auto labels = mappedInput.mapLabels("Exit", "Download", "Search", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
