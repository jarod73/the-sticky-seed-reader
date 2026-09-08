#pragma once

#include <Arduino.h>
#include <Microphone.h>

#include <cstdint>
#include <memory>

/**
 * VoiceRecorder
 *
 * Direct voice note and audio capture engine for the Knowles PDM microphone
 * on Seeed Studio reTerminal Sticky (GPIO39 CLK, GPIO38 DAT).
 *
 * Architecture & Memory Design:
 * - Samples at 16,000 Hz (16 kHz), 16-bit signed PCM mono (the universal speech-to-text standard).
 * - Allocates PCM buffers strictly in 8MB Octal PSRAM using `psram_malloc()`, preserving
 *   precious internal SRAM for display rendering and task stacks.
 * - Generates standard 44-byte RIFF WAV headers in-place on completion.
 * - Interfaced via USB companion protocol (`CMD:RECORD:START`, `CMD:RECORD:STOP`, `CMD:RECORD:FETCH`)
 *   for instant streaming to Whisper STT, Gemini, or transcription agents.
 */
namespace VoiceRecorder {

/**
 * Initializes the hardware PDM microphone driver.
 * Safe to call multiple times (idempotent).
 *
 * @return True if hardware initialization succeeded.
 */
bool begin();

/**
 * Begins streaming audio samples into PSRAM.
 *
 * @param maxSeconds Maximum recording time limit in seconds (default: 10s).
 * @return True if PSRAM allocation and I2S stream started successfully.
 */
bool startRecording(uint8_t maxSeconds = 10);

/**
 * Periodic poll method called in the main loop to read available I2S audio frames.
 *
 * @return True if actively recording, false if completed or stopped.
 */
bool updateRecording();

/**
 * Concludes active audio recording, writes the RIFF WAV header, and returns the finished buffer.
 *
 * @param wavBytes Output parameter receiving the total WAV file byte size.
 * @return Pointer to the WAV file in PSRAM, or nullptr if no audio was captured.
 */
const uint8_t* stopRecording(size_t& wavBytes);

/**
 * Checks whether audio capture is currently active.
 */
bool isRecording();

/**
 * Frees the audio buffer from PSRAM.
 */
void release();

}  // namespace VoiceRecorder
