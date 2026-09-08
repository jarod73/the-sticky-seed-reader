#pragma once

#include <Arduino.h>
#include <GfxRenderer.h>
#include <HalStorage.h>

#include "MappedInputManager.h"

/**
 * UsbSerialCompanion
 *
 * High-speed companion communication protocol over USB Serial (CDC/UART0 at 921,600 baud).
 * Allows desktop tools, WebSerial browser apps, and Python scripts to interface directly
 * with Seeed Studio reTerminal Sticky.
 *
 * Supported Commands:
 * - `CMD:PING` -> Returns `PONG` (Heartbeat)
 * - `CMD:STATUS` -> Emits JSON with battery voltage, current mA, room temperature, RH%, RAM & PSRAM metrics.
 * - `CMD:PUT:<path>:<size>` -> Streams a binary book or file directly to SD/SPIFFS.
 * - `CMD:LS:<path>` -> Emits JSON array of directory contents.
 * - `CMD:DEL:<path>` -> Deletes a file.
 * - `CMD:KEY:<NEXT|PREV|CONFIRM|BACK>` -> Synthetic keypress injection for remote page turns.
 * - `CMD:SCREENSHOT` -> Dumps raw 48,000-byte E-Ink framebuffer over serial.
 * - `CMD:RECORD:START:<seconds>` -> Captures PDM microphone audio.
 * - `CMD:RECORD:STOP` -> Concludes recording.
 * - `CMD:RECORD:FETCH` -> Streams 16kHz WAV audio bytes.
 */
namespace UsbSerialCompanion {

/**
 * Initializes telemetry sensors for status reporting.
 */
void begin();

/**
 * Periodic update loop invoked on every main firmware cycle.
 */
void update();

/**
 * Dispatches and processes incoming serial commands.
 *
 * @param serial Active stream (e.g. Serial).
 * @param renderer Reference to GfxRenderer (for screenshots).
 * @param mappedInput Reference to MappedInputManager (for remote key injection).
 * @return True if a companion command was recognized and handled.
 */
bool handleSerial(Stream& serial, GfxRenderer& renderer, MappedInputManager& mappedInput);

}  // namespace UsbSerialCompanion
