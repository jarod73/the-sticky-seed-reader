#include "ReadingStatsActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>

#include <algorithm>

#include "CrossPointSettings.h"
#include "components/UITheme.h"
#include "fontIds.h"

ReadingStatsActivity::ReadingStatsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("ReadingStats", renderer, mappedInput) {}

void ReadingStatsActivity::onEnter() {
  Activity::onEnter();
  requestUpdate();
}

void ReadingStatsActivity::onExit() { Activity::onExit(); }

void ReadingStatsActivity::loop() {
  Activity::loop();

  int tapX = 0, tapY = 0;
  if (mappedInput.wasScreenTapped(tapX, tapY) || mappedInput.wasBackGesture() || mappedInput.wasHomeGesture() ||
      mappedInput.wasReleased(MappedInputManager::Button::Back) ||
      mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    onGoHome();
  }
}

void ReadingStatsActivity::drawStatCard(int x, int y, int w, int h, const char* label, const char* value,
                                        const char* icon) {
  renderer.drawRoundedRect(x, y, w, h, 2, 8, true);
  renderer.fillRect(x + 2, y + 2, w - 4, h - 4, false);

  int textY = y + 16;
  if (icon) {
    renderer.drawText(UI_10_FONT_ID, x + 16, textY, icon, true, EpdFontFamily::BOLD);
    renderer.drawText(UI_10_FONT_ID, x + 36, textY, label, true);
  } else {
    renderer.drawText(UI_10_FONT_ID, x + 16, textY, label, true);
  }

  renderer.drawText(UI_12_FONT_ID, x + 16, y + h - 20, value, true, EpdFontFamily::BOLD);
}

void ReadingStatsActivity::draw7DayBarChart(int x, int y, int w, int h) {
  renderer.drawRoundedRect(x, y, w, h, 2, 8, true);
  renderer.fillRect(x + 2, y + 2, w - 4, h - 4, false);

  renderer.drawText(UI_10_FONT_ID, x + 16, y + 16, "Weekly Reading Activity (Minutes)", true, EpdFontFamily::BOLD);

  const auto history = READING_STATS.getLast7DaysMinutes();
  if (history.empty()) return;

  uint32_t maxMinutes = 1;
  for (const auto& [date, mins] : history) {
    if (mins > maxMinutes) maxMinutes = mins;
  }
  if (maxMinutes < 30) maxMinutes = 30;

  const int chartX = x + 24;
  const int chartY = y + 48;
  const int chartW = w - 48;
  const int chartH = h - 80;
  const int barSlotW = chartW / 7;
  const int barW = std::max(12, barSlotW - 12);

  renderer.drawLine(chartX, chartY + chartH, chartX + chartW, chartY + chartH, true);

  for (size_t i = 0; i < history.size() && i < 7; ++i) {
    const uint32_t mins = history[i].second;
    const int barH = static_cast<int>((mins * chartH) / maxMinutes);
    const int bx = chartX + i * barSlotW + (barSlotW - barW) / 2;
    const int by = chartY + chartH - barH;

    if (barH > 0) {
      renderer.fillRect(bx, by, barW, barH, true);
    }

    // Minutes label above bar
    char minBuf[16];
    snprintf(minBuf, sizeof(minBuf), "%um", mins);
    const int minW = renderer.getTextWidth(SMALL_FONT_ID, minBuf);
    renderer.drawText(SMALL_FONT_ID, bx + (barW - minW) / 2, by - 12, minBuf, true);

    // Day label below axis
    const std::string& dateStr = history[i].first;
    std::string dayLabel = (dateStr.length() >= 5) ? dateStr.substr(dateStr.length() - 5) : dateStr;
    const int dayW = renderer.getTextWidth(SMALL_FONT_ID, dayLabel.c_str());
    renderer.drawText(SMALL_FONT_ID, bx + (barW - dayW) / 2, chartY + chartH + 14, dayLabel.c_str(), true);
  }
}

void ReadingStatsActivity::render(RenderLock&&) {
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();

  renderer.clearScreen(0xFF);

  // 1. Header Bar
  GUI.drawHeader(renderer, Rect{0, 0, screenW, 40}, "Reading Insights & Statistics");

  // 2. Daily Streak Banner
  char streakBuf[64];
  const uint16_t streak = READING_STATS.getDailyStreak();
  if (streak > 0) {
    snprintf(streakBuf, sizeof(streakBuf), "🔥 %u Day Reading Streak!", streak);
  } else {
    snprintf(streakBuf, sizeof(streakBuf), "📖 Start your daily streak today!");
  }
  renderer.drawCenteredText(UI_12_FONT_ID, 64, streakBuf, true, EpdFontFamily::BOLD);

  // 3. Four Statistics Metric Cards (2x2 Grid)
  const int cardW = (screenW - 56) / 2;
  const int cardH = 70;
  const int gridY = 96;

  char timeBuf[32];
  const uint32_t totalMins = READING_STATS.getTotalReadingMinutes();
  snprintf(timeBuf, sizeof(timeBuf), "%uh %um", totalMins / 60, totalMins % 60);
  drawStatCard(16, gridY, cardW, cardH, "Total Time", timeBuf);

  char pagesBuf[32];
  snprintf(pagesBuf, sizeof(pagesBuf), "%u pages", READING_STATS.getTotalPagesRead());
  drawStatCard(screenW - 16 - cardW, gridY, cardW, cardH, "Pages Read", pagesBuf);

  char booksBuf[32];
  snprintf(booksBuf, sizeof(booksBuf), "%u books", READING_STATS.getTotalBooksFinished());
  drawStatCard(16, gridY + cardH + 12, cardW, cardH, "Finished", booksBuf);

  char wpmBuf[32];
  snprintf(wpmBuf, sizeof(wpmBuf), "%u WPM", READING_STATS.getAverageWpm());
  drawStatCard(screenW - 16 - cardW, gridY + cardH + 12, cardW, cardH, "Speed", wpmBuf);

  // 4. 7-Day Interactive Activity Bar Chart
  const int chartY = gridY + cardH * 2 + 28;
  const int chartH = screenH - chartY - 40;
  draw7DayBarChart(16, chartY, screenW - 32, chartH);

  // 5. Button hints / footer
  const auto labels = mappedInput.mapLabels("Back", "", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
