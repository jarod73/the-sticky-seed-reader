#pragma once

#include "ReadingStatsStore.h"
#include "activities/Activity.h"

/**
 * @brief Kobo-style Reading Statistics & Heatmap Dashboard on E-Ink.
 *
 * Displays:
 * 1. Daily reading streak flame badge.
 * 2. 7-Day interactive visual bar chart / reading heatmap.
 * 3. Total reading time, pages completed, and books finished.
 * 4. Average reading speed in Words Per Minute (WPM).
 */
class ReadingStatsActivity final : public Activity {
 public:
  explicit ReadingStatsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~ReadingStatsActivity() override = default;

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void drawStatCard(int x, int y, int w, int h, const char* label, const char* value, const char* icon = nullptr);
  void draw7DayBarChart(int x, int y, int w, int h);
};
