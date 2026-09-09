#pragma once

#include <ArduinoJson.h>
#include <PersistableStore.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

/**
 * @brief Persistent Reading Statistics Store (Kobo-style reading telemetry).
 *
 * Tracks reading time, rolling words-per-minute (WPM), daily streaks,
 * and 7-day reading history on E-Ink without privacy-invading cloud telemetry.
 */
class ReadingStatsStore : public PersistableStore<ReadingStatsStore> {
 private:
  ReadingStatsStore() = default;
  ~ReadingStatsStore() = default;

  friend class PersistableStore<ReadingStatsStore>;

  uint32_t totalReadingSeconds = 0;
  uint32_t totalPagesRead = 0;
  uint32_t totalBooksFinished = 0;
  uint16_t averageWpm = 220;
  uint16_t currentDailyStreak = 0;
  std::string lastReadDate;  // YYYY-MM-DD
  std::map<std::string, uint32_t> dailyMinutes;  // YYYY-MM-DD -> minutes

 public:
  static const char* getFilePath() { return "/.crosspoint/stats.json"; }

  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);

  void recordReadingSession(uint32_t seconds, uint32_t wordsRead = 0);
  void recordPageTurn(uint32_t estimatedWords = 250);
  void recordBookFinished();

  uint32_t getTotalReadingMinutes() const { return totalReadingSeconds / 60; }
  uint32_t getTotalPagesRead() const { return totalPagesRead; }
  uint32_t getTotalBooksFinished() const { return totalBooksFinished; }
  uint16_t getAverageWpm() const { return averageWpm > 0 ? averageWpm : 220; }
  uint16_t getDailyStreak() const { return currentDailyStreak; }

  uint16_t getEstimatedMinutesLeft(uint32_t remainingWords) const {
    const uint16_t wpm = getAverageWpm();
    return (wpm > 0) ? static_cast<uint16_t>((remainingWords + wpm - 1) / wpm) : 1;
  }

  std::vector<std::pair<std::string, uint32_t>> getLast7DaysMinutes() const;
};

#define READING_STATS ReadingStatsStore::getInstance()
