#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ReaderActivity.h"
#include "util/ReadabilityExtractor.h"
#include "util/TextWrapUtils.h"

/**
 * @brief Standalone local HTML and saved web article reader on E-Ink.
 *
 * Cleans web pages using ReadabilityExtractor, stripping navigation bars,
 * scripts, and ads, and renders them as reflowable paginated book chapters.
 */
class HtmlReaderActivity final : public ReaderActivity {
 public:
  HtmlReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                     bool allowFastInitialRefresh = false);
  ~HtmlReaderActivity() override = default;

  bool loadBook() override;
  std::string getBookTitle() const override;
  bool pageTurn(bool isForward) override;
  bool isAtEndOfBook() const override;
  void renderBook() override;

 private:
  bool loadAndParseHtml();
  void paginate();

  ReadabilityArticle article_;
  std::vector<std::string> pages_;
  size_t currentPage_ = 0;
  bool isLoaded_ = false;
};
