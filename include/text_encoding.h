#ifndef _ELEVENMPV_TEXT_ENCODING_H_
#define _ELEVENMPV_TEXT_ENCODING_H_

#include <stddef.h>

// Turns the bytes of a text file of unknown encoding into valid UTF-8, for the
// .lrc lyrics files (source/lyrics_load.c). Pure logic: no vita2d and no SCE
// headers, so it is tested on the PC (tests/test_text_encoding.c).

// Converts `len` bytes at `in` to UTF-8, deciding the encoding in layers:
//   1. a UTF-8 BOM is dropped and the rest is taken as UTF-8;
//   2. a UTF-16 BOM (FF FE little endian, FE FF big endian) means UTF-16;
//   3. without a BOM, text that is valid UTF-8 is copied as is;
//   4. anything else is Windows-1252.
// The result is always valid UTF-8 whatever the input: invalid UTF-8 after a
// BOM, lone UTF-16 surrogates, an odd trailing UTF-16 byte and the five
// unassigned Windows-1252 bytes all become U+FFFD.
//
// On success returns 0 and stores in *out a malloc'd, NUL-terminated buffer
// (the caller frees it) and its length without the NUL in *out_len. Returns -1
// and stores NULL when memory runs out. `in` may be NULL when `len` is 0.
int TextEncoding_ToUtf8(const unsigned char *in, size_t len, char **out, size_t *out_len);

// Copies `len` bytes of text that should already be UTF-8 (lyrics embedded in
// a track), replacing every invalid sequence with U+FFFD. Same contract as
// TextEncoding_ToUtf8.
int TextEncoding_SanitizeUtf8(const unsigned char *in, size_t len, char **out, size_t *out_len);

// Length of the valid UTF-8 sequence that starts at `s`, looking at no more
// than `avail` bytes, or 0 when it is not valid (overlong forms, surrogates,
// values above U+10FFFF, stray continuation bytes, truncated sequences).
size_t TextEncoding_Utf8SeqLen(const unsigned char *s, size_t avail);

#endif
