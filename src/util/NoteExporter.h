#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "BookmarkEntry.h"

/**
 * NoteExporter
 *
 * Dedicated export engine for study notes and spaced-repetition flashcards.
 *
 * Features:
 * 1. Obsidian Markdown Export: Generates clean, standard Markdown files with YAML
 *    frontmatter (tags, device metadata, title, author, chapter numbers, reading progress,
 *    and blockquote highlights) under `/.crosspoint/export/notes/<sanitized_title>.md`.
 * 2. Anki Flashcard Deck Export: Appends vocabulary lookups from the e-book dictionary
 *    into an Anki-compatible tab-separated values file (`/.crosspoint/export/anki/vocab_anki.tsv`).
 *    Fields: `Word \t Definition \t BookTitle \t Timestamp`.
 */
namespace NoteExporter {

/**
 * Exports bookmarks, highlights, and annotations for a specific book to an Obsidian markdown document.
 *
 * @param bookTitle Title of the e-book.
 * @param bookAuthor Author of the e-book.
 * @param bookmarks Vector of bookmarks/highlights to serialize.
 * @return True if the file was created and written successfully.
 */
bool exportObsidianMarkdown(const std::string& bookTitle, const std::string& bookAuthor,
                            const std::vector<BookmarkEntry>& bookmarks);

/**
 * Appends a word and its dictionary definition to the Anki study deck.
 *
 * @param word The looked-up word.
 * @param definition The dictionary explanation/definition.
 * @param bookTitle Optional source book title.
 * @return True on successful write.
 */
bool exportAnkiFlashcard(const std::string& word, const std::string& definition,
                         const std::string& bookTitle = "");

}  // namespace NoteExporter
