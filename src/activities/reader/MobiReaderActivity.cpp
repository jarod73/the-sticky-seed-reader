#include "MobiReaderActivity.h"

#include <FsHelpers.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>

#include "util/ReadabilityExtractor.h"

MobiReaderActivity::MobiReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                                       const bool allowFastInitialRefresh)
    : ParagraphReaderActivity("MobiReader", renderer, mappedInput, std::move(bookPath), allowFastInitialRefresh) {}

bool MobiReaderActivity::loadBook() {
  if (loadAndDecompressMobi()) {
    paginate();
    loadProgress();
    return !pages_.empty();
  }
  return false;
}

bool MobiReaderActivity::decompressPalmDoc(const uint8_t* in, size_t inLen, std::string& out) {
  size_t i = 0;
  out.reserve(out.size() + inLen * 2);

  while (i < inLen) {
    const uint8_t b = in[i++];
    if (b == 0x00) {
      out.push_back('\0');
    } else if (b <= 0x08) {
      // Literal sequence of length b
      for (uint8_t k = 0; k < b && i < inLen; ++k) {
        out.push_back(static_cast<char>(in[i++]));
      }
    } else if (b <= 0x7f) {
      // Single literal byte
      out.push_back(static_cast<char>(b));
    } else if (b <= 0xbf) {
      // Distance/length pair
      if (i >= inLen) break;
      const uint8_t next = in[i++];
      const size_t dist = ((b & 0x3f) << 3) | (next >> 5);
      const size_t len = (next & 0x07) + 3;

      if (dist == 0 || dist > out.size()) continue;

      const size_t startPos = out.size() - dist;
      for (size_t k = 0; k < len; ++k) {
        out.push_back(out[startPos + k]);
      }
    } else {
      // Space + character (0xc0..0xff)
      out.push_back(' ');
      out.push_back(static_cast<char>(b ^ 0x80));
    }
  }

  return true;
}

bool MobiReaderActivity::loadAndDecompressMobi() {
  HalFile file;
  if (!Storage.openFileForRead("MOBI", bookPath.c_str(), file)) {
    LOG_ERR("MOBI", "Failed to open MOBI file: %s", bookPath.c_str());
    return false;
  }

  // 1. Read PDB Header (32 bytes title, numRecords at offset 76)
  char titleBuf[33] = {0};
  file.read(reinterpret_cast<uint8_t*>(titleBuf), 32);
  title_ = titleBuf;

  file.seek(76);
  uint8_t recCountBuf[2];
  file.read(recCountBuf, 2);
  const uint16_t numRecords = (static_cast<uint16_t>(recCountBuf[0]) << 8) | recCountBuf[1];

  if (numRecords < 2) {
    LOG_ERR("MOBI", "Invalid MOBI: record count %u < 2", numRecords);
    return false;
  }

  // 2. Read record offsets
  std::vector<uint32_t> recordOffsets;
  recordOffsets.reserve(std::min<size_t>(numRecords, 512));

  for (uint16_t r = 0; r < numRecords && r < 512; ++r) {
    uint8_t entry[8];
    if (file.read(entry, 8) != 8) break;
    const uint32_t offset = (static_cast<uint32_t>(entry[0]) << 24) | (static_cast<uint32_t>(entry[1]) << 16) |
                            (static_cast<uint32_t>(entry[2]) << 8) | static_cast<uint32_t>(entry[3]);
    recordOffsets.push_back(offset);
  }

  // 3. Read PalmDOC header (Record 0)
  if (recordOffsets.size() < 2) return false;
  file.seek(recordOffsets[0]);
  uint8_t palmDocHdr[16];
  file.read(palmDocHdr, 16);
  const uint16_t compression = (static_cast<uint16_t>(palmDocHdr[0]) << 8) | palmDocHdr[1];
  const uint16_t textRecordCount = (static_cast<uint16_t>(palmDocHdr[8]) << 8) | palmDocHdr[9];

  LOG_INF("MOBI", "MOBI '%s': compression=%u, textRecords=%u", title_.c_str(), compression, textRecordCount);

  // 4. Decompress text records
  std::string fullHtml;
  fullHtml.reserve(65536);

  const size_t recordsToRead = std::min<size_t>(textRecordCount, recordOffsets.size() - 1);
  for (size_t r = 1; r <= recordsToRead; ++r) {
    const uint32_t start = recordOffsets[r];
    const uint32_t end = (r + 1 < recordOffsets.size()) ? recordOffsets[r + 1] : start + 4096;
    const size_t recSize = (end > start) ? (end - start) : 0;
    if (recSize == 0 || recSize > 16384) continue;

    file.seek(start);
    std::vector<uint8_t> recBuf(recSize);
    if (file.read(recBuf.data(), recSize) != static_cast<int>(recSize)) continue;

    if (compression == 2) {
      decompressPalmDoc(recBuf.data(), recSize, fullHtml);
    } else {
      fullHtml.append(reinterpret_cast<char*>(recBuf.data()), recSize);
    }
  }

  auto article = ReadabilityExtractor::extract(fullHtml);
  paragraphs_ = std::move(article.paragraphs);
  isLoaded_ = !paragraphs_.empty();
  LOG_INF("MOBI", "Extracted %zu paragraphs from MOBI", paragraphs_.size());
  return isLoaded_;
}
