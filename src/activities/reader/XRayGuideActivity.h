#pragma once

#include <string>
#include <vector>

#include "activities/Activity.h"

/**
 * @brief Kindle-style "X-Ray" / Character & Concept Guide on E-Ink.
 *
 * Provides instant 1-screen character dossiers, historical context explainers,
 * and key concept descriptions while reading.
 */
class XRayGuideActivity final : public Activity {
 public:
  explicit XRayGuideActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string term = "");
  ~XRayGuideActivity() override = default;

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void fetchConceptDetails(const std::string& term);

  std::string targetTerm_;
  std::string conceptTitle_;
  std::string conceptSubtitle_;
  std::string conceptSummary_;
  std::string errorMessage_;
  bool isLoading_ = true;
  bool isFailed_ = false;
};
