#include "Fb2ReaderActivity.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>
#include <ZipFile.h>

Fb2ReaderActivity::Fb2ReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                                     const bool allowFastInitialRefresh)
    : ParagraphReaderActivity("Fb2Reader", renderer, mappedInput, std::move(bookPath), allowFastInitialRefresh) {}

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
    loadProgress();
    return !pages_.empty();
  }
  return false;
}

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

  std::string firstName = extractTag("<first-name>", "</first-name>");
  std::string lastName = extractTag("<last-name>", "</last-name>");
  if (!firstName.empty() || !lastName.empty()) {
    author_ = firstName.empty() ? lastName : (lastName.empty() ? firstName : (firstName + " " + lastName));
  } else {
    author_ = extractTag("<author>", "</author>");
  }

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
