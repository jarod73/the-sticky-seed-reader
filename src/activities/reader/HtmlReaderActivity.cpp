#include "HtmlReaderActivity.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

HtmlReaderActivity::HtmlReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                                       const bool allowFastInitialRefresh)
    : ReaderActivity("HtmlReader", renderer, mappedInput, std::move(bookPath), allowFastInitialRefresh) {}

bool HtmlReaderActivity::loadBook() {
  if (loadAndParseHtml()) {
    paginate();
    return !pages_.empty();
  }
  return false;
}

std::string HtmlReaderActivity::getBookTitle() const {
  if (!article_.title.empty()) {
    return article_.title;
  }
  const size_t slash = bookPath.find_last_of('/');
  return (slash != std::string::npos) ? bookPath.substr(slash + 1) : bookPath;
}

bool HtmlReaderActivity::pageTurn(const bool isForward) {
  if (isForward) {
    if (currentPage_ + 1 < pages_.size()) {
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

bool HtmlReaderActivity::isAtEndOfBook() const {
  return pages_.empty() || (currentPage_ + 1 >= pages_.size());
}

bool HtmlReaderActivity::loadAndParseHtml() {
  String content = Storage.readFile(bookPath.c_str());
  if (content.isEmpty()) {
    LOG_ERR("HTML", "Failed to read HTML file: %s", bookPath.c_str());
    return false;
  }

  std::string htmlStr(content.c_str(), content.length());
  article_ = ReadabilityExtractor::extract(htmlStr);
  isLoaded_ = !article_.paragraphs.empty();
  LOG_INF("HTML", "Parsed HTML article: %s (%zu paragraphs)", article_.title.c_str(), article_.paragraphs.size());
  return isLoaded_;
}

void HtmlReaderActivity::paginate() {
  pages_.clear();
  if (!isLoaded_) return;

  const int fontId = SETTINGS.getReaderFontId();
  const int screenWidth = renderer.getScreenWidth();
  const int screenHeight = renderer.getScreenHeight();
  const int contentWidth = screenWidth - 48;
  const int contentHeight = screenHeight - 64;

  TextWrapUtils::paginateParagraphs(renderer, fontId, article_.paragraphs, contentWidth, contentHeight, pages_);

  if (currentPage_ >= pages_.size() && !pages_.empty()) {
    currentPage_ = pages_.size() - 1;
  }
}

void HtmlReaderActivity::renderBook() {
  renderer.clearScreen(0xFF);

  if (!isLoaded_ || pages_.empty()) {
    renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() / 2, "No readable content found in HTML", true,
                              EpdFontFamily::BOLD);
    return;
  }

  const int fontId = SETTINGS.getReaderFontId();
  const int fontLineHeight = renderer.getLineHeight(fontId);
  const int leftMargin = 24;
  int currentY = 32;

  // Title on First Page
  if (currentPage_ == 0 && !article_.title.empty()) {
    renderer.drawText(fontId, leftMargin, currentY, article_.title.c_str(), true, EpdFontFamily::BOLD);
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

  // Draw Page Number Footer
  char footerBuf[64];
  snprintf(footerBuf, sizeof(footerBuf), "%zu / %zu", currentPage_ + 1, pages_.size());
  renderer.drawCenteredText(SMALL_FONT_ID, renderer.getScreenHeight() - 16, footerBuf, true);
}
