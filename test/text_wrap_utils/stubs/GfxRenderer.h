#pragma once

#include <EpdFontFamily.h>

#include <string>

class GfxRenderer {
 public:
  int getScreenWidth() const { return 480; }
  int getScreenHeight() const { return 800; }
  int getLineHeight(int) const { return 16; }
  int getTextWidth(int, const char* text) const {
    if (!text) return 0;
    int len = 0;
    while (*text++) len += 8;
    return len;
  }
  void drawText(int, int, int, const char*, bool = false,
                EpdFontFamily::Style = EpdFontFamily::REGULAR) const {}
};
