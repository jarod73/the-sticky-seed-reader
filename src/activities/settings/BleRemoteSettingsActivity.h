#pragma once

#include "activities/Activity.h"
#include "network/BleRemoteManager.h"

/**
 * @brief Bluetooth Page-Turner Remote / Ring Pairing and Diagnostics Activity on E-Ink.
 */
class BleRemoteSettingsActivity final : public Activity {
 public:
  explicit BleRemoteSettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  ~BleRemoteSettingsActivity() override = default;

  void onEnter() override;
  void onExit() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void scanForDevices();
  void selectDevice(size_t index);

  int selectedIndex_ = 0;
  std::string statusMessage_;
  bool isScanning_ = false;
};
