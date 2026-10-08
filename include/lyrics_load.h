#ifndef _ELEVENMPV_LYRICS_LOAD_H_
#define _ELEVENMPV_LYRICS_LOAD_H_

#include <stddef.h>

#include "lyrics.h"

// Finds the lyrics of the track at `path`: the .lrc next to it (same name,
// .lrc extension) and `embedded`, the text the decoder copied from the track's
// tags (metadata.lyrics, or NULL). Picks one with Lyrics_Choose and stores it
// in *out, empty when neither has lyrics.
//
// Takes ownership of `embedded` and frees it in every case. A .lrc larger than
// LYRICS_MAX_BYTES is skipped without reading it.
void LyricsLoad_ForTrack(const char *path, char *embedded, size_t embedded_len, Lyrics *out);

#endif
