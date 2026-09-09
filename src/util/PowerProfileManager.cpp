#include "PowerProfileManager.h"

#include <Arduino.h>
#include <HalStorage.h>
#include <Logging.h>
#include <WiFi.h>
#include <esp_sleep.h>

void PowerProfileManager::preparePeripheralsForSleep() {
  // Gracefully power off Wi-Fi modem domain
  if (WiFi.getMode() != WIFI_MODE_NULL) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }

  // Flush and prepare storage (SD card / SPI bus)
  HalStorage::getInstance().prepareForDeepSleep();
}

void PowerProfileManager::enterAmbientDeepSleep(uint32_t sleepMinutes, bool allowTouchWake) {
  LOG_INF("POWER", "Entering ambient deep sleep for %u minutes (touchWake=%d)", sleepMinutes, allowTouchWake);

  preparePeripheralsForSleep();

  // 1. Arm timer wakeup
  const uint64_t sleepMicros = static_cast<uint64_t>(sleepMinutes) * 60ULL * 1000000ULL;
  esp_sleep_enable_timer_wakeup(sleepMicros);

  // 2. Configure Power Button wakeup (GPIO 0 / Power Button active low)
#if defined(FREEINK_DEVICE_STICKY) || defined(CONFIG_IDF_TARGET_ESP32S3)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
#endif

  // 3. Enter Deep Sleep
  esp_deep_sleep_start();
}

PowerProfileManager::WakeReason PowerProfileManager::getWakeReason() {
  esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
  switch (cause) {
    case ESP_SLEEP_WAKEUP_TIMER:
      return WakeReason::Timer;
    case ESP_SLEEP_WAKEUP_EXT0:
    case ESP_SLEEP_WAKEUP_EXT1:
      return WakeReason::Button;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      return WakeReason::Touch;
    default:
      return WakeReason::Unknown;
  }
}
