#ifndef _ELEVENMPV_LYRICS_H_
#define _ELEVENMPV_LYRICS_H_

#include <stddef.h>
#include <stdint.h>

// Lyrics of the track that is playing: the LRC parser, the plain text reader
// and the rule that picks between the .lrc file and the embedded lyrics. Pure
// logic: no vita2d and no SCE headers, so it is tested on the PC
// (tests/test_lyrics.c). Reading the file is source/lyrics_load.c's job.

// A .lrc or an embedded text larger than this is ignored as if it did not
// exist. The loaders check it before reading (source/lyrics_load.c, Audio_SetLyrics).
#define LYRICS_MAX_BYTES (64 * 1024)

// Converting to UTF-8 can triple a text (every Windows-1252 byte, or every
// invalid byte that becomes U+FFFD), so the parser takes up to this much.
#define LYRICS_MAX_TEXT_BYTES (3 * LYRICS_MAX_BYTES)

// Bits of Lyrics_Line.flags from this one up are left to the caller, which
// can mark lines with what it works out about them once (the lyrics view
// stores whether a line needs the fallback font).
#define LYRICS_LINE_CALLER_FLAGS 0x80

// Lyrics_Line.flags: the whole line is between brackets, like a Genius
// section header "[Verse: Rumi, Zoey]". It is drawn dimmed.
#define LYRICS_LINE_HEADER 0x01

typedef struct {
	uint32_t ms;  // start time; 0 in lyrics without timestamps
	uint32_t off; // the line's text is text[off], NUL-terminated...
	uint32_t len; // ...and len bytes long
	uint8_t flags;
} Lyrics_Line;

typedef struct {
	char *text;         // one buffer with the text of every line
	Lyrics_Line *lines; // sorted by time when synced; a line with two
	                    // timestamps shows up twice, pointing at the same text
	uint32_t count;     // 0 means no lyrics, and then both pointers are NULL
	int synced;
} Lyrics;

// Parses `len` bytes of UTF-8 (a .lrc after TextEncoding_ToUtf8, or embedded
// lyrics). Lyrics with at least one valid [mm:ss], [mm:ss.x], [mm:ss.xx] or
// [mm:ss.xxx] are synced: lines without a valid timestamp are dropped, word
// marks <mm:ss.xx> are removed, [offset:N] is applied and the lines are
// sorted by time, keeping the file order for equal times. Otherwise every line
// is kept in order, blank ones included. In both cases the known metadata tags
// ([ti:], [ar:], [al:], [au:], [by:], [length:], [offset:], [re:], [ve:]) are
// dropped and trailing spaces are trimmed. Text without any non-blank line,
// or longer than LYRICS_MAX_TEXT_BYTES, gives count 0.
//
// Returns 0, or -1 when memory runs out (and then count is 0 too). `utf8` may
// be NULL when `len` is 0. Free the result with Lyrics_Free.
int Lyrics_Parse(const char *utf8, size_t len, Lyrics *out);

// Index of the line playing at `ms`: the last one whose time is <= ms, or -1
// before the first one and for lyrics without timestamps.
int Lyrics_LineAt(const Lyrics *lyrics, uint32_t ms);

// Picks the lyrics to show between the .lrc file and the embedded text: the
// one with timestamps wins, and with a tie the .lrc wins. Lyrics with count 0
// do not count. Moves the winner into *out, frees the other one, and leaves
// both sources empty.
void Lyrics_Choose(Lyrics *from_lrc, Lyrics *embedded, Lyrics *out);

// Frees the buffers and leaves `lyrics` empty. Takes an empty one too.
void Lyrics_Free(Lyrics *lyrics);

#endif
