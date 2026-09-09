#include "TextWrapUtils.h"

#include <GfxRenderer.h>

#include <algorithm>
#include <cctype>

std::vector<std::string> TextWrapUtils::wrapToWidth(const GfxRenderer& renderer, int fontId, std::string_view text,
                                                    int maxWidthPixels, size_t maxLines) {
  std::vector<std::string> lines;
  if (text.empty() || maxWidthPixels <= 0) return lines;

  // Approximate reserve size
  lines.reserve(std::max(size_t(4), text.size() / 30));

  const int approxCharWidth = std::max(8, renderer.getTextWidth(fontId, "M") * 2 / 3);
  const size_t maxCharsEstimate = std::max(size_t(10), static_cast<size_t>(maxWidthPixels / approxCharWidth));

  size_t start = 0;
  while (start < text.length()) {
    if (maxLines > 0 && lines.size() >= maxLines) break;

    size_t len = std::min(maxCharsEstimate, text.length() - start);

    // Word boundary hunt: look backwards for a space delimiter
    if (start + len < text.length()) {
      size_t space = text.rfind(' ', start + len);
      if (space != std::string_view::npos && space > start) {
        len = space - start;
      }
    }

    std::string line(text.substr(start, len));
    // Verify pixel width and shrink if necessary for wide words
    while (line.length() > 5 && renderer.getTextWidth(fontId, line.c_str()) > maxWidthPixels) {
      line.pop_back();
      len--;
    }

    lines.push_back(std::move(line));
    start += len;

    // Skip trailing spaces
    while (start < text.length() && std::isspace(static_cast<unsigned char>(text[start]))) {
      start++;
    }
  }

  return lines;
}

std::vector<std::string> TextWrapUtils::wrapToCharCount(std::string_view text, size_t maxCharsPerLine,
                                                        size_t maxLines) {
  std::vector<std::string> lines;
  if (text.empty() || maxCharsPerLine == 0) return lines;

  lines.reserve(std::max(size_t(4), text.size() / maxCharsPerLine));

  size_t start = 0;
  while (start < text.length()) {
    if (maxLines > 0 && lines.size() >= maxLines) break;

    size_t len = std::min(maxCharsPerLine, text.length() - start);
    if (start + len < text.length()) {
      size_t space = text.rfind(' ', start + len);
      if (space != std::string_view::npos && space > start) {
        len = space - start;
      }
    }

    lines.emplace_back(text.substr(start, len));
    start += len;

    while (start < text.length() && std::isspace(static_cast<unsigned char>(text[start]))) {
      start++;
    }
  }

  return lines;
}

int TextWrapUtils::drawWrappedParagraph(GfxRenderer& renderer, int fontId, int x, int y, int maxWidth, int maxHeight,
                                        std::string_view text, EpdFontFamily::Style style, int extraLineSpacing) {
  if (text.empty()) return y;

  const int lineHeight = renderer.getLineHeight(fontId) + extraLineSpacing;
  const size_t maxLines = (maxHeight > 0) ? std::max(size_t(1), static_cast<size_t>(maxHeight / lineHeight)) : 0;

  std::vector<std::string> lines = wrapToWidth(renderer, fontId, text, maxWidth, maxLines);

  int currentY = y;
  for (const auto& line : lines) {
    if (maxHeight > 0 && (currentY - y + lineHeight) > maxHeight) break;
    renderer.drawText(fontId, x, currentY, line.c_str(), true, style);
    currentY += lineHeight;
  }

  return currentY;
}

void TextWrapUtils::paginateParagraphs(const GfxRenderer& renderer, int fontId,
                                       const std::vector<std::string>& paragraphs, int contentWidth, int contentHeight,
                                       std::vector<std::string>& outPages) {
  outPages.clear();
  if (paragraphs.empty() || contentWidth <= 0 || contentHeight <= 0) return;

  const int lineHeight = renderer.getLineHeight(fontId) + 6;
  const int linesPerPage = std::max(5, contentHeight / lineHeight);
  const int approxCharWidth = std::max(8, renderer.getTextWidth(fontId, "M") * 2 / 3);
  const size_t approxCharsPerLine = std::max(size_t(20), static_cast<size_t>(contentWidth / approxCharWidth));

  std::string currentPageText;
  int currentLineCount = 0;

  for (const auto& para : paragraphs) {
    std::vector<std::string> wrapped = wrapToCharCount(para, approxCharsPerLine);
    for (const auto& line : wrapped) {
      if (!currentPageText.empty()) currentPageText += "\n";
      currentPageText += line;
      currentLineCount++;

      if (currentLineCount >= linesPerPage) {
        outPages.push_back(std::move(currentPageText));
        currentPageText.clear();
        currentLineCount = 0;
      }
    }

    // Paragraph break spacing
    if (currentLineCount > 0 && currentLineCount + 1 < linesPerPage) {
      currentPageText += "\n";
      currentLineCount++;
    }
  }

  if (!currentPageText.empty()) {
    outPages.push_back(std::move(currentPageText));
  }
}
