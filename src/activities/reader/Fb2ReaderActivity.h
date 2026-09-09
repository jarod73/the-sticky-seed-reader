#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ReaderActivity.h"
#include "util/TextWrapUtils.h"

/**
 * @brief FictionBook 2.0 (.fb2, .fb2.zip) XML e-book reader.
 *
 * Parses structured FictionBook XML markup into chapters, headings, and
 * reflowable paragraphs on E-Ink.
 */
class Fb2ReaderActivity final : public ReaderActivity {
 public:
  Fb2ReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                    bool allowFastInitialRefresh = false);
  ~Fb2ReaderActivity() override = default;

  bool loadBook() override;
  std::string getBookTitle() const override;
  std::string getBookAuthor() const override { return author_; }
  bool pageTurn(bool isForward) override;
  bool isAtEndOfBook() const override;
  void renderBook() override;

 private:
  bool parseFb2Xml(const std::string& xmlContent);
  void paginate();

  std::string title_;
  std::string author_;
  std::vector<std::string> paragraphs_;
  std::vector<std::string> pages_;
  size_t currentPage_ = 0;
  bool isLoaded_ = false;
};
