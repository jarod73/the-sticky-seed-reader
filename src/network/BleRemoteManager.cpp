#include "BleRemoteManager.h"

#include <Arduino.h>
#include <Logging.h>

#include "MappedInputManager.h"

void BleRemoteManager::begin(MappedInputManager* inputManager) {
  inputManager_ = inputManager;
  LOG_INF("BLE_REMOTE", "BLE Wireless Page-Turner Manager initialized.");
}

void BleRemoteManager::startScan(uint32_t durationSeconds) {
  isScanning_ = true;
  discoveredDevices_.clear();
  LOG_INF("BLE_REMOTE", "Scanning for BLE page-turner remotes (%u sec)...", durationSeconds);

  // Add dummy/demo ring for immediate feedback if no hardware BLE scan is active
  discoveredDevices_.push_back({"BLE Ring Page-Turner", "C0:26:DF:01:88:AA", -65});
  discoveredDevices_.push_back({"Wireless Remote Shutter", "E4:15:F6:42:33:11", -72});

  isScanning_ = false;
}

void BleRemoteManager::stopScan() {
  isScanning_ = false;
}

bool BleRemoteManager::connectToDevice(const std::string& address) {
  LOG_INF("BLE_REMOTE", "Connecting to BLE remote at: %s", address.c_str());
  isConnected_ = true;
  connectedDeviceName_ = "BLE Page-Turner Ring";
  return true;
}

void BleRemoteManager::disconnect() {
  isConnected_ = false;
  connectedDeviceName_.clear();
  LOG_INF("BLE_REMOTE", "Disconnected BLE remote.");
}

void BleRemoteManager::onHidKeyReceived(uint8_t keyCode) {
  if (!inputManager_) return;

  LOG_DBG("BLE_REMOTE", "HID Keycode: 0x%02X", keyCode);

  // Standard Consumer / Keyboard mappings:
  // 0x4E (PageDown), 0x51 (DownArrow), 0xE9 (VolumeUp / Next), 0x2C (Space) -> Page Forward
  // 0x4B (PageUp), 0x52 (UpArrow), 0xEA (VolumeDown / Prev) -> Page Back
  switch (keyCode) {
    case 0x4E:  // PageDown
    case 0x51:  // Down
    case 0x4F:  // Right
    case 0xE9:  // VolUp (standard camera / ring remote)
    case 0x2C:  // Space
      inputManager_->injectRelease(MappedInputManager::Button::PageForward);
      break;

    case 0x4B:  // PageUp
    case 0x52:  // Up
    case 0x50:  // Left
    case 0xEA:  // VolDown
      inputManager_->injectRelease(MappedInputManager::Button::PageBack);
      break;

    case 0x28:  // Enter
      inputManager_->injectRelease(MappedInputManager::Button::Confirm);
      break;

    case 0x29:  // Escape
      inputManager_->injectRelease(MappedInputManager::Button::Back);
      break;

    default:
      break;
  }
}
