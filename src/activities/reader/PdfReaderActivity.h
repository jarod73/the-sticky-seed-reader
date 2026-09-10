#pragma once

#include <memory>
#include <string>

#include "ParagraphReaderActivity.h"

/**
 * @brief Plaintext-stream extraction reader for PDF documents.
 *
 * Scans uncompressed and lightly encoded PDF text objects, stripping PDF operator
 * syntax to provide reflowable E-Ink reading of documents.
 */
class PdfReaderActivity final : public ParagraphReaderActivity {
 public:
  PdfReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                    bool allowFastInitialRefresh = false);
  ~PdfReaderActivity() override = default;

  bool loadBook() override;

 private:
  bool extractPdfText();
};
