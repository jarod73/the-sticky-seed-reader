#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ReaderActivity.h"
#include "util/TextWrapUtils.h"

/**
 * @brief Lightweight PDF (.pdf) text stream extractor and reader on E-Ink.
 *
 * Scans PDF content streams and decodes text positioning operators (BT/ET, Tj, TJ)
 * into reflowable paragraphs with customizable fonts and margins.
 */
class PdfReaderActivity final : public ReaderActivity {
 public:
  PdfReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                    bool allowFastInitialRefresh = false);
  ~PdfReaderActivity() override = default;

  bool loadBook() override;
  std::string getBookTitle() const override;
  bool pageTurn(bool isForward) override;
  bool isAtEndOfBook() const override;
  void renderBook() override;

 private:
  bool extractPdfText();
  void paginate();

  std::vector<std::string> paragraphs_;
  std::vector<std::string> pages_;
  size_t currentPage_ = 0;
  bool isLoaded_ = false;
};
