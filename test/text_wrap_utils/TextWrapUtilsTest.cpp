#include <gtest/gtest.h>

#include "util/TextWrapUtils.h"

TEST(TextWrapUtilsTest, WrapToCharCountEmpty) {
  auto lines = TextWrapUtils::wrapToCharCount("", 20);
  EXPECT_TRUE(lines.empty());
}

TEST(TextWrapUtilsTest, WrapToCharCountShort) {
  auto lines = TextWrapUtils::wrapToCharCount("Hello World", 20);
  ASSERT_EQ(lines.size(), 1u);
  EXPECT_EQ(lines[0], "Hello World");
}

TEST(TextWrapUtilsTest, WrapToCharCountMultipleWords) {
  std::string text = "The quick brown fox jumps over the lazy dog";
  auto lines = TextWrapUtils::wrapToCharCount(text, 15);
  ASSERT_GE(lines.size(), 2u);

  std::string reconstructed;
  for (size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) reconstructed += " ";
    reconstructed += lines[i];
  }
  EXPECT_EQ(reconstructed, text);
}

TEST(TextWrapUtilsTest, WrapToCharCountMaxLinesLimit) {
  std::string text = "Line one words. Line two words. Line three words. Line four words.";
  auto lines = TextWrapUtils::wrapToCharCount(text, 15, 2);
  EXPECT_EQ(lines.size(), 2u);
}
