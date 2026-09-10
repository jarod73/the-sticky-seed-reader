#include "ParagraphReaderActivity.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "ProgressFile.h"
#include "ReaderUtils.h"
#include "components/UITheme.h"
#include "fontIds.h"

ParagraphReaderActivity::ParagraphReaderActivity(const char* name, GfxRenderer& renderer,
                                                 MappedInputManager& mappedInput, std::string bookPath,
                                                 const bool allowFastInitialRefresh)
    : ReaderActivity(name, renderer, mappedInput, std::move(bookPath), allowFastInitialRefresh) {}

std::string ParagraphReaderActivity::getBookTitle() const {
  if (!title_.empty()) {
    return title_;
  }
  const size_t slash = bookPath.find_last_of('/');
  return (slash != std::string::npos) ? bookPath.substr(slash + 1) : bookPath;
}

std::string ParagraphReaderActivity::getCachePath() const {
  return ReaderUtils::getCachePathForBook(bookPath, "book");
}

bool ParagraphReaderActivity::pageTurn(const bool isForward) {
  if (isForward) {
    if (currentPage_ < pages_.size()) {
      currentPage_++;
      requestUpdate();
      return true;
    }
  } else {
    if (currentPage_ > 0) {
      currentPage_--;
      requestUpdate();
      return true;
    }
  }
  return false;
}

bool ParagraphReaderActivity::skipPages(const int amount) {
  int newPage = static_cast<int>(currentPage_) + amount;
  if (newPage < 0) newPage = 0;
  if (newPage > static_cast<int>(pages_.size())) newPage = static_cast<int>(pages_.size());
  if (newPage != static_cast<int>(currentPage_)) {
    currentPage_ = static_cast<size_t>(newPage);
    requestUpdate();
    return true;
  }
  return false;
}

bool ParagraphReaderActivity::isAtEndOfBook() const {
  return isLoaded_ && !pages_.empty() && (currentPage_ >= pages_.size());
}

void ParagraphReaderActivity::onReturnFromEndOfBook() {
  currentPage_ = pages_.empty() ? 0 : pages_.size() - 1;
}

void ParagraphReaderActivity::saveProgress() const {
  if (pages_.empty()) return;
  const std::string cachePath = getCachePath();
  ReaderUtils::setupCacheDir(cachePath);

  uint8_t data[4];
  data[0] = static_cast<uint8_t>(currentPage_ & 0xFF);
  data[1] = static_cast<uint8_t>((currentPage_ >> 8) & 0xFF);
  data[2] = static_cast<uint8_t>((currentPage_ >> 16) & 0xFF);
  data[3] = static_cast<uint8_t>((currentPage_ >> 24) & 0xFF);
  if (!ProgressFile::writeAtomic(cachePath, data, sizeof(data))) {
    LOG_ERR("PRA", "Failed to save progress: page %zu", currentPage_);
  }
}

void ParagraphReaderActivity::loadProgress() {
  const std::string cachePath = getCachePath();
  HalFile f;
  if (Storage.openFileForRead("PRA", cachePath + "/progress.bin", f)) {
    uint8_t data[4];
    if (f.read(data, 4) == 4) {
      currentPage_ = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
      if (!pages_.empty() && currentPage_ >= pages_.size()) {
        currentPage_ = pages_.size() - 1;
      }
      LOG_DBG("PRA", "Loaded progress: page %zu/%zu", currentPage_ + 1, pages_.size());
    }
  }
}

void ParagraphReaderActivity::paginate() {
  pages_.clear();
  if (!isLoaded_) return;

  const int fontId = SETTINGS.getReaderFontId();
  const int screenWidth = renderer.getScreenWidth();
  const int screenHeight = renderer.getScreenHeight();
  const int contentWidth = screenWidth - 48;
  const int contentHeight = screenHeight - 64;

  TextWrapUtils::paginateParagraphs(renderer, fontId, paragraphs_, contentWidth, contentHeight, pages_);

  if (currentPage_ >= pages_.size() && !pages_.empty()) {
    currentPage_ = pages_.size() - 1;
  }
}

void ParagraphReaderActivity::renderBook() {
  renderer.clearScreen(0xFF);

  if (!isLoaded_ || pages_.empty()) {
    renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() / 2, "No readable text found", true,
                              EpdFontFamily::BOLD);
    renderer.displayBuffer();
    return;
  }

  const int fontId = SETTINGS.getReaderFontId();
  const int fontLineHeight = renderer.getLineHeight(fontId);
  const int leftMargin = 24;

  auto renderContent = [&]() {
    int currentY = 32;

    // Title Header on Page 1
    if (currentPage_ == 0 && !title_.empty()) {
      renderer.drawText(fontId, leftMargin, currentY, title_.c_str(), true, EpdFontFamily::BOLD);
      currentY += fontLineHeight + 12;
      renderer.drawLine(leftMargin, currentY, renderer.getScreenWidth() - 24, currentY, true);
      currentY += 16;
    }

    // Draw Page Lines
    const std::string& text = pages_[currentPage_];
    size_t pos = 0;
    while (pos < text.length()) {
      size_t next = text.find('\n', pos);
      std::string line = (next == std::string::npos) ? text.substr(pos) : text.substr(pos, next - pos);
      if (!line.empty()) {
        renderer.drawText(fontId, leftMargin, currentY, line.c_str(), true);
      }
      currentY += fontLineHeight + 4;
      if (next == std::string::npos) break;
      pos = next + 1;
    }
  };

  renderContent();

  // Draw Footer Page Number / Status Bar
  const float progress = pages_.empty() ? 0.0f : ((currentPage_ + 1) * 100.0f / pages_.size());
  std::string barTitle;
  if (SETTINGS.statusBarSpec().showsTitle()) {
    barTitle = getBookTitle();
  }
  GUI.drawStatusBar(renderer, progress, static_cast<int>(currentPage_ + 1), static_cast<int>(pages_.size()), barTitle);

  if (SETTINGS.textAntiAliasing) {
    ReaderUtils::displayBaseWithRefreshCycle(renderer, pagesUntilFullRefresh);
    ReaderUtils::renderAntiAliased(renderer, [&renderContent]() { renderContent(); });
  } else {
    ReaderUtils::displayWithRefreshCycle(renderer, pagesUntilFullRefresh);
  }

  saveProgress();
}

ScreenshotInfo ParagraphReaderActivity::getScreenshotInfo() const {
  ScreenshotInfo info;
  const std::string t = getBookTitle();
  snprintf(info.title, sizeof(info.title), "%s", t.c_str());
  info.currentPage = static_cast<int>(currentPage_ + 1);
  info.totalPages = static_cast<int>(pages_.size());
  info.progressPercent =
      pages_.empty() ? 0 : static_cast<int>((currentPage_ + 1) * 100.0f / pages_.size() + 0.5f);
  if (info.progressPercent > 100) info.progressPercent = 100;
  return info;
}
