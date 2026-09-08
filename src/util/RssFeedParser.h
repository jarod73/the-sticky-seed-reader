#pragma once

#include <string>
#include <string_view>
#include <vector>

/**
 * An individual article / post entry parsed from an RSS or Atom syndication feed.
 */
struct RssItem {
  std::string title;        ///< Headline or entry title
  std::string link;         ///< Canonical URL link to the full web article
  std::string description;  ///< Summary, synopsis, or snippet
  std::string pubDate;      ///< Publication date/timestamp
};

/**
 * Container for a parsed syndication feed.
 */
struct RssFeed {
  std::string title;            ///< Channel or publication title
  std::string description;      ///< Publication tagline or description
  std::vector<RssItem> items;   ///< List of parsed feed items
  bool valid = false;           ///< True if parsing successfully extracted at least one item
};

/**
 * RssFeedParser
 *
 * Lightweight, zero-dependency XML syndication parser supporting:
 * - RSS 2.0 (`<channel>`, `<item>`, `<title>`, `<link>`, `<description>`, `<pubDate>`)
 * - Atom 1.0 (`<feed>`, `<entry>`, `<title>`, `<link href="..."/>`, `<summary>`, `<published>`)
 * - CDATA blocks (`<![CDATA[...]]>`)
 * - HTML entity decoding in titles and summaries
 */
class RssFeedParser {
 public:
  /**
   * Parses raw XML syndication data into an RssFeed structure.
   *
   * @param xml Raw XML string (or string_view).
   * @param maxItems Maximum number of items to parse (default: 10).
   * @return Parsed RssFeed object.
   */
  static RssFeed parse(std::string_view xml, size_t maxItems = 10);
};
