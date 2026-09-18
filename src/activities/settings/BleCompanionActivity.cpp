#include "BleCompanionActivity.h"

#include <GfxRenderer.h>
#include <HalStorage.h>
#include <I18n.h>
#include <Logging.h>

#include "CrossPointSettings.h"
#include "MappedInputManager.h"
#include "components/UITheme.h"
#include "fontIds.h"

BleCompanionActivity::BleCompanionActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("BleCompanionActivity", renderer, mappedInput) {}

void BleCompanionActivity::onEnter() {
  Activity::onEnter();
  statusText_ = "Advertising... Ready for Companion Connection";
  logLines_.clear();
  logLines_.push_back("Server started. Listening for incoming connections...");

  BLE_COMPANION.begin(&mappedInput);
  BLE_COMPANION.setEventCallback(
      [this](const std::string& eventType, const std::string& detail) { handleEvent(eventType, detail); });
  BLE_COMPANION.startServer();

  requestUpdate();
}

void BleCompanionActivity::onExit() {
  BLE_COMPANION.setEventCallback(nullptr);
  BLE_COMPANION.stopServer();
  Activity::onExit();
}

void BleCompanionActivity::handleEvent(const std::string& eventType, const std::string& detail) {
  if (eventType == "STATE_CHANGED") {
    statusText_ = (detail == "ADVERTISING") ? "Advertising... Ready to pair" : "BLE Server stopped";
  } else if (eventType == "WIFI_CONNECTING") {
    statusText_ = "Wi-Fi: Connecting to '" + detail + "'...";
    logLines_.push_back("Received Wi-Fi credentials for: " + detail);
  } else if (eventType == "WIFI_CONNECTED") {
    statusText_ = "Wi-Fi Connected! IP: " + detail;
    logLines_.push_back("Connected to network. Assigned IP: " + detail);
  } else if (eventType == "WIFI_FAILED") {
    statusText_ = "Wi-Fi Connection Failed: " + detail;
    logLines_.push_back("Wi-Fi connection attempt failed (" + detail + ")");
  } else if (eventType == "CLOUD_CONFIG_SAVED") {
    statusText_ = "Cloud Config Synced Successfully!";
    logLines_.push_back("Cloud API tokens updated and encrypted to NVS/SD.");
  }

  // Keep log size bounded
  while (logLines_.size() > 6) {
    logLines_.erase(logLines_.begin());
  }

  needsRerender_ = true;
}

void BleCompanionActivity::loop() {
  BLE_COMPANION.loop();

  if (needsRerender_) {
    needsRerender_ = false;
    requestUpdate();
  }

  if (mappedInput.wasBackGesture() || mappedInput.wasHomeGesture() ||
      mappedInput.wasReleased(MappedInputManager::Button::Back) ||
      mappedInput.wasReleased(MappedInputManager::Button::Left)) {
    onGoHome();
    return;
  }

  int tx = 0, ty = 0;
  if (mappedInput.wasScreenTapped(tx, ty)) {
    if (ty < 50 || ty > renderer.getScreenHeight() - 60) {
      onGoHome();
      return;
    }
  }

  if (mappedInput.wasReleased(MappedInputManager::Button::Confirm)) {
    // Restart advertising if needed
    if (!BLE_COMPANION.isAdvertising()) {
      BLE_COMPANION.startServer();
      requestUpdate();
    }
  }
}

void BleCompanionActivity::render(RenderLock&&) {
  const int screenW = renderer.getScreenWidth();
  const int screenH = renderer.getScreenHeight();

  renderer.clearScreen(0xFF);
  GUI.drawHeader(renderer, Rect{0, 0, screenW, 40}, "Phone Companion & BLE Sync");

  // 1. Device Identifier & Status Card
  renderer.drawRoundedRect(16, 50, screenW - 32, 85, 2, 8, true);
  std::string devName = "Device: " + BLE_COMPANION.getDeviceName();
  renderer.drawText(UI_12_FONT_ID, 32, 68, devName.c_str(), true, EpdFontFamily::BOLD);
  renderer.drawText(SMALL_FONT_ID, 32, 92, statusText_.c_str(), true);
  renderer.drawText(SMALL_FONT_ID, 32, 114, "GATT Service: Nordic UART (6E400001-...)", true);

  // 2. Instructions Card
  renderer.drawRoundedRect(16, 145, screenW - 32, 100, 1, 6, true);
  renderer.drawText(UI_10_FONT_ID, 28, 160, "Companion App Instructions:", true, EpdFontFamily::BOLD);
  renderer.drawText(SMALL_FONT_ID, 28, 180, "1. Open TheStickyReader mobile app or BLE Scanner.", true);
  renderer.drawText(SMALL_FONT_ID, 28, 200, "2. Select this device to pair and send Wi-Fi credentials.", true);
  renderer.drawText(SMALL_FONT_ID, 28, 220, "3. Sync reading statistics, cloud tokens, or control pages.", true);

  // 3. Live Sync Activity Log Box
  const int logY = 255;
  const int logH = screenH - logY - 60;
  if (logH > 80) {
    renderer.drawRoundedRect(16, logY, screenW - 32, logH, 1, 6, true);
    renderer.drawText(UI_10_FONT_ID, 28, logY + 16, "Live Event Log:", true, EpdFontFamily::BOLD);

    int lineY = logY + 38;
    for (const auto& line : logLines_) {
      if (lineY + 18 < logY + logH) {
        renderer.drawText(SMALL_FONT_ID, 28, lineY, line.c_str(), true);
        lineY += 20;
      }
    }
  }

  const auto labels = mappedInput.mapLabels("Done", "Restart", "", "");
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);

  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}
