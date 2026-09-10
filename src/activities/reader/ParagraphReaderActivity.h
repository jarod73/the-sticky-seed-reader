#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ReaderActivity.h"
#include "util/TextWrapUtils.h"

/**
 * @brief Base class for text/paragraph-based e-book readers (FB2, MOBI, HTML, PDF).
 *
 * Encapsulates reflowable text pagination, layout rendering, anti-aliasing,
 * page turn navigation, reading progress persistence, and end-of-book handling.
 */
class ParagraphReaderActivity : public ReaderActivity {
 protected:
  std::string title_;
  std::string author_;
  std::vector<std::string> paragraphs_;
  std::vector<std::string> pages_;
  size_t currentPage_ = 0;
  bool isLoaded_ = false;

  ParagraphReaderActivity(const char* name, GfxRenderer& renderer, MappedInputManager& mappedInput,
                          std::string bookPath, bool allowFastInitialRefresh = false);

  void paginate();
  void saveProgress() const;
  void loadProgress();
  std::string getCachePath() const;

 public:
  ~ParagraphReaderActivity() override = default;

  std::string getBookTitle() const override;
  std::string getBookAuthor() const override { return author_; }
  bool pageTurn(bool isForward) override;
  bool skipPages(int amount) override;
  bool isAtEndOfBook() const override;
  void onReturnFromEndOfBook() override;
  void renderBook() override;

  ScreenshotInfo getScreenshotInfo() const override;
};
