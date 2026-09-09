#pragma once

#include <string>
#include <string_view>
#include <vector>

/**
 * Struct representing a parsed, clean, and readable web article.
 */
struct ReadabilityArticle {
  std::string title;                    ///< Extracted article or webpage title
  std::string byline;                   ///< Author / publication byline if available
  std::string content;                  ///< Full plain-text article content
  std::vector<std::string> paragraphs;  ///< Distinct paragraphs ready for e-ink pagination
  bool valid = false;                   ///< True if parsing succeeded and content was found
};

/**
 * ReadabilityExtractor
 *
 * A high-performance, low-memory HTML readability parser optimized for ESP32.
 *
 * Design & Architecture:
 * - Scans HTML using lightweight `std::string_view` without allocating huge DOM trees.
 * - Strips noise elements: `<script>`, `<style>`, `<noscript>`, `<nav>`, `<header>`, `<footer>`, `<aside>`, `<svg>`.
 * - Focuses on `<article>` or `<main>` landmark regions when present for cleaner text extraction.
 * - Converts HTML entities and preserves paragraph breaks for optimal e-paper layout.
 */
class ReadabilityExtractor {
 public:
  /**
   * Parses raw HTML into a structured ReadabilityArticle.
   *
   * @param html Raw HTML string (or string_view).
   * @return Extracted article with metadata and paragraphs.
   */
  static ReadabilityArticle extract(std::string_view html);

  /**
   * Extracts the page title from `<title>` tags.
   */
  static std::string extractTitle(std::string_view html);

  /**
   * Cleans raw HTML and converts it into formatted, readable plain text.
   */
  static std::string cleanContent(std::string_view html);
};
