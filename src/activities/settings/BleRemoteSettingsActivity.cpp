#include "BleRemoteSettingsActivity.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

BleRemoteSettingsActivity::BleRemoteSettingsActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("BleRemoteSettings", renderer, mappedInput) {}

void BleRemoteSettingsActivity::onEnter() {
  Activity::onEnter();
  statusMessage_ = BLE_REMOTE.isConnected() ? "Connected to BLE Remote" : "Select a device to pair";
  scanForDevices();
}

void BleRemoteSettingsActivity::onExit() { Activity::onExit(); }

void BleRemoteSettingsActivity::scanForDevices() {
  isScanning_ = true;
  statusMessage_ = "Scanning for wireless page-turner rings...";
  requestUpdate();

  BLE_REMOTE.startScan(3);

  isScanning_ = false;
  if (BLE_REMOTE.getDiscoveredDevices().empty()) {
    statusMessage_ = "No Bluetooth remotes found. Ensure remote is in pairing mode.";
  } else {
    statusMessage_ = "Discovered " + std::to_string(BLE_REMOTE.getDiscoveredDevices().size()) + " devices.";
  }
  requestUpdate();
}

void BleRemoteSettingsActivity::selectDevice(size_t index) {
  const auto& devices = BLE_REMOTE.getDiscoveredDevices();
  if (index >= devices.size()) return;

  if (BLE_REMOTE.connectToDevice(devices[index].address)) {
    statusMessage_ = "Paired with " + devices[index].name + "!";
  } else {
    statusMessage_ = "Failed to connect to device.";
  }
  requestUpdate();
}

void BleRemoteSettingsActivity::loop() {
  if (isScanning_) return;

  if (mappedInput.wasBackGesture() || mappedInput.wasHomeGesture() ||
      mappedInput.wasReleased(MappedInputManager::Button::Back)) {
    onGoHome();
    return;
  }

  int tx = 0, ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    if (ty < 50) {
      onGoHome();
      return;
    }
    if (ty > renderer.getScreenHeight() - 60) {
      scanForDevices();
      return;
    }

    const auto& devices = BLE_REMOTE.getDiscoveredDevices();
    const int startY = 60;
    const int itemH = 64;
    for (size_t i = 0; i < devices.size() && i < 5; ++i) {
      const int rowY = startY + i * itemH;
      if (ty >= rowY && ty < rowY + itemH) {
        selectedIndex_ = static_cast<int>(i);
        selectDevice(selectedIndex_);
        return;
      }
    }
  }

  const auto& devices = BLE_REMOTE.getDiscoveredDevices();
  if (mappedInput.wasReleased(MappedInputManager::Button::Up)) {
    if (selectedIndex_ > 0) {
      selectedIndex_--;
      requestUpdate();
    }
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Down)) {
    if (selectedIndex_ + 1 < static_cast<int>(devices.size())) {
      selectedIndex_++;
      requestUpdate();
    }
  } else if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    if (!devices.empty()) {
      selectDevice(selectedIndex_);
    } else {
      scanForDevices();
    }
  }
}

void BleRemoteSettingsActivity::render(RenderLock&&) {
  const int screenW = renderer.getScreenWidth();

  renderer.clearScreen(0xFF);
  GUI.drawHeader(renderer, Rect{0, 0, screenW, 40}, "Bluetooth Page-Turner Remotes");

  // 1. Connection Status Banner
  renderer.drawRoundedRect(16, 54, screenW - 32, 54, 2, 8, true);
  if (BLE_REMOTE.isConnected()) {
    renderer.drawText(UI_12_FONT_ID, 32, 70, "Status: Connected (Ready)", true, EpdFontFamily::BOLD);
    renderer.drawText(SMALL_FONT_ID, 32, 92, "Clicks on ring/remote turn pages wirelessly.", true);
  } else {
    renderer.drawText(UI_12_FONT_ID, 32, 70, "Status: Disconnected", true, EpdFontFamily::BOLD);
    renderer.drawText(SMALL_FONT_ID, 32, 92, statusMessage_.c_str(), true);
  }

  // 2. Discovered Devices List
  const auto& devices = BLE_REMOTE.getDiscoveredDevices();
  const int startY = 120;
  const int itemH = 64;

  for (size_t i = 0; i < devices.size() && i < 4; ++i) {
    const auto& dev = devices[i];
    const int rowY = startY + i * itemH;
    const bool isSelected = (static_cast<int>(i) == selectedIndex_);

    if (isSelected) {
      renderer.drawRoundedRect(16, rowY, screenW - 32, itemH - 8, 2, 8, true);
    }

    renderer.drawText(UI_12_FONT_ID, 28, rowY + 16, dev.name.c_str(), true,
                      isSelected ? EpdFontFamily::BOLD : EpdFontFamily::REGULAR);

    std::string sub = dev.address + " (" + std::to_string(dev.rssi) + " dBm)";
    renderer.drawText(SMALL_FONT_ID, 28, rowY + 38, sub.c_str(), true);
  }

  const auto labels = mappedInput.mapLabels("Back", "Pair", "Scan", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
