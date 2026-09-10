#include "PdfReaderActivity.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>

PdfReaderActivity::PdfReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                                     const bool allowFastInitialRefresh)
    : ParagraphReaderActivity("PdfReader", renderer, mappedInput, std::move(bookPath), allowFastInitialRefresh) {}

bool PdfReaderActivity::loadBook() {
  if (extractPdfText()) {
    paginate();
    loadProgress();
    return !pages_.empty();
  }
  return false;
}

bool PdfReaderActivity::extractPdfText() {
  HalFile file;
  if (!Storage.openFileForRead("PDF", bookPath.c_str(), file)) {
    LOG_ERR("PDF", "Failed to open PDF: %s", bookPath.c_str());
    return false;
  }

  paragraphs_.clear();
  paragraphs_.reserve(256);

  std::string currentParagraph;
  currentParagraph.reserve(256);

  constexpr size_t BUF_SIZE = 4096;
  std::vector<char> buffer(BUF_SIZE + 1);

  std::string carryOver;

  while (file.available()) {
    int bytesRead = file.read(reinterpret_cast<uint8_t*>(buffer.data()), BUF_SIZE);
    if (bytesRead <= 0) break;
    buffer[bytesRead] = '\0';

    std::string chunk = carryOver + buffer.data();
    carryOver.clear();

    size_t pos = 0;
    const size_t len = chunk.size();

    while (pos < len) {
      // Find (Text) Tj or TJ text blocks
      size_t openParen = chunk.find('(', pos);
      if (openParen == std::string::npos) {
        if (len - pos < 128) {
          carryOver = chunk.substr(pos);
        }
        break;
      }

      size_t closeParen = chunk.find(')', openParen);
      if (closeParen == std::string::npos) {
        carryOver = chunk.substr(openParen);
        break;
      }

      std::string textSnippet = chunk.substr(openParen + 1, closeParen - openParen - 1);

      // Filter out binary/unprintable streams
      const bool isPrintable = std::all_of(textSnippet.begin(), textSnippet.end(), [](char c) {
        const auto uc = static_cast<unsigned char>(c);
        return uc >= 32 || c == '\n' || c == '\r' || c == '\t';
      });

      if (isPrintable && !textSnippet.empty()) {
        if (!currentParagraph.empty()) currentParagraph += " ";
        currentParagraph += textSnippet;

        if (currentParagraph.size() > 400 || textSnippet.back() == '.' || textSnippet.back() == '!' ||
            textSnippet.back() == '?') {
          paragraphs_.push_back(std::move(currentParagraph));
          currentParagraph.clear();
          currentParagraph.reserve(256);
        }
      }

      pos = closeParen + 1;
    }
  }

  if (!currentParagraph.empty()) {
    paragraphs_.push_back(std::move(currentParagraph));
  }

  isLoaded_ = !paragraphs_.empty();
  LOG_INF("PDF", "Extracted %zu paragraphs from PDF: %s", paragraphs_.size(), bookPath.c_str());
  return isLoaded_;
}
