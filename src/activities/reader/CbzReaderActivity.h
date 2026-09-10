#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ReaderActivity.h"

/**
 * @brief High-performance Comic Book and Manga archive reader for .cbz, .cbr, and .zip files.
 *
 * Extracts and decodes sequential comic pages on demand directly into PSRAM,
 * rendering full-screen images on the 800x480 E-Ink display with automatic aspect-ratio
 * scaling and Floyd-Steinberg dithering.
 */
class CbzReaderActivity final : public ReaderActivity {
 public:
  CbzReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                    bool allowFastInitialRefresh = false);
  ~CbzReaderActivity() override;

  bool loadBook() override;
  std::string getBookTitle() const override;
  bool pageTurn(bool isForward) override;
  bool skipPages(int amount) override;
  bool isAtEndOfBook() const override;
  void onReturnFromEndOfBook() override;
  void renderBook() override;

  ScreenshotInfo getScreenshotInfo() const override;

 private:
  bool indexArchive();
  bool renderPageImage(const std::string& tempPath, const std::string& originalExt);
  void saveProgress() const;
  void loadProgress();
  std::string getCachePath() const;

  std::vector<std::string> pageEntries_;
  size_t currentPage_ = 0;
};
