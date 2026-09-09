#include "ReadingStatsStore.h"

#include <Logging.h>

#include <algorithm>
#include <ctime>

namespace {
std::string getTodayDateString() {
  time_t now = time(nullptr);
  if (now < 1600000000) {
    // If RTC not synced to epoch, use fixed fallback date slot
    return "2026-09-09";
  }
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);
  char buf[32];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
  return std::string(buf);
}
bool isConsecutiveDay(const std::string& prevDateStr, const std::string& curDateStr) {
  if (prevDateStr.size() != 10 || curDateStr.size() != 10) return false;
  struct tm tmPrev = {}, tmCur = {};
  if (sscanf(prevDateStr.c_str(), "%d-%d-%d", &tmPrev.tm_year, &tmPrev.tm_mon, &tmPrev.tm_mday) != 3) return false;
  if (sscanf(curDateStr.c_str(), "%d-%d-%d", &tmCur.tm_year, &tmCur.tm_mon, &tmCur.tm_mday) != 3) return false;
  tmPrev.tm_year -= 1900;
  tmPrev.tm_mon -= 1;
  tmCur.tm_year -= 1900;
  tmCur.tm_mon -= 1;
  time_t tPrev = mktime(&tmPrev);
  time_t tCur = mktime(&tmCur);
  if (tPrev == -1 || tCur == -1) return false;
  double diffSec = difftime(tCur, tPrev);
  return diffSec >= 43200.0 && diffSec <= 129600.0;
}
}  // namespace

void ReadingStatsStore::toJson(JsonDocument& doc) const {
  doc["total_seconds"] = totalReadingSeconds;
  doc["total_pages"] = totalPagesRead;
  doc["total_books_finished"] = totalBooksFinished;
  doc["average_wpm"] = averageWpm;
  doc["streak"] = currentDailyStreak;
  doc["last_date"] = lastReadDate;

  JsonObject history = doc["daily_minutes"].to<JsonObject>();
  for (const auto& [date, mins] : dailyMinutes) {
    history[date] = mins;
  }
}

bool ReadingStatsStore::fromJson(JsonVariantConst doc) {
  totalReadingSeconds = doc["total_seconds"] | 0;
  totalPagesRead = doc["total_pages"] | 0;
  totalBooksFinished = doc["total_books_finished"] | 0;
  averageWpm = doc["average_wpm"] | 220;
  currentDailyStreak = doc["streak"] | 0;
  lastReadDate = doc["last_date"] | "";

  dailyMinutes.clear();
  JsonObjectConst history = doc["daily_minutes"].as<JsonObjectConst>();
  for (JsonPairConst kv : history) {
    dailyMinutes[kv.key().c_str()] = kv.value().as<uint32_t>();
  }

  LOG_INF("STATS", "Loaded reading stats: %u mins, %u pages, %u WPM, %u day streak",
          totalReadingSeconds / 60, totalPagesRead, averageWpm, currentDailyStreak);
  return true;
}

void ReadingStatsStore::recordReadingSession(uint32_t seconds, uint32_t wordsRead) {
  if (seconds == 0) return;
  totalReadingSeconds += seconds;

  const std::string today = getTodayDateString();
  const uint32_t mins = (seconds + 59) / 60;
  dailyMinutes[today] += mins;

  // Calculate daily streak
  if (lastReadDate != today) {
    if (!lastReadDate.empty() && isConsecutiveDay(lastReadDate, today)) {
      currentDailyStreak++;
    } else {
      currentDailyStreak = 1;
    }
    lastReadDate = today;
  }

  // Update rolling WPM if words were provided
  if (wordsRead > 0 && seconds > 10) {
    const uint16_t sessionWpm = static_cast<uint16_t>((wordsRead * 60) / seconds);
    if (sessionWpm >= 80 && sessionWpm <= 800) {
      averageWpm = (averageWpm * 3 + sessionWpm) / 4;  // Exponential moving average
    }
  }

  // Prune history to last 30 days
  while (dailyMinutes.size() > 30) {
    dailyMinutes.erase(dailyMinutes.begin());
  }

  saveToFile();
}

void ReadingStatsStore::recordPageTurn(uint32_t estimatedWords) {
  totalPagesRead++;
  recordReadingSession(30, estimatedWords);
}

void ReadingStatsStore::recordBookFinished() {
  totalBooksFinished++;
  saveToFile();
}

std::vector<std::pair<std::string, uint32_t>> ReadingStatsStore::getLast7DaysMinutes() const {
  std::vector<std::pair<std::string, uint32_t>> result;
  result.reserve(7);

  // Return last up to 7 recorded dates, or fill empty if sparse
  for (auto it = dailyMinutes.rbegin(); it != dailyMinutes.rend() && result.size() < 7; ++it) {
    result.emplace_back(it->first, it->second);
  }
  std::reverse(result.begin(), result.end());

  if (result.empty()) {
    result.emplace_back("Today", 0);
  }
  return result;
}
