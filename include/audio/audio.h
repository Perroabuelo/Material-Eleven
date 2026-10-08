#ifndef _ELEVENMPV_AUDIO_H_
#define _ELEVENMPV_AUDIO_H_

#include <stddef.h>
#include <psp2/types.h>
#include <vita2d.h>

extern SceBool playing, paused;

typedef struct {
	SceBool has_meta;
    char title[64];
    char album[64];
    char artist[64];
    char year[64];
    char comment[64];
    char genre[64];
    vita2d_texture *cover_image;
    // Lyrics embedded in the track (FLAC, OGG and Opus LYRICS or
    // UNSYNCEDLYRICS, MP3 USLT) as raw UTF-8, malloc'd and NUL-terminated, or
    // NULL. Menu_InitMusic takes them right after Audio_Init and leaves NULL;
    // Audio_Term frees whatever is still here.
    char *lyrics;
    size_t lyrics_len;
} Audio_Metadata;

extern Audio_Metadata metadata;

int Audio_Init(const char *path);
// SCE_TRUE from the moment Audio_Init succeeds until Audio_Term runs, so
// other screens can tell whether a track is loaded and playing in the
// background after the user has navigated away from Now Playing.
SceBool Audio_HasTrack(void);
SceBool Audio_IsPaused(void);
void Audio_Pause(void);
void Audio_Stop(void);
SceUInt64 Audio_GetPosition(void);
SceUInt64 Audio_GetLength(void);
SceUInt64 Audio_GetPositionSeconds(void);
SceUInt64 Audio_GetLengthSeconds(void);
// Position in milliseconds, for the lyrics view. Like Audio_GetPosition it is
// the decoded position, a block or two (some 20 ms) ahead of what is heard.
SceUInt32 Audio_GetPositionMs(void);
SceUInt64 Audio_Seek(SceUInt64 index);
void Audio_Term(void);

// For the decoders' *_Init: copies `len` bytes of embedded lyrics to
// metadata.lyrics. Ignored when lyrics are already there (the first tag found
// wins), and when the text is empty or larger than LYRICS_MAX_BYTES.
void Audio_SetLyrics(const char *text, size_t len);

#endif
