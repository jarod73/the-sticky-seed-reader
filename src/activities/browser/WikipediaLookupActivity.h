#pragma once

#include <string>
#include "activities/Activity.h"

class WikipediaLookupActivity final : public Activity {
 private:
  std::string searchTerm;
  std::string articleTitle;
  std::string description;
  std::string extract;
  std::string errorMessage;
  bool loading = true;
  bool failed = false;

  void fetchSummary(const std::string& term);

 public:
  explicit WikipediaLookupActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                                   std::string term = "");
  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;
};
