#pragma once

#include <string>
#include <vector>

#include "activities/Activity.h"
#include "network/BleCompanionServer.h"

/**
 * @brief Interactive BLE Phone Companion Pairing & Wi-Fi Provisioning Screen on E-Ink.
 */
class BleCompanionActivity final : public Activity {
 public:
  explicit BleCompanionActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~BleCompanionActivity() override = default;

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void handleEvent(const std::string& eventType, const std::string& detail);

  std::string statusText_ = "Starting BLE Companion Server...";
  std::vector<std::string> logLines_;
  bool needsRerender_ = false;
};
