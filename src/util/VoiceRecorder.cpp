#include "VoiceRecorder.h"

#include <Logging.h>
#include <Memory.h>

#include <cstring>

namespace VoiceRecorder {

static Microphone mic;
static bool initialized = false;
static bool recording = false;

static uint8_t* pcmBuffer = nullptr;
static size_t pcmCapacity = 0;
static size_t pcmRecordedBytes = 0;
static unsigned long recordStartTime = 0;
static unsigned long maxRecordDurationMs = 10000;

static constexpr size_t WAV_HEADER_SIZE = 44;

static void writeWavHeader(uint8_t* header, uint32_t pcmDataSize, uint32_t sampleRate) {
  uint32_t totalChunkSize = pcmDataSize + 36;
  uint32_t byteRate = sampleRate * 2;  // 16-bit mono = 2 bytes per sample

  memcpy(header, "RIFF", 4);
  memcpy(header + 4, &totalChunkSize, 4);
  memcpy(header + 8, "WAVE", 4);
  memcpy(header + 12, "fmt ", 4);

  uint32_t subchunk1Size = 16;
  uint16_t audioFormat = 1;  // PCM
  uint16_t numChannels = 1;  // Mono
  uint16_t blockAlign = 2;   // 16-bit mono
  uint16_t bitsPerSample = 16;

  memcpy(header + 16, &subchunk1Size, 4);
  memcpy(header + 20, &audioFormat, 2);
  memcpy(header + 22, &numChannels, 2);
  memcpy(header + 24, &sampleRate, 4);
  memcpy(header + 28, &byteRate, 4);
  memcpy(header + 32, &blockAlign, 2);
  memcpy(header + 34, &bitsPerSample, 2);
  memcpy(header + 36, "data", 4);
  memcpy(header + 40, &pcmDataSize, 4);
}

bool begin() {
  if (!initialized) {
    initialized = mic.begin(16000);
    if (!initialized) {
      LOG_ERR("VOICE", "Failed to initialize PDM microphone");
      return false;
    }
    LOG_INF("VOICE", "PDM microphone initialized at 16kHz");
  }
  return true;
}

bool startRecording(uint8_t maxSeconds) {
  if (!begin()) return false;

  release();

  maxRecordDurationMs = static_cast<unsigned long>(maxSeconds) * 1000UL;
  // 16kHz 16-bit mono = 32000 bytes/sec + 44 bytes header
  pcmCapacity = (16000 * 2 * maxSeconds) + WAV_HEADER_SIZE;
  pcmBuffer = static_cast<uint8_t*>(psram_malloc(pcmCapacity));
  if (!pcmBuffer) {
    LOG_ERR("VOICE", "Failed to allocate %zu bytes in PSRAM for voice recording", pcmCapacity);
    return false;
  }

  // Reserve first 44 bytes for WAV header
  pcmRecordedBytes = 0;
  recordStartTime = millis();
  recording = true;
  LOG_INF("VOICE", "Started voice recording (up to %u seconds)", maxSeconds);
  return true;
}

bool updateRecording() {
  if (!recording || !pcmBuffer) return false;

  if (millis() - recordStartTime >= maxRecordDurationMs) {
    recording = false;
    return false;
  }

  // Read samples into buffer (offset past 44-byte WAV header)
  uint8_t* dst = pcmBuffer + WAV_HEADER_SIZE + pcmRecordedBytes;
  size_t remainingBytes = pcmCapacity - (WAV_HEADER_SIZE + pcmRecordedBytes);
  size_t maxSamples = remainingBytes / sizeof(int16_t);

  if (maxSamples > 256) maxSamples = 256;

  int samplesRead = mic.read(reinterpret_cast<int16_t*>(dst), maxSamples, 20);
  if (samplesRead > 0) {
    pcmRecordedBytes += samplesRead * sizeof(int16_t);
  }

  return recording;
}

const uint8_t* stopRecording(size_t& wavBytes) {
  recording = false;
  if (!pcmBuffer || pcmRecordedBytes == 0) {
    wavBytes = 0;
    return nullptr;
  }

  writeWavHeader(pcmBuffer, pcmRecordedBytes, 16000);
  wavBytes = pcmRecordedBytes + WAV_HEADER_SIZE;
  LOG_INF("VOICE", "Voice recording complete: %zu bytes WAV", wavBytes);
  return pcmBuffer;
}

bool isRecording() { return recording; }

void release() {
  recording = false;
  if (pcmBuffer) {
    psram_free(pcmBuffer);
    pcmBuffer = nullptr;
    pcmCapacity = 0;
    pcmRecordedBytes = 0;
  }
}

}  // namespace VoiceRecorder
