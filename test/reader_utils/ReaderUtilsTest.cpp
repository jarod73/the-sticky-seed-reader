#include <gtest/gtest.h>

#include <string>

namespace {
inline std::string getCachePathForBook(const std::string& bookPath, const char* prefix = "book") {
  const size_t hash = std::hash<std::string>{}(bookPath);
  return std::string("/.crosspoint/") + prefix + "_" + std::to_string(hash);
}

inline size_t clampPageSkip(size_t currentPage, int amount, size_t totalPages) {
  int newPage = static_cast<int>(currentPage) + amount;
  if (newPage < 0) return 0;
  if (newPage > static_cast<int>(totalPages)) return totalPages;
  return static_cast<size_t>(newPage);
}

inline bool isAtEndOfBook(size_t currentPage, size_t totalPages) {
  return (totalPages > 0) && (currentPage >= totalPages);
}

inline size_t getReturnFromEndOfBookPage(size_t totalPages) {
  return totalPages > 0 ? totalPages - 1 : 0;
}
}  // namespace

TEST(ReaderUtilsTest, CachePathDeterminism) {
  const std::string path1 = "/sd/books/novel.fb2";
  const std::string path2 = "/sd/books/novel.fb2";
  EXPECT_EQ(getCachePathForBook(path1, "book"), getCachePathForBook(path2, "book"));
}

TEST(ReaderUtilsTest, CachePathDifferentiation) {
  const std::string path1 = "/sd/books/bookA.mobi";
  const std::string path2 = "/sd/books/bookB.mobi";
  EXPECT_NE(getCachePathForBook(path1, "book"), getCachePathForBook(path2, "book"));
}

TEST(ReaderUtilsTest, CachePathPrefix) {
  const std::string path = "/sd/comics/manga.cbz";
  std::string comicCache = getCachePathForBook(path, "comic");
  std::string bookCache = getCachePathForBook(path, "book");
  EXPECT_NE(comicCache, bookCache);
  EXPECT_EQ(comicCache.rfind("/.crosspoint/comic_", 0), 0u);
  EXPECT_EQ(bookCache.rfind("/.crosspoint/book_", 0), 0u);
}

TEST(ReaderUtilsTest, EndOfBookPageBounds) {
  // Page indices: 0, 1, 2, 3, 4 with totalPages = 5
  // When reading the last page (index 4):
  EXPECT_FALSE(isAtEndOfBook(4, 5));

  // Turning forward past the last page (index 5):
  EXPECT_TRUE(isAtEndOfBook(5, 5));

  // Beyond last page (index 6):
  EXPECT_TRUE(isAtEndOfBook(6, 5));

  // Turning back from end-of-book returns to last valid page:
  EXPECT_EQ(getReturnFromEndOfBookPage(5), 4u);
  EXPECT_EQ(getReturnFromEndOfBookPage(0), 0u);
}

TEST(ReaderUtilsTest, SkipPagesBoundsClamping) {
  EXPECT_EQ(clampPageSkip(0, 5, 10), 5u);
  EXPECT_EQ(clampPageSkip(5, 20, 10), 10u);
  EXPECT_EQ(clampPageSkip(5, -20, 10), 0u);
  EXPECT_EQ(clampPageSkip(0, -5, 10), 0u);
}
