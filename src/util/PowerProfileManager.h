#pragma once

#include <cstdint>

/**
 * @brief Power management and deep-sleep profile coordinator for reTerminal Sticky.
 *
 * Provides ultra-low-power ambient sleep management:
 * - Configures RTC and ESP32-S3 deep sleep timer (<15µA quiescent draw).
 * - Arms hardware wakeup sources:
 *     * Timer (periodic e-ink dashboard refreshes).
 *     * Power Button / Key interrupt (immediate user wake to interactive UI).
 *     * Capacitive Touch GT911 INT line (instant wake-on-touch).
 * - Gracefully powers down Wi-Fi, radio subsystems, and storage before sleep entry.
 */
class PowerProfileManager {
 public:
  enum class WakeReason {
    Timer,
    Button,
    Touch,
    Unknown,
  };

  /**
   * @brief Enters ultra-low-power deep sleep for the specified duration.
   * @param sleepMinutes Number of minutes to sleep before timer wakeup (e.g. 15, 30, 60).
   * @param allowTouchWake If true, arms touch interrupt to wake immediately on screen touch.
   */
  static void enterAmbientDeepSleep(uint32_t sleepMinutes, bool allowTouchWake = true);

  /**
   * @brief Determines why the MCU woke from deep sleep.
   */
  static WakeReason getWakeReason();

  /**
   * @brief Prepares hardware peripherals for sleep (turns off frontlight, closes storage).
   */
  static void preparePeripheralsForSleep();
};
