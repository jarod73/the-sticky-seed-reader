#pragma once
#include "CrossPointSettings.h"
#include "fontIds.h"

// FreeInkUI font slots. Row heights, header height, and touch sizes are
// derived by FreeInkApp from the body font's line height (themeTokensForLineHeight).
// Maps to 4 adjustable user sizes (Small, Medium, Large, Extra Large).
struct UIScaleSpec {
  int smallFontId;
  int bodyFontId;
  int titleFontId;
};

inline UIScaleSpec uiScaleSpec() {
  UIScaleSpec spec{};
  switch (SETTINGS.uiFontSize) {
    case CrossPointSettings::UI_FONT_SIZE_SMALL:
      spec.smallFontId = SMALL_FONT_ID;
      spec.bodyFontId = UI_10_FONT_ID;
      spec.titleFontId = UI_12_FONT_ID;
      break;
    case CrossPointSettings::UI_FONT_SIZE_MEDIUM:
      spec.smallFontId = UI_10_FONT_ID;
      spec.bodyFontId = UI_12_FONT_ID;
      spec.titleFontId = NOTOSANS_14_FONT_ID;
      break;
    case CrossPointSettings::UI_FONT_SIZE_EXTRA_LARGE:
      spec.smallFontId = NOTOSANS_14_FONT_ID;
      spec.bodyFontId = NOTOSANS_16_FONT_ID;
      spec.titleFontId = NOTOSANS_18_FONT_ID;
      break;
    case CrossPointSettings::UI_FONT_SIZE_LARGE:
    default:
      spec.smallFontId = UI_12_FONT_ID;
      spec.bodyFontId = NOTOSANS_14_FONT_ID;
      spec.titleFontId = NOTOSANS_16_FONT_ID;
      break;
  }
  return spec;
}
