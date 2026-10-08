#ifndef _ELEVENMPV_LYRICS_LAYOUT_H_
#define _ELEVENMPV_LYRICS_LAYOUT_H_

#include <stddef.h>
#include <stdint.h>

#include "lyrics.h"

// Splits the lines of a Lyrics into rows that fit the width of the lyrics
// view. Pure logic: text is measured through a callback (UI_TextWidth on the
// console, a fixed width per code point on the PC), so it is tested on the PC
// (tests/test_lyrics_layout.c). Built once when the track opens, so drawing a
// frame neither measures nor allocates.

// No row is longer than this many bytes, whatever the measure says, so the
// view can copy a row to a fixed buffer to draw it.
#define LYRICS_LAYOUT_MAX_ROW_BYTES 512

typedef struct {
	uint32_t line; // index into Lyrics.lines
	uint32_t off;  // the row is Lyrics.text[off] ...
	uint32_t len;  // ... len bytes long, not NUL-terminated (0 for a blank line)
} LyricsLayout_Row;

typedef struct {
	LyricsLayout_Row *rows; // every line in order, each one with at least one row
	uint32_t count;
} LyricsLayout;

// Width of `len` bytes of `text`, which belong to line `line`: the caller
// decides the text size per line, so all the rows of a line share it.
typedef float (*LyricsLayout_Measure)(const char *text, size_t len, uint32_t line, void *ctx);

// Breaks every line at spaces so each row is at most `width` wide. A word
// wider than that is cut between code points, and two CJK characters (hangul,
// kana, kanji) may be split too, since those scripts do not always separate
// words with spaces. A cut never falls inside a UTF-8 sequence, and every row
// holds at least one code point. Spaces where a line breaks are dropped.
//
// Returns 0, or -1 when memory runs out (and then `out` is empty). Free the
// result with LyricsLayout_Free.
int LyricsLayout_Build(const Lyrics *lyrics, float width, LyricsLayout_Measure measure, void *ctx, LyricsLayout *out);

// First row of line `line`, or layout->count when there is no such line.
uint32_t LyricsLayout_FirstRow(const LyricsLayout *layout, uint32_t line);

// Frees the rows and leaves `layout` empty. Takes an empty one too.
void LyricsLayout_Free(LyricsLayout *layout);

#endif
