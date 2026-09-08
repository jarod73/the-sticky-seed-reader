#include "UsbSerialCompanion.h"

#include <ArduinoJson.h>
#include <BatteryMonitor.h>
#include <EnvironmentSensor.h>
#include <Logging.h>

#include "VoiceRecorder.h"

namespace UsbSerialCompanion {

static BatteryMonitor batteryMonitor;
static EnvironmentSensor envSensor;
static bool sensorsInitialized = false;

void begin() {
  if (!sensorsInitialized) {
    envSensor.begin();
    sensorsInitialized = true;
  }
}

void update() {
  if (VoiceRecorder::isRecording()) {
    VoiceRecorder::updateRecording();
  }
}

static void sendJsonStatus(Stream& serial) {
  begin();
  JsonDocument doc;
  doc["device"] = "reTerminal Sticky";
  doc["soc"] = "ESP32-S3";
  doc["battery_pct"] = batteryMonitor.readPercentage();
  doc["battery_mv"] = batteryMonitor.readMillivolts();

  int16_t currentMa = 0;
  if (batteryMonitor.readCurrentMa(currentMa)) {
    doc["battery_ma"] = currentMa;
  }
  uint16_t timeToEmpty = 0;
  if (batteryMonitor.readTimeToEmptyMinutes(timeToEmpty)) {
    doc["time_to_empty_min"] = timeToEmpty;
  }

  float tempC = 0.0f;
  float humidity = 0.0f;
  if (envSensor.read(tempC, humidity)) {
    doc["temperature_c"] = tempC;
    doc["humidity_pct"] = humidity;
  }

  doc["free_heap"] = ESP.getFreeHeap();
  doc["total_heap"] = ESP.getHeapSize();
#if defined(BOARD_HAS_PSRAM)
  doc["free_psram"] = ESP.getFreePsram();
  doc["total_psram"] = ESP.getPsramSize();
#endif
  doc["sd_ready"] = Storage.ready();

  serial.print("JSON_STATUS:");
  serializeJson(doc, serial);
  serial.println();
}

static void handlePutFile(Stream& serial, const String& line) {
  // Format: CMD:PUT:<filepath>:<filesize>
  int firstColon = line.indexOf(':', 8);
  if (firstColon < 0) {
    serial.println("ERR:PUT_FORMAT");
    return;
  }

  String filePath = line.substring(8, firstColon);
  size_t fileSize = line.substring(firstColon + 1).toInt();
  if (fileSize == 0 || filePath.length() == 0) {
    serial.println("ERR:INVALID_ARGS");
    return;
  }

  // Ensure leading slash
  if (!filePath.startsWith("/")) {
    filePath = "/" + filePath;
  }

  HalFile file = Storage.open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
  if (!file) {
    serial.printf("ERR:OPEN_FAILED:%s\n", filePath.c_str());
    return;
  }

  serial.println("READY_FOR_DATA");

  size_t bytesReceived = 0;
  uint8_t buffer[512];
  unsigned long lastByteTime = millis();

  while (bytesReceived < fileSize) {
    if (serial.available() > 0) {
      size_t toRead = std::min((size_t)serial.available(), sizeof(buffer));
      toRead = std::min(toRead, fileSize - bytesReceived);
      size_t n = serial.readBytes(buffer, toRead);
      if (n > 0) {
        file.write(buffer, n);
        bytesReceived += n;
        lastByteTime = millis();
      }
    } else {
      if (millis() - lastByteTime > 5000) {
        serial.println("ERR:TIMEOUT");
        file.close();
        return;
      }
      delay(1);
    }
  }

  file.close();
  serial.printf("OK:SAVED:%s:%zu\n", filePath.c_str(), bytesReceived);
}

static void handleListFiles(Stream& serial, const String& path) {
  std::vector<String> files = Storage.listFiles(path.c_str(), 100);
  JsonDocument doc;
  JsonArray arr = doc.to<JsonArray>();
  for (const auto& f : files) {
    arr.add(f.c_str());
  }
  serial.print("JSON_FILES:");
  serializeJson(doc, serial);
  serial.println();
}

bool handleSerial(Stream& serial, GfxRenderer& renderer, MappedInputManager& mappedInput) {
  if (serial.available() <= 0) return false;

  String line = serial.readStringUntil('\n');
  line.trim();

  if (!line.startsWith("CMD:")) return false;

  String cmd = line.substring(4);
  cmd.trim();

  if (cmd == "PING") {
    serial.println("PONG");
    return true;
  }

  if (cmd == "STATUS") {
    sendJsonStatus(serial);
    return true;
  }

  if (cmd == "SCREENSHOT") {
    const uint32_t bufferSize = renderer.getBufferSize();
    serial.printf("SCREENSHOT_START:%d\n", bufferSize);
    uint8_t* buf = renderer.getFrameBuffer();
    serial.write(buf, bufferSize);
    serial.printf("SCREENSHOT_END\n");
    return true;
  }

  if (cmd.startsWith("RECORD:START")) {
    uint8_t secs = 10;
    if (cmd.startsWith("RECORD:START:")) {
      secs = cmd.substring(13).toInt();
      if (secs == 0) secs = 10;
    }
    if (VoiceRecorder::startRecording(secs)) {
      serial.printf("OK:RECORDING_STARTED:%u\n", secs);
    } else {
      serial.println("ERR:MIC_UNAVAILABLE");
    }
    return true;
  }

  if (cmd == "RECORD:STOP") {
    size_t wavBytes = 0;
    const uint8_t* wav = VoiceRecorder::stopRecording(wavBytes);
    if (wav && wavBytes > 0) {
      serial.printf("OK:RECORDING_STOPPED:%zu\n", wavBytes);
    } else {
      serial.println("ERR:NO_AUDIO");
    }
    return true;
  }

  if (cmd == "RECORD:FETCH") {
    size_t wavBytes = 0;
    const uint8_t* wav = VoiceRecorder::stopRecording(wavBytes);
    if (wav && wavBytes > 0) {
      serial.printf("WAV_START:%zu\n", wavBytes);
      serial.write(wav, wavBytes);
      serial.println("\nWAV_END");
      VoiceRecorder::release();
    } else {
      serial.println("ERR:NO_AUDIO");
    }
    return true;
  }

  if (cmd.startsWith("PUT:")) {
    handlePutFile(serial, line);
    return true;
  }

  if (cmd.startsWith("LS")) {
    String path = "/";
    if (cmd.startsWith("LS:")) {
      path = cmd.substring(3);
    }
    handleListFiles(serial, path);
    return true;
  }

  if (cmd.startsWith("DEL:")) {
    String path = cmd.substring(4);
    if (Storage.remove(path.c_str())) {
      serial.printf("OK:DELETED:%s\n", path.c_str());
    } else {
      serial.printf("ERR:DELETE_FAILED:%s\n", path.c_str());
    }
    return true;
  }

  if (cmd.startsWith("KEY:")) {
    String key = cmd.substring(4);
    if (key == "NEXT" || key == "RIGHT" || key == "PAGEDOWN") {
      mappedInput.injectRelease(MappedInputManager::Button::PageForward);
      serial.println("OK:KEY:PAGE_FORWARD");
    } else if (key == "PREV" || key == "LEFT" || key == "PAGEUP") {
      mappedInput.injectRelease(MappedInputManager::Button::PageBack);
      serial.println("OK:KEY:PAGE_BACK");
    } else if (key == "CONFIRM" || key == "ENTER") {
      mappedInput.injectRelease(MappedInputManager::Button::Confirm);
      serial.println("OK:KEY:CONFIRM");
    } else if (key == "BACK" || key == "ESC") {
      mappedInput.injectRelease(MappedInputManager::Button::Back);
      serial.println("OK:KEY:BACK");
    } else {
      serial.printf("ERR:UNKNOWN_KEY:%s\n", key.c_str());
    }
    return true;
  }

  if (cmd == "HELP") {
    serial.println("COMMANDS: STATUS, PUT:<path>:<size>, LS:<path>, DEL:<path>, KEY:<name>, PING, SCREENSHOT");
    return true;
  }

  return false;
}

}  // namespace UsbSerialCompanion
