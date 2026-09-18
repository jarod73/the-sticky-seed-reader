#pragma once

#include <Arduino.h>

#include <functional>
#include <string>
#include <vector>

class MappedInputManager;
class NimBLEServer;
class NimBLEService;
class NimBLECharacteristic;

/**
 * @brief Bluetooth Low Energy (BLE) GATT Companion Server for The Sticky Seed Reader.
 *
 * Exposes a standard Nordic UART-compatible custom GATT Service on the ESP32-S3:
 * - Service UUID: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
 * - RX Characteristic (Write): 6E400002-B5A3-F393-E0A9-E50E24DCCA9E
 * - TX Characteristic (Notify): 6E400003-B5A3-F393-E0A9-E50E24DCCA9E
 *
 * Features:
 * 1. Zero-friction mobile Wi-Fi Provisioning (CMD:WIFI_PROVISION:<SSID>:<PASS>)
 * 2. Real-time Device Telemetry & Reading Status (CMD:STATUS)
 * 3. Cloud Credentials Synchronization (CMD:CLOUD_CONFIG:<JSON>)
 * 4. Remote Page Navigation & Key Injection (CMD:KEY:<NAME>)
 * 5. Deterministic Advertising Identifier (StickySeed-XXXX)
 */
class BleCompanionServer {
 public:
  using EventCallback = std::function<void(const std::string& eventType, const std::string& detail)>;

  static BleCompanionServer& getInstance() {
    static BleCompanionServer instance;
    return instance;
  }

  void begin(MappedInputManager* inputManager = nullptr);
  void startServer();
  void stopServer();
  void loop();

  bool isRunning() const { return isRunning_; }
  bool isConnected() const { return isConnected_; }
  bool isAdvertising() const { return isAdvertising_; }

  const std::string& getDeviceName() const { return deviceName_; }
  const std::string& getConnectedClientAddress() const { return connectedClientAddress_; }

  void setEventCallback(EventCallback cb) { eventCallback_ = std::move(cb); }
  void sendNotification(const std::string& message);

  // Direct protocol command dispatcher (callable from BLE RX or internal tests)
  void processCommand(const std::string& command);

 private:
  BleCompanionServer() = default;
  ~BleCompanionServer() = default;

  void setupGattServices();
  void handleWifiProvision(const std::string& ssid, const std::string& pass);
  void sendStatusJson();
  void handleCloudConfig(const std::string& jsonPayload);
  void handleKeyInjection(const std::string& keyName);

  MappedInputManager* inputManager_ = nullptr;
  NimBLEServer* bleServer_ = nullptr;
  NimBLECharacteristic* txCharacteristic_ = nullptr;
  NimBLECharacteristic* rxCharacteristic_ = nullptr;

  bool isRunning_ = false;
  bool isConnected_ = false;
  bool isAdvertising_ = false;
  std::string deviceName_;
  std::string connectedClientAddress_;
  EventCallback eventCallback_;

  // Asynchronous Wi-Fi connection task state
  enum class WifiTaskState { IDLE, CONNECTING, CONNECTED, FAILED };
  WifiTaskState wifiState_ = WifiTaskState::IDLE;
  std::string pendingSsid_;
  std::string pendingPass_;
  unsigned long wifiConnectStartTime_ = 0;
};

#define BLE_COMPANION BleCompanionServer::getInstance()
