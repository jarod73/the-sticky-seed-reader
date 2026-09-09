#include "ReadabilityExtractor.h"

#include <algorithm>
#include <cctype>

#include "HtmlToPlainText.h"

namespace {

/**
 * Case-insensitive substring search over string_view.
 * Avoids any dynamic memory allocation or string duplication.
 */
size_t findCaseInsensitive(std::string_view str, std::string_view sub, size_t pos = 0) {
  if (sub.empty() || pos >= str.size()) return std::string_view::npos;
  auto it = std::search(str.begin() + pos, str.end(), sub.begin(), sub.end(), [](char c1, char c2) {
    return std::tolower(static_cast<unsigned char>(c1)) == std::tolower(static_cast<unsigned char>(c2));
  });
  return (it != str.end()) ? std::distance(str.begin(), it) : std::string_view::npos;
}

/**
 * Checks if a given tag name matches any noise tag that should be filtered out.
 */
bool isNoiseTag(std::string_view tag) {
  return tag == "script" || tag == "style" || tag == "noscript" || tag == "nav" || tag == "header" || tag == "footer" ||
         tag == "aside" || tag == "svg" || tag == "form";
}

}  // namespace

std::string ReadabilityExtractor::extractTitle(std::string_view html) {
  size_t start = findCaseInsensitive(html, "<title>");
  if (start == std::string_view::npos) start = findCaseInsensitive(html, "<title ");
  if (start != std::string_view::npos) {
    size_t contentStart = html.find('>', start);
    if (contentStart != std::string_view::npos) {
      contentStart++;
      size_t end = findCaseInsensitive(html, "</title>", contentStart);
      if (end != std::string_view::npos) {
        std::string rawTitle(html.substr(contentStart, end - contentStart));
        return htmlToPlainText(rawTitle);
      }
    }
  }
  return "Web Article";
}

std::string ReadabilityExtractor::cleanContent(std::string_view html) {
  if (html.empty()) return "";

  // 1. Locate main/article boundary if available to isolate article body
  size_t articleStart = findCaseInsensitive(html, "<article");
  size_t articleEnd = std::string_view::npos;
  if (articleStart != std::string_view::npos) {
    size_t openTagEnd = html.find('>', articleStart);
    if (openTagEnd != std::string_view::npos) {
      articleStart = openTagEnd + 1;
      articleEnd = findCaseInsensitive(html, "</article>", articleStart);
    }
  } else {
    // Try <main> tag
    articleStart = findCaseInsensitive(html, "<main");
    if (articleStart != std::string_view::npos) {
      size_t openTagEnd = html.find('>', articleStart);
      if (openTagEnd != std::string_view::npos) {
        articleStart = openTagEnd + 1;
        articleEnd = findCaseInsensitive(html, "</main>", articleStart);
      }
    }
  }

  std::string_view targetSlice =
      (articleStart != std::string_view::npos)
          ? html.substr(articleStart, (articleEnd != std::string_view::npos) ? (articleEnd - articleStart)
                                                                             : (html.size() - articleStart))
          : html;

  // 2. Linear single-pass sanitizer: filter out <script>...</script>, <style>...</style>, etc.
  std::string cleanHtml;
  cleanHtml.reserve(std::min(targetSlice.size(), size_t(32768)));

  size_t i = 0;
  while (i < targetSlice.size()) {
    if (targetSlice[i] == '<') {
      size_t closeAngle = targetSlice.find('>', i);
      if (closeAngle == std::string_view::npos) break;

      // Inspect tag name
      size_t tagStart = i + 1;
      if (tagStart < closeAngle && targetSlice[tagStart] == '/') tagStart++;

      size_t tagEnd = tagStart;
      while (tagEnd < closeAngle && !std::isspace(static_cast<unsigned char>(targetSlice[tagEnd])) &&
             targetSlice[tagEnd] != '/') {
        tagEnd++;
      }

      std::string tagName(targetSlice.substr(tagStart, tagEnd - tagStart));
      std::transform(tagName.begin(), tagName.end(), tagName.begin(), [](unsigned char c) { return std::tolower(c); });

      if (isNoiseTag(tagName)) {
        // Skip entire tag block until closing tag e.g. </script>
        std::string closeTag = "</" + tagName + ">";
        size_t blockEnd = findCaseInsensitive(targetSlice, closeTag, closeAngle);
        if (blockEnd != std::string_view::npos) {
          i = blockEnd + closeTag.length();
          continue;
        } else {
          i = closeAngle + 1;
          continue;
        }
      }

      // Preserve benign HTML tags (like <p>, <br>, <h1>, <div>) for htmlToPlainText()
      cleanHtml.append(targetSlice.data() + i, closeAngle - i + 1);
      i = closeAngle + 1;
    } else {
      cleanHtml.push_back(targetSlice[i]);
      i++;
    }
  }

  return htmlToPlainText(cleanHtml);
}

ReadabilityArticle ReadabilityExtractor::extract(std::string_view html) {
  ReadabilityArticle article;
  if (html.empty()) return article;

  article.title = extractTitle(html);
  article.content = cleanContent(html);
  article.valid = !article.content.empty();

  // Split plain text into distinct paragraphs separated by blank lines
  size_t start = 0;
  while (start < article.content.length()) {
    size_t end = article.content.find("\n\n", start);
    if (end == std::string::npos) {
      std::string p = article.content.substr(start);
      if (!p.empty()) article.paragraphs.push_back(std::move(p));
      break;
    }
    std::string p = article.content.substr(start, end - start);
    if (!p.empty()) article.paragraphs.push_back(std::move(p));
    start = end + 2;
    while (start < article.content.length() && article.content[start] == '\n') start++;
  }

  return article;
}
