#include "NoteExporter.h"

#include <HalStorage.h>
#include <Logging.h>

#include <algorithm>
#include <sstream>

namespace {

std::string sanitizeFilename(const std::string& name) {
  std::string clean = name;
  for (char& c : clean) {
    if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
      c = '_';
    }
  }
  if (clean.empty()) clean = "untitled";
  return clean;
}

std::string sanitizeTsv(const std::string& text) {
  std::string clean;
  clean.reserve(text.size());
  for (char c : text) {
    if (c == '\t') {
      clean.append("    ");
    } else if (c == '\r') {
      continue;
    } else if (c == '\n') {
      clean.append("<br>");
    } else {
      clean.push_back(c);
    }
  }
  return clean;
}

}  // namespace

namespace NoteExporter {

bool exportObsidianMarkdown(const std::string& bookTitle, const std::string& bookAuthor,
                            const std::vector<BookmarkEntry>& bookmarks) {
  if (bookmarks.empty()) {
    LOG_INF("EXP", "No bookmarks to export for '%s'", bookTitle.c_str());
    return false;
  }

  Storage.ensureDirectoryExists("/.crosspoint");
  Storage.ensureDirectoryExists("/.crosspoint/export");
  Storage.ensureDirectoryExists("/.crosspoint/export/notes");

  std::string filename = "/.crosspoint/export/notes/" + sanitizeFilename(bookTitle) + ".md";
  HalFile file = Storage.open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC);
  if (!file) {
    LOG_ERR("EXP", "Failed to create notes file '%s'", filename.c_str());
    return false;
  }

  std::string frontmatter;
  frontmatter += "---\n";
  frontmatter += "title: \"" + bookTitle + "\"\n";
  frontmatter += "author: \"" + bookAuthor + "\"\n";
  frontmatter += "device: \"Seeed Studio reTerminal Sticky\"\n";
  frontmatter += "tags:\n";
  frontmatter += "  - crosspoint\n";
  frontmatter += "  - reading-notes\n";
  frontmatter += "---\n\n";

  frontmatter += "# " + bookTitle + "\n";
  if (!bookAuthor.empty()) {
    frontmatter += "**Author:** " + bookAuthor + "\n\n";
  }
  frontmatter += "## Bookmarks & Highlights\n\n";

  file.write(reinterpret_cast<const uint8_t*>(frontmatter.data()), frontmatter.size());

  for (size_t i = 0; i < bookmarks.size(); i++) {
    const auto& b = bookmarks[i];
    std::string entry;
    entry += "### Note " + std::to_string(i + 1) + "\n";
    if (!b.summary.empty()) {
      entry += "> " + b.summary + "\n\n";
    }
    entry += "- **Chapter:** " + std::to_string(b.computedSpineIndex + 1) + "\n";
    entry += "- **Progress:** " + std::to_string(static_cast<int>(b.percentage * 100.0f)) + "%\n\n";
    file.write(reinterpret_cast<const uint8_t*>(entry.data()), entry.size());
  }

  LOG_INF("EXP", "Exported %zu notes to '%s'", bookmarks.size(), filename.c_str());
  return true;
}

bool exportAnkiFlashcard(const std::string& word, const std::string& definition,
                         const std::string& bookTitle) {
  if (word.empty() || definition.empty()) return false;

  Storage.ensureDirectoryExists("/.crosspoint");
  Storage.ensureDirectoryExists("/.crosspoint/export");
  Storage.ensureDirectoryExists("/.crosspoint/export/anki");

  const char* filename = "/.crosspoint/export/anki/vocab_anki.tsv";
  HalFile file = Storage.open(filename, O_WRONLY | O_CREAT | O_AT_END);
  if (!file) {
    LOG_ERR("EXP", "Failed to open Anki TSV file '%s'", filename);
    return false;
  }

  std::string row;
  row += sanitizeTsv(word);
  row += '\t';
  row += sanitizeTsv(definition);
  row += '\t';
  row += sanitizeTsv(bookTitle);
  row += '\n';

  file.write(reinterpret_cast<const uint8_t*>(row.data()), row.size());
  LOG_INF("EXP", "Exported word '%s' to Anki deck", word.c_str());
  return true;
}

}  // namespace NoteExporter
