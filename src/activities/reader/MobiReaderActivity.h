#pragma once

#include <memory>
#include <string>

#include "ParagraphReaderActivity.h"

/**
 * @brief Mobipocket (.mobi, .prc) PalmDoc unencrypted e-book reader.
 *
 * Reads Mobipocket/PalmDoc databases from Flash or SD card, decompresses
 * text records, extracts clean body text, and renders reflowable pages.
 */
class MobiReaderActivity final : public ParagraphReaderActivity {
 public:
  MobiReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                     bool allowFastInitialRefresh = false);
  ~MobiReaderActivity() override = default;

  bool loadBook() override;

 private:
  bool loadAndDecompressMobi();
  bool decompressPalmDoc(const uint8_t* in, size_t inLen, std::string& out);
};
