#pragma once

#include <memory>
#include <string>

#include "ParagraphReaderActivity.h"
#include "util/ReadabilityExtractor.h"

/**
 * @brief Standalone HTML and Web Article reader for saved .html and .htm files.
 *
 * Utilizes readability extraction to isolate article content and renders
 * formatted, paginated chapters.
 */
class HtmlReaderActivity final : public ParagraphReaderActivity {
 public:
  HtmlReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                     bool allowFastInitialRefresh = false);
  ~HtmlReaderActivity() override = default;

  bool loadBook() override;

 private:
  bool loadAndParseHtml();
};
