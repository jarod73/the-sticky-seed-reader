#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ReaderActivity.h"
#include "util/TextWrapUtils.h"

/**
 * @brief Mobipocket / PalmDOC (.mobi, .prc, .azw) e-book reader.
 *
 * Parses Palm Database (PDB) headers and implements streaming PalmDOC LZ77
 * decompression to extract reflowable book text onto E-Ink.
 */
class MobiReaderActivity final : public ReaderActivity {
 public:
  MobiReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                     bool allowFastInitialRefresh = false);
  ~MobiReaderActivity() override = default;

  bool loadBook() override;
  std::string getBookTitle() const override;
  bool pageTurn(bool isForward) override;
  bool isAtEndOfBook() const override;
  void renderBook() override;

  static bool decompressPalmDoc(const uint8_t* compressed, size_t compressedSize, std::string& outText);

 private:
  bool loadAndDecompressMobi();
  void paginate();

  std::string title_;
  std::vector<std::string> paragraphs_;
  std::vector<std::string> pages_;
  size_t currentPage_ = 0;
  bool isLoaded_ = false;
};
