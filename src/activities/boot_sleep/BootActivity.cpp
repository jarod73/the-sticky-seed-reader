#include "BootActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include "fontIds.h"
#include "images/StickySeedLogo120.h"

void BootActivity::onEnter() {
  Activity::onEnter();

  const auto pageWidth = renderer.getScreenWidth();
  const auto pageHeight = renderer.getScreenHeight();

  renderer.clearScreen();
  renderer.drawImage(StickySeedLogo120, (pageWidth - 120) / 2, (pageHeight - 120) / 2 - 20, 120, 120);
  renderer.drawCenteredText(UI_12_FONT_ID, pageHeight / 2 + 55, "The Sticky Seed Reader", true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight / 2 + 82, tr(STR_BOOTING));
  renderer.drawCenteredText(SMALL_FONT_ID, pageHeight - 25, "reTerminal Sticky Edition v" CROSSPOINT_VERSION);
  renderer.displayBuffer();
}
