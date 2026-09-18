#include "BleCompanionServer.h"

#include <ArduinoJson.h>
#include <BatteryMonitor.h>
#include <EnvironmentSensor.h>
#include <Logging.h>
#include <NimBLEDevice.h>
#include <WiFi.h>

#include "MappedInputManager.h"
#include "RecentBooksStore.h"
#include "WifiCredentialStore.h"
#include "network/CloudCredentialStore.h"

namespace {
// Nordic UART Service UUIDs
constexpr const char* SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr const char* RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
constexpr const char* TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

static BatteryMonitor g_batteryMonitor;
static EnvironmentSensor g_envSensor;
static bool g_sensorsInit = false;

std::string generateDefaultDeviceName() {
  uint64_t mac = ESP.getEfuseMac();
  uint16_t suffix = static_cast<uint16_t>((mac >> 32) ^ (mac & 0xFFFF));
  char nameBuf[32];
  snprintf(nameBuf, sizeof(nameBuf), "StickySeed-%04X", suffix);
  return std::string(nameBuf);
}
}  // namespace

// Server Callback
class BleCompanionServerCallbacks : public NimBLEServerCallbacks {
 public:
  explicit BleCompanionServerCallbacks(BleCompanionServer& parent) : parent_(parent) {}

  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    LOG_INF("BLE_COMPANION", "Client connected: %s", connInfo.getAddress().toString().c_str());
    parent_.sendNotification("OK:CONNECTED");
  }

  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    LOG_INF("BLE_COMPANION", "Client disconnected (reason: %d)", reason);
    if (parent_.isRunning()) {
      NimBLEDevice::startAdvertising();
    }
  }

 private:
  BleCompanionServer& parent_;
};

// RX Characteristic Callback
class BleCompanionRxCallbacks : public NimBLECharacteristicCallbacks {
 public:
  explicit BleCompanionRxCallbacks(BleCompanionServer& parent) : parent_(parent) {}

  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    std::string val = pCharacteristic->getValue();
    if (!val.empty()) {
      LOG_DBG("BLE_COMPANION", "Received command (%zu bytes): %s", val.size(), val.c_str());
      parent_.processCommand(val);
    }
  }

 private:
  BleCompanionServer& parent_;
};

void BleCompanionServer::begin(MappedInputManager* inputManager) {
  inputManager_ = inputManager;
  if (deviceName_.empty()) {
    deviceName_ = generateDefaultDeviceName();
  }

  if (!g_sensorsInit) {
    g_envSensor.begin();
    g_sensorsInit = true;
  }
}

void BleCompanionServer::startServer() {
  if (isRunning_) return;

  LOG_INF("BLE_COMPANION", "Starting BLE Companion GATT Server as '%s'...", deviceName_.c_str());

  NimBLEDevice::init(deviceName_);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // +9dBm max TX power

  setupGattServices();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setName(deviceName_);
  pAdvertising->enableScanResponse(true);
  pAdvertising->start();

  isRunning_ = true;
  isAdvertising_ = true;

  if (eventCallback_) {
    eventCallback_("STATE_CHANGED", "ADVERTISING");
  }
  LOG_INF("BLE_COMPANION", "Advertising started.");
}

void BleCompanionServer::setupGattServices() {
  bleServer_ = NimBLEDevice::createServer();
  bleServer_->setCallbacks(new BleCompanionServerCallbacks(*this));

  NimBLEService* pService = bleServer_->createService(SERVICE_UUID);

  // TX Characteristic (Notify from device to companion app)
  txCharacteristic_ =
      pService->createCharacteristic(TX_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);

  // RX Characteristic (Write from companion app to device)
  rxCharacteristic_ = pService->createCharacteristic(
      RX_CHAR_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rxCharacteristic_->setCallbacks(new BleCompanionRxCallbacks(*this));
}

void BleCompanionServer::stopServer() {
  if (!isRunning_) return;

  LOG_INF("BLE_COMPANION", "Stopping BLE Companion GATT Server...");
  if (NimBLEDevice::getAdvertising()->isAdvertising()) {
    NimBLEDevice::stopAdvertising();
  }

  if (bleServer_) {
    NimBLEDevice::deinit(true);
    bleServer_ = nullptr;
    txCharacteristic_ = nullptr;
    rxCharacteristic_ = nullptr;
  }

  isRunning_ = false;
  isConnected_ = false;
  isAdvertising_ = false;

  if (eventCallback_) {
    eventCallback_("STATE_CHANGED", "STOPPED");
  }
}

void BleCompanionServer::sendNotification(const std::string& message) {
  if (txCharacteristic_ && isRunning_) {
    txCharacteristic_->setValue(message);
    txCharacteristic_->notify();
    LOG_DBG("BLE_COMPANION", "TX: %s", message.c_str());
  }
}

void BleCompanionServer::processCommand(const std::string& rawCommand) {
  std::string cmd = rawCommand;
  // Trim trailing whitespace or CR/LF
  while (!cmd.empty() && (cmd.back() == '\r' || cmd.back() == '\n' || cmd.back() == ' ')) {
    cmd.pop_back();
  }

  if (cmd.empty()) return;

  if (cmd == "PING" || cmd == "CMD:PING") {
    sendNotification("PONG");
    return;
  }

  if (cmd == "STATUS" || cmd == "CMD:STATUS" || cmd == "CMD:GET_STATUS") {
    sendStatusJson();
    return;
  }

  if (cmd.rfind("CMD:WIFI_PROVISION:", 0) == 0 || cmd.rfind("WIFI:", 0) == 0) {
    size_t prefixLen = (cmd.rfind("CMD:WIFI_PROVISION:", 0) == 0) ? 19 : 5;
    std::string payload = cmd.substr(prefixLen);
    size_t colon = payload.find(':');
    if (colon != std::string::npos) {
      std::string ssid = payload.substr(0, colon);
      std::string pass = payload.substr(colon + 1);
      handleWifiProvision(ssid, pass);
    } else {
      handleWifiProvision(payload, "");
    }
    return;
  }

  if (cmd.rfind("CMD:CLOUD_CONFIG:", 0) == 0) {
    handleCloudConfig(cmd.substr(17));
    return;
  }

  if (cmd.rfind("CMD:KEY:", 0) == 0 || cmd.rfind("KEY:", 0) == 0) {
    size_t prefixLen = (cmd.rfind("CMD:KEY:", 0) == 0) ? 8 : 4;
    handleKeyInjection(cmd.substr(prefixLen));
    return;
  }

  LOG_ERR("BLE_COMPANION", "Unrecognized command: %s", cmd.c_str());
  sendNotification("ERR:UNKNOWN_COMMAND");
}

void BleCompanionServer::handleWifiProvision(const std::string& ssid, const std::string& pass) {
  LOG_INF("BLE_COMPANION", "Received Wi-Fi Provisioning for SSID: %s", ssid.c_str());

  if (ssid.empty()) {
    sendNotification("ERR:WIFI_INVALID_SSID");
    return;
  }

  pendingSsid_ = ssid;
  pendingPass_ = pass;
  wifiState_ = WifiTaskState::CONNECTING;
  wifiConnectStartTime_ = millis();

  WiFi.disconnect(true);
  delay(50);
  WiFi.mode(WIFI_STA);
  WiFi.begin(pendingSsid_.c_str(), pendingPass_.empty() ? nullptr : pendingPass_.c_str());

  sendNotification("STATUS:WIFI_CONNECTING");
  if (eventCallback_) {
    eventCallback_("WIFI_CONNECTING", pendingSsid_);
  }
}

void BleCompanionServer::sendStatusJson() {
  JsonDocument doc;
  doc["device"] = "reTerminal Sticky";
  doc["soc"] = "ESP32-S3";
  doc["version"] = CROSSPOINT_VERSION;

  doc["battery_pct"] = g_batteryMonitor.readPercentage();
  doc["battery_mv"] = g_batteryMonitor.readMillivolts();
  auto bStatus = g_batteryMonitor.readStatus();
  if (bStatus.chargingKnown) {
    doc["charging"] = bStatus.charging;
  }

  float tempC = 0.0f, humidity = 0.0f;
  if (g_envSensor.read(tempC, humidity)) {
    doc["temperature_c"] = tempC;
    doc["humidity_pct"] = humidity;
  }

  doc["free_heap"] = ESP.getFreeHeap();
  doc["total_heap"] = ESP.getHeapSize();
#if defined(BOARD_HAS_PSRAM)
  doc["free_psram"] = ESP.getFreePsram();
  doc["total_psram"] = ESP.getPsramSize();
#endif

  doc["wifi_connected"] = (WiFi.status() == WL_CONNECTED);
  if (WiFi.status() == WL_CONNECTED) {
    doc["ip"] = WiFi.localIP().toString();
    doc["ssid"] = WiFi.SSID();
    doc["rssi"] = WiFi.RSSI();
  }

  // Active book metadata
  if (!RECENT_BOOKS.getBooks().empty()) {
    const auto& book = RECENT_BOOKS.getBooks().front();
    doc["active_book"] = book.title;
    doc["active_author"] = book.author;
  }

  std::string jsonStr;
  serializeJson(doc, jsonStr);
  sendNotification("JSON_STATUS:" + jsonStr);
}

void BleCompanionServer::handleCloudConfig(const std::string& jsonPayload) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, jsonPayload);
  if (err) {
    LOG_ERR("BLE_COMPANION", "Failed to deserialize cloud config: %s", err.c_str());
    sendNotification("ERR:JSON_PARSE_FAILED");
    return;
  }

  if (CLOUD_CREDENTIALS.fromJson(doc.as<JsonVariantConst>())) {
    CLOUD_CREDENTIALS.saveToFile();
    LOG_INF("BLE_COMPANION", "Cloud credentials successfully updated from companion.");
    sendNotification("OK:CLOUD_CONFIG_SAVED");
    if (eventCallback_) {
      eventCallback_("CLOUD_CONFIG_SAVED", "SUCCESS");
    }
  } else {
    sendNotification("ERR:CLOUD_CONFIG_INVALID");
  }
}

void BleCompanionServer::handleKeyInjection(const std::string& keyName) {
  if (!inputManager_) {
    sendNotification("ERR:INPUT_MANAGER_UNAVAILABLE");
    return;
  }

  if (keyName == "PAGE_FORWARD" || keyName == "NEXT" || keyName == "RIGHT") {
    inputManager_->injectRelease(MappedInputManager::Button::PageForward);
    sendNotification("OK:KEY:PAGE_FORWARD");
  } else if (keyName == "PAGE_BACK" || keyName == "PREV" || keyName == "LEFT") {
    inputManager_->injectRelease(MappedInputManager::Button::PageBack);
    sendNotification("OK:KEY:PAGE_BACK");
  } else if (keyName == "CONFIRM" || keyName == "ENTER") {
    inputManager_->injectRelease(MappedInputManager::Button::Confirm);
    sendNotification("OK:KEY:CONFIRM");
  } else if (keyName == "BACK" || keyName == "ESC") {
    inputManager_->injectRelease(MappedInputManager::Button::Back);
    sendNotification("OK:KEY:BACK");
  } else {
    sendNotification("ERR:UNKNOWN_KEY:" + keyName);
  }
}

void BleCompanionServer::loop() {
  if (wifiState_ == WifiTaskState::CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      wifiState_ = WifiTaskState::CONNECTED;
      std::string ip = WiFi.localIP().toString().c_str();
      LOG_INF("BLE_COMPANION", "Wi-Fi Connected via Provisioning! IP: %s", ip.c_str());

      // Save credentials permanently
      WIFI_STORE.addCredential(pendingSsid_, pendingPass_);
      WIFI_STORE.setLastConnectedSsid(pendingSsid_);
      WIFI_STORE.saveToFile();

      sendNotification("OK:WIFI_CONNECTED:" + ip);
      if (eventCallback_) {
        eventCallback_("WIFI_CONNECTED", ip);
      }
    } else if (millis() - wifiConnectStartTime_ > 15000) {
      wifiState_ = WifiTaskState::FAILED;
      LOG_ERR("BLE_COMPANION", "Wi-Fi connection timed out.");
      sendNotification("ERR:WIFI_CONNECT_TIMEOUT");
      if (eventCallback_) {
        eventCallback_("WIFI_FAILED", "TIMEOUT");
      }
    }
  }
}
