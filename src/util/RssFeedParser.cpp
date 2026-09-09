#include "RssFeedParser.h"

#include <algorithm>
#include <cctype>

#include "HtmlToPlainText.h"

namespace {

/**
 * Extracts child text between `<tag>` and `</tag>` within a bounded slice [startPos, endPos].
 * Unwraps CDATA blocks and decodes HTML entities.
 */
std::string extractTagContent(std::string_view xml, std::string_view tag, size_t startPos, size_t endPos) {
  std::string openTag = "<" + std::string(tag);
  size_t open = xml.find(openTag, startPos);
  if (open == std::string_view::npos || open >= endPos) return "";

  size_t contentStart = xml.find('>', open);
  if (contentStart == std::string_view::npos || contentStart >= endPos) return "";
  contentStart++;

  std::string closeTag = "</" + std::string(tag) + ">";
  size_t close = xml.find(closeTag, contentStart);
  if (close == std::string_view::npos || close > endPos) return "";

  std::string_view content = xml.substr(contentStart, close - contentStart);

  // Handle CDATA wrapper: <![CDATA[...]]>
  if (content.rfind("<![CDATA[", 0) == 0 && content.length() >= 12) {
    content = content.substr(9, content.length() - 12);
  }

  std::string contentStr(content);
  return htmlToPlainText(contentStr);
}

/**
 * Extracts URL links from either RSS format (<link>URL</link>) or Atom format (<link href="URL"... />).
 */
std::string extractLink(std::string_view xml, size_t startPos, size_t endPos) {
  size_t linkTag = xml.find("<link", startPos);
  if (linkTag == std::string_view::npos || linkTag >= endPos) return "";

  size_t tagEnd = xml.find('>', linkTag);
  if (tagEnd == std::string_view::npos || tagEnd > endPos) return "";

  std::string_view tagSlice = xml.substr(linkTag, tagEnd - linkTag + 1);

  // Check for Atom attribute format: href="..."
  size_t hrefPos = tagSlice.find("href=\"");
  if (hrefPos != std::string_view::npos) {
    size_t valStart = hrefPos + 6;
    size_t valEnd = tagSlice.find('"', valStart);
    if (valEnd != std::string_view::npos) {
      return std::string(tagSlice.substr(valStart, valEnd - valStart));
    }
  }

  // Check for single quotes: href='...'
  hrefPos = tagSlice.find("href='");
  if (hrefPos != std::string_view::npos) {
    size_t valStart = hrefPos + 6;
    size_t valEnd = tagSlice.find('\'', valStart);
    if (valEnd != std::string_view::npos) {
      return std::string(tagSlice.substr(valStart, valEnd - valStart));
    }
  }

  // Fallback to standard RSS child text: <link>URL</link>
  return extractTagContent(xml, "link", startPos, endPos);
}

}  // namespace

RssFeed RssFeedParser::parse(std::string_view xml, size_t maxItems) {
  RssFeed feed;
  if (xml.empty()) return feed;

  feed.items.reserve(std::min(maxItems, size_t(32)));

  // Extract channel/feed title
  feed.title = extractTagContent(xml, "title", 0, std::min(xml.size(), size_t(2048)));
  if (feed.title.empty()) feed.title = "News Headlines";

  // Scan items (<item> for RSS 2.0, <entry> for Atom 1.0)
  size_t pos = 0;
  while (feed.items.size() < maxItems && pos < xml.size()) {
    size_t itemStart = xml.find("<item", pos);
    bool isAtom = false;

    size_t entryStart = xml.find("<entry", pos);
    if (entryStart != std::string_view::npos && (itemStart == std::string_view::npos || entryStart < itemStart)) {
      itemStart = entryStart;
      isAtom = true;
    }

    if (itemStart == std::string_view::npos) break;

    size_t itemEnd = xml.find(isAtom ? "</entry>" : "</item>", itemStart);
    if (itemEnd == std::string_view::npos) break;

    RssItem item;
    item.title = extractTagContent(xml, "title", itemStart, itemEnd);
    item.description = extractTagContent(xml, isAtom ? "summary" : "description", itemStart, itemEnd);
    if (item.description.empty() && isAtom) {
      item.description = extractTagContent(xml, "content", itemStart, itemEnd);
    }
    item.link = extractLink(xml, itemStart, itemEnd);
    item.pubDate = extractTagContent(xml, isAtom ? "published" : "pubDate", itemStart, itemEnd);

    if (!item.title.empty()) {
      feed.items.push_back(std::move(item));
    }

    pos = itemEnd + (isAtom ? 8 : 7);
  }

  feed.valid = !feed.items.empty();
  return feed;
}
