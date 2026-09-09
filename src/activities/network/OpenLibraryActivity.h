#pragma once

#include <string>
#include <vector>

#include "activities/Activity.h"

struct OpenLibraryBook {
  std::string title;
  std::string author;
  std::string publishYear;
  std::string iaId;        // Internet Archive item ID for direct EPUB download
  std::string openLibKey;  // /works/OL...
};

/**
 * @brief Public Library Search & Direct E-Book Downloader via Open Library & Internet Archive.
 *
 * Provides instant on-device searching across millions of public library books,
 * downloading DRM-free EPUBs directly into the device library over Wi-Fi.
 */
class OpenLibraryActivity final : public Activity {
 public:
  explicit OpenLibraryActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string initialQuery = "");
  ~OpenLibraryActivity() override = default;

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void promptSearch();
  void performSearch(const std::string& query);
  void downloadSelectedBook(size_t index);

  std::string currentQuery_;
  std::string statusMessage_;
  std::vector<OpenLibraryBook> results_;
  int selectedIndex_ = 0;
  bool isLoading_ = false;
  bool isDownloading_ = false;
  int downloadProgress_ = 0;
};
