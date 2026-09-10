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
  String content = Storage.readFile(bookPath.c_str());
  if (content.isEmpty()) {
    LOG_ERR("HTML", "Failed to read HTML file: %s", bookPath.c_str());
    return false;
  }

  std::string htmlStr(content.c_str(), content.length());
  auto article = ReadabilityExtractor::extract(htmlStr);
  title_ = std::move(article.title);
  paragraphs_ = std::move(article.paragraphs);
  isLoaded_ = !paragraphs_.empty();
  LOG_INF("HTML", "Parsed HTML article: %s (%zu paragraphs)", title_.c_str(), paragraphs_.size());
  return isLoaded_;
}
