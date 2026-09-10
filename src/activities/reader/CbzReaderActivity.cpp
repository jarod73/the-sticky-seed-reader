#include "CbzReaderActivity.h"

#include <Bitmap.h>
#include <Epub/converters/JpegToFramebufferConverter.h>
#include <Epub/converters/PngToFramebufferConverter.h>
#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>
#include <Memory.h>
#include <ZipFile.h>

#include <algorithm>
#include <cmath>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "ProgressFile.h"
#include "ReaderUtils.h"
#include "components/UITheme.h"
#include "fontIds.h"

namespace {
constexpr char TEMP_CBZ_PAGE_PATH[] = "/.cbz_page.tmp";
}

CbzReaderActivity::CbzReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                                     const bool allowFastInitialRefresh)
    : ReaderActivity("CbzReader", renderer, mappedInput, std::move(bookPath), allowFastInitialRefresh) {}

CbzReaderActivity::~CbzReaderActivity() { Storage.remove(TEMP_CBZ_PAGE_PATH); }

bool CbzReaderActivity::loadBook() {
  if (indexArchive()) {
    loadProgress();
    return true;
  }
  return false;
}

std::string CbzReaderActivity::getBookTitle() const {
  const size_t slash = bookPath.find_last_of('/');
  return (slash != std::string::npos) ? bookPath.substr(slash + 1) : bookPath;
}

std::string CbzReaderActivity::getCachePath() const {
  return ReaderUtils::getCachePathForBook(bookPath, "comic");
}

bool CbzReaderActivity::pageTurn(const bool isForward) {
  if (isForward) {
    if (currentPage_ < pageEntries_.size()) {
      currentPage_++;
      requestUpdate();
      return true;
    }
  } else {
    if (currentPage_ > 0) {
      currentPage_--;
      requestUpdate();
      return true;
    }
  }
  return false;
}

bool CbzReaderActivity::skipPages(const int amount) {
  int newPage = static_cast<int>(currentPage_) + amount;
  if (newPage < 0) newPage = 0;
  if (newPage > static_cast<int>(pageEntries_.size())) newPage = static_cast<int>(pageEntries_.size());
  if (newPage != static_cast<int>(currentPage_)) {
    currentPage_ = static_cast<size_t>(newPage);
    requestUpdate();
    return true;
  }
  return false;
}

bool CbzReaderActivity::isAtEndOfBook() const {
  return !pageEntries_.empty() && (currentPage_ >= pageEntries_.size());
}

void CbzReaderActivity::onReturnFromEndOfBook() {
  currentPage_ = pageEntries_.empty() ? 0 : pageEntries_.size() - 1;
}

void CbzReaderActivity::saveProgress() const {
  if (pageEntries_.empty()) return;
  const std::string cachePath = getCachePath();
  ReaderUtils::setupCacheDir(cachePath);

  uint8_t data[4];
  data[0] = static_cast<uint8_t>(currentPage_ & 0xFF);
  data[1] = static_cast<uint8_t>((currentPage_ >> 8) & 0xFF);
  data[2] = static_cast<uint8_t>((currentPage_ >> 16) & 0xFF);
  data[3] = static_cast<uint8_t>((currentPage_ >> 24) & 0xFF);
  if (!ProgressFile::writeAtomic(cachePath, data, sizeof(data))) {
    LOG_ERR("CBZ", "Failed to save progress: page %zu", currentPage_);
  }
}

void CbzReaderActivity::loadProgress() {
  const std::string cachePath = getCachePath();
  HalFile f;
  if (Storage.openFileForRead("CBZ", cachePath + "/progress.bin", f)) {
    uint8_t data[4];
    if (f.read(data, 4) == 4) {
      currentPage_ = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
      if (!pageEntries_.empty() && currentPage_ >= pageEntries_.size()) {
        currentPage_ = pageEntries_.size() - 1;
      }
      LOG_DBG("CBZ", "Loaded progress: page %zu/%zu", currentPage_ + 1, pageEntries_.size());
    }
  }
}

bool CbzReaderActivity::indexArchive() {
  pageEntries_.clear();
  ZipFile zip(bookPath);

  zip.enumerateFilePaths([this](std::string_view path) {
    if (FsHelpers::hasJpgExtension(path) || FsHelpers::hasPngExtension(path) || FsHelpers::hasBmpExtension(path) ||
        FsHelpers::hasGifExtension(path)) {
      // Ignore macOS metadata / resource forks
      if (path.find("__MACOSX") == std::string_view::npos && path.find("/.") == std::string_view::npos) {
        pageEntries_.emplace_back(path);
      }
    }
  });

  std::sort(pageEntries_.begin(), pageEntries_.end(), FsHelpers::naturalLess);
  LOG_INF("CBZ", "Indexed %zu comic pages in: %s", pageEntries_.size(), bookPath.c_str());
  return !pageEntries_.empty();
}

bool CbzReaderActivity::renderPageImage(const std::string& tempPath, const std::string& originalExt) {
  const int screenWidth = renderer.getScreenWidth();
  const int maxImageHeight = renderer.getScreenHeight() - 24;

  if (FsHelpers::hasJpgExtension(originalExt)) {
    ImageDimensions dimensions{};
    if (!JpegToFramebufferConverter::getDimensionsStatic(tempPath, dimensions) || dimensions.width <= 0 ||
        dimensions.height <= 0) {
      return false;
    }

    const float scale = std::min(static_cast<float>(screenWidth) / dimensions.width,
                                 static_cast<float>(maxImageHeight) / dimensions.height);
    const int width = std::min(screenWidth, static_cast<int>(dimensions.width * scale));
    const int height = std::min(maxImageHeight, static_cast<int>(dimensions.height * scale));
    const RenderConfig config{(screenWidth - width) / 2, (maxImageHeight - height) / 2, width, height};

    JpegToFramebufferConverter converter;
    return converter.decodeToFramebuffer(tempPath, renderer, config);
  }

  if (FsHelpers::hasPngExtension(originalExt)) {
    ImageDimensions dimensions{};
    if (!PngToFramebufferConverter::getDimensionsStatic(tempPath, dimensions) || dimensions.width <= 0 ||
        dimensions.height <= 0) {
      return false;
    }

    const float scale = std::min(static_cast<float>(screenWidth) / dimensions.width,
                                 static_cast<float>(maxImageHeight) / dimensions.height);
    const int width = std::min(screenWidth, static_cast<int>(dimensions.width * scale));
    const int height = std::min(maxImageHeight, static_cast<int>(dimensions.height * scale));
    const RenderConfig config{(screenWidth - width) / 2, (maxImageHeight - height) / 2, width, height};

    PngToFramebufferConverter converter;
    return converter.decodeToFramebuffer(tempPath, renderer, config);
  }

  if (FsHelpers::hasBmpExtension(originalExt)) {
    HalFile file;
    if (Storage.openFileForRead("CBZ", tempPath, file)) {
      Bitmap bitmap(file, true);
      if (bitmap.parseHeaders() == BmpReaderError::Ok) {
        int x = 0;
        int y = 0;
        if (bitmap.getWidth() > screenWidth || bitmap.getHeight() > maxImageHeight) {
          const float ratio = static_cast<float>(bitmap.getWidth()) / static_cast<float>(bitmap.getHeight());
          const float screenRatio = static_cast<float>(screenWidth) / static_cast<float>(maxImageHeight);
          if (ratio > screenRatio) {
            x = 0;
            y = std::round((static_cast<float>(maxImageHeight) - static_cast<float>(screenWidth) / ratio) / 2);
          } else {
            x = std::round((static_cast<float>(screenWidth) - static_cast<float>(maxImageHeight) * ratio) / 2);
            y = 0;
          }
        } else {
          x = (screenWidth - bitmap.getWidth()) / 2;
          y = (maxImageHeight - bitmap.getHeight()) / 2;
        }

        renderer.drawBitmap(bitmap, x, y, screenWidth, maxImageHeight, 0, 0);
        file.close();
        return true;
      }
      file.close();
    }
  }

  return false;
}

void CbzReaderActivity::renderBook() {
  renderer.clearScreen(0xFF);

  if (pageEntries_.empty()) {
    renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() / 2, "No comic pages found in archive", true,
                              EpdFontFamily::BOLD);
    renderer.displayBuffer();
    return;
  }

  const std::string& currentEntry = pageEntries_[currentPage_];

  // Extract comic page from zip archive into temporary file on storage
  ZipFile zip(bookPath);
  HalFile outFile;
  bool extractSuccess = false;

  Storage.remove(TEMP_CBZ_PAGE_PATH);
  if (Storage.openFileForWrite("CBZ", TEMP_CBZ_PAGE_PATH, outFile)) {
    extractSuccess = zip.readFileToStream(currentEntry.c_str(), outFile, 4096);
    outFile.close();
  }

  if (extractSuccess) {
    if (!renderPageImage(TEMP_CBZ_PAGE_PATH, currentEntry)) {
      renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() / 2, "Error decoding comic page", true);
    }
    Storage.remove(TEMP_CBZ_PAGE_PATH);
  } else {
    renderer.drawCenteredText(UI_12_FONT_ID, renderer.getScreenHeight() / 2, "Failed to extract page from archive",
                              true);
  }

  // Draw Page Number Footer / Status Bar
  const float progress = pageEntries_.empty() ? 0.0f : ((currentPage_ + 1) * 100.0f / pageEntries_.size());
  std::string barTitle;
  if (SETTINGS.statusBarSpec().showsTitle()) {
    barTitle = getBookTitle();
  }
  GUI.drawStatusBar(renderer, progress, static_cast<int>(currentPage_ + 1), static_cast<int>(pageEntries_.size()),
                    barTitle);

  ReaderUtils::displayWithRefreshCycle(renderer, pagesUntilFullRefresh);

  saveProgress();
}

ScreenshotInfo CbzReaderActivity::getScreenshotInfo() const {
  ScreenshotInfo info;
  const std::string t = getBookTitle();
  snprintf(info.title, sizeof(info.title), "%s", t.c_str());
  info.currentPage = static_cast<int>(currentPage_ + 1);
  info.totalPages = static_cast<int>(pageEntries_.size());
  info.progressPercent =
      pageEntries_.empty() ? 0 : static_cast<int>((currentPage_ + 1) * 100.0f / pageEntries_.size() + 0.5f);
  if (info.progressPercent > 100) info.progressPercent = 100;
  return info;
}
