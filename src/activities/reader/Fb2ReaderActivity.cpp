#include "Fb2ReaderActivity.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>
#include <ZipFile.h>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

Fb2ReaderActivity::Fb2ReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                                     const bool allowFastInitialRefresh)
    : ReaderActivity("Fb2Reader", renderer, mappedInput, std::move(bookPath), allowFastInitialRefresh) {}

bool Fb2ReaderActivity::loadBook() {
  std::string xmlData;
  if (FsHelpers::checkFileExtension(bookPath, ".fb2.zip") || FsHelpers::checkFileExtension(bookPath, ".zip")) {
    ZipFile zip(bookPath);
    zip.enumerateFilePaths([this, &zip, &xmlData](std::string_view path) {
      if (FsHelpers::checkFileExtension(path, ".fb2") && xmlData.empty()) {
        size_t sz = 0;
        uint8_t* raw = zip.readFileToMemory(std::string(path).c_str(), &sz);
        if (raw && sz > 0) {
          xmlData.assign(reinterpret_cast<char*>(raw), sz);
          free(raw);
        }
      }
    });
  } else {
    String content = Storage.readFile(bookPath.c_str());
    if (!content.isEmpty()) {
      xmlData.assign(content.c_str(), content.length());
    }
  }

  if (parseFb2Xml(xmlData)) {
    paginate();
    return !pages_.empty();
  }
  return false;
}

std::string Fb2ReaderActivity::getBookTitle() const {
  if (!title_.empty()) {
    return title_;
  }
  const size_t slash = bookPath.find_last_of('/');
  return (slash != std::string::npos) ? bookPath.substr(slash + 1) : bookPath;
}

bool Fb2ReaderActivity::pageTurn(const bool isForward) {
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

bool Fb2ReaderActivity::isAtEndOfBook() const { return pages_.empty() || (currentPage_ + 1 >= pages_.size()); }

bool Fb2ReaderActivity::parseFb2Xml(const std::string& xml) {
  if (xml.empty()) return false;

  paragraphs_.clear();
  paragraphs_.reserve(256);

  size_t pos = 0;
  const size_t len = xml.size();

  // Helper to extract text between opening and closing tags
  auto extractTag = [&](const std::string& openTag, const std::string& closeTag) -> std::string {
    size_t start = xml.find(openTag);
    if (start == std::string::npos) return "";
    start += openTag.length();
    size_t end = xml.find(closeTag, start);
    if (end == std::string::npos) return "";
    return xml.substr(start, end - start);
  };

  title_ = extractTag("<book-title>", "</book-title>");
  if (title_.empty()) title_ = extractTag("<title>", "</title>");

  // Extract <p> paragraphs
  while (pos < len) {
    size_t pStart = xml.find("<p>", pos);
    if (pStart == std::string::npos) break;
    pStart += 3;
    size_t pEnd = xml.find("</p>", pStart);
    if (pEnd == std::string::npos) break;

    std::string pText = xml.substr(pStart, pEnd - pStart);

    // Strip inline XML tags (e.g. <emphasis>, <strong>)
    std::string cleanText;
    cleanText.reserve(pText.size());
    bool inTag = false;
    for (char c : pText) {
      if (c == '<') {
        inTag = true;
      } else if (c == '>') {
        inTag = false;
      } else if (!inTag) {
        cleanText += c;
      }
    }

    if (!cleanText.empty()) {
      paragraphs_.push_back(std::move(cleanText));
    }
    pos = pEnd + 4;
  }

  isLoaded_ = !paragraphs_.empty();
  LOG_INF("FB2", "Parsed FB2: %s (%zu paragraphs)", title_.c_str(), paragraphs_.size());
  return isLoaded_;
}

void Fb2ReaderActivity::paginate() {
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

void Fb2ReaderActivity::renderBook() {
  renderer.clearScreen(0xFF);

  if (!isLoaded_ || pages_.empty()) {
    renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() / 2, "No readable text found in FB2", true,
                              EpdFontFamily::BOLD);
    return;
  }

  const int fontId = SETTINGS.getReaderFontId();
  const int fontLineHeight = renderer.getLineHeight(fontId);
  const int leftMargin = 24;
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

  // Draw Footer Page Number
  char footerBuf[64];
  snprintf(footerBuf, sizeof(footerBuf), "%zu / %zu", currentPage_ + 1, pages_.size());
  renderer.drawCenteredText(SMALL_FONT_ID, renderer.getScreenHeight() - 16, footerBuf, true);
}
