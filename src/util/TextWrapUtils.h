#pragma once

#include <EpdFontFamily.h>

#include <string>
#include <string_view>
#include <vector>

class GfxRenderer;

/**
 * TextWrapUtils
 *
 * A reusable, modular text formatting and pagination utility designed for
 * embedded E-Ink displays. Handles:
 *
 * 1. Word-wrapping long strings into line lists bounded by pixel width or character count.
 * 2. Multi-line paragraph rendering with custom line heights and font styling.
 * 3. Paginated text distribution across multiple screen views.
 *
 * Designed to be zero-cost, memory-conscious, and friendly for both Junior
 * and Senior developers to maintain.
 */
class TextWrapUtils {
 public:
  /**
   * Word-wraps a text string into multiple lines bounded by pixel width.
   *
   * @param renderer Reference to the active GfxRenderer (for font metrics).
   * @param fontId The target font ID.
   * @param text The input text string to wrap.
   * @param maxWidthPixels Maximum allowable width in screen pixels.
   * @param maxLines Optional cap on the number of returned lines (0 = unlimited).
   * @return A vector of wrapped lines.
   */
  static std::vector<std::string> wrapToWidth(const GfxRenderer& renderer, int fontId, std::string_view text,
                                              int maxWidthPixels, size_t maxLines = 0);

  /**
   * Fast word-wrapper based on estimated character capacity per line.
   *
   * @param text The input text to wrap.
   * @param maxCharsPerLine Approximate number of characters per line.
   * @param maxLines Optional cap on total lines (0 = unlimited).
   * @return A vector of wrapped lines.
   */
  static std::vector<std::string> wrapToCharCount(std::string_view text, size_t maxCharsPerLine, size_t maxLines = 0);

  /**
   * Renders a multi-line paragraph directly onto the display within a bounding box.
   *
   * @param renderer Reference to the active GfxRenderer.
   * @param fontId Font ID to use for drawing.
   * @param x Left coordinate (pixels).
   * @param y Top coordinate (pixels).
   * @param maxWidth Maximum width in pixels.
   * @param maxHeight Maximum allowable vertical height (pixels).
   * @param text The text content to format and draw.
   * @param style Font style (Regular, Bold, Italic).
   * @param extraLineSpacing Additional padding added between lines in pixels (default: 4).
   * @return The final Y coordinate reached after drawing all lines.
   */
  static int drawWrappedParagraph(GfxRenderer& renderer, int fontId, int x, int y, int maxWidth, int maxHeight,
                                  std::string_view text, EpdFontFamily::Style style = EpdFontFamily::REGULAR,
                                  int extraLineSpacing = 4);

  /**
   * Paginates structured paragraphs into discrete pages fitting the screen area.
   *
   * @param renderer Reference to the active GfxRenderer.
   * @param fontId Font ID for calculating line height and character width.
   * @param paragraphs List of raw paragraph strings.
   * @param contentWidth Usable width of the content area (pixels).
   * @param contentHeight Usable height of the content area (pixels).
   * @param outPages Output vector populated with formatted page text (lines joined by '\n').
   */
  static void paginateParagraphs(const GfxRenderer& renderer, int fontId, const std::vector<std::string>& paragraphs,
                                 int contentWidth, int contentHeight, std::vector<std::string>& outPages);
};
