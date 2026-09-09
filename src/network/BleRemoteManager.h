#pragma once

#include <string>
#include <vector>

class MappedInputManager;

/**
 * @brief Bluetooth Low Energy (BLE) Wireless Page-Turner Remote Manager.
 *
 * Scans, pairs, and listens to BLE HID page-turner rings, presentation remotes,
 * and camera clickers, translating wireless click events into instant page turns.
 */
class BleRemoteManager {
 public:
  struct DiscoveredDevice {
    std::string name;
    std::string address;
    int rssi = 0;
  };

  static BleRemoteManager& getInstance() {
    static BleRemoteManager instance;
    return instance;
  }

  void begin(MappedInputManager* inputManager);
  void startScan(uint32_t durationSeconds = 5);
  void stopScan();
  bool isScanning() const { return isScanning_; }
  bool isConnected() const { return isConnected_; }

  const std::vector<DiscoveredDevice>& getDiscoveredDevices() const { return discoveredDevices_; }
  bool connectToDevice(const std::string& address);
  void disconnect();

  void onHidKeyReceived(uint8_t keyCode);

 private:
  BleRemoteManager() = default;
  ~BleRemoteManager() = default;

  MappedInputManager* inputManager_ = nullptr;
  bool isScanning_ = false;
  bool isConnected_ = false;
  std::string connectedDeviceName_;
  std::vector<DiscoveredDevice> discoveredDevices_;
};

#define BLE_REMOTE BleRemoteManager::getInstance()
