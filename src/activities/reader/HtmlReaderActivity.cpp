#include "HtmlReaderActivity.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>

HtmlReaderActivity::HtmlReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                                       bool allowFastInitialRefresh)
    : ParagraphReaderActivity("HtmlReader", renderer, mappedInput, std::move(bookPath), allowFastInitialRefresh) {}

bool HtmlReaderActivity::loadBook() {
  if (loadAndParseHtml()) {
    paginate();
    loadProgress();
    return !pages_.empty();
  }
  return false;
}

bool HtmlReaderActivity::loadAndParseHtml() {
  HalFile file;
  if (!Storage.openFileForRead("HTML", bookPath.c_str(), file)) {
    LOG_ERR("HTML", "Failed to open HTML file: %s", bookPath.c_str());
    return false;
  }

  const size_t fileSize = file.size();
  std::string htmlStr;
  htmlStr.resize(fileSize);
  if (fileSize > 0 && file.read(reinterpret_cast<uint8_t*>(&htmlStr[0]), fileSize) != fileSize) {
    LOG_ERR("HTML", "Failed to read full HTML file: %s", bookPath.c_str());
    return false;
  }

  auto article = ReadabilityExtractor::extract(htmlStr);
  title_ = std::move(article.title);
  paragraphs_ = std::move(article.paragraphs);
  isLoaded_ = !paragraphs_.empty();
  LOG_INF("HTML", "Parsed HTML article: %s (%zu paragraphs)", title_.c_str(), paragraphs_.size());
  return isLoaded_;
}
