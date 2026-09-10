#include <gtest/gtest.h>

#include <string>

// Test standalone implementation of getCachePathForBook logic
namespace {
std::string getCachePathForBook(const std::string& bookPath, const char* prefix = "book") {
  const size_t hash = std::hash<std::string>{}(bookPath);
  return std::string("/.crosspoint/") + prefix + "_" + std::to_string(hash);
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
  const size_t totalPages = 5;
  // Page indices: 0, 1, 2, 3, 4.
  // When reading the last page (index 4):
  size_t currentPage = 4;
  bool atEnd = (currentPage >= totalPages);
  EXPECT_FALSE(atEnd);  // Final page must NOT trigger end-of-book sentinel yet!

  // Turning forward past the last page:
  currentPage++;
  atEnd = (currentPage >= totalPages);
  EXPECT_TRUE(atEnd);   // Now sentinel is reached!

  // Turning back from end-of-book returns to last valid page:
  size_t returnPage = totalPages > 0 ? totalPages - 1 : 0;
  EXPECT_EQ(returnPage, 4u);
}

TEST(ReaderUtilsTest, SkipPagesBoundsClamping) {
  const size_t totalPages = 10;
  size_t currentPage = 0;

  auto skip = [&](int amount) {
    int newPage = static_cast<int>(currentPage) + amount;
    if (newPage < 0) newPage = 0;
    if (newPage > static_cast<int>(totalPages)) newPage = static_cast<int>(totalPages);
    currentPage = static_cast<size_t>(newPage);
  };

  skip(5);
  EXPECT_EQ(currentPage, 5u);

  skip(20);
  EXPECT_EQ(currentPage, 10u);  // Clamped to end-of-book sentinel

  skip(-50);
  EXPECT_EQ(currentPage, 0u);   // Clamped to first page
}
