#pragma once

#include <memory>
#include <string>

#include "ParagraphReaderActivity.h"

/**
 * @brief FictionBook 2.0 (.fb2, .fb2.zip) XML e-book reader.
 *
 * Parses structured FictionBook XML markup into chapters, headings, and
 * reflowable paragraphs on E-Ink.
 */
class Fb2ReaderActivity final : public ParagraphReaderActivity {
 public:
  Fb2ReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, std::string bookPath,
                    bool allowFastInitialRefresh = false);
  ~Fb2ReaderActivity() override = default;

  bool loadBook() override;

 private:
  bool parseFb2Xml(const std::string& xmlContent);
};
