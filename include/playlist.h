#ifndef _ELEVENMPV_PLAYLIST_H_
#define _ELEVENMPV_PLAYLIST_H_

#include <stddef.h>

// A user playlist: a name and an ordered list of tracks, read from and written
// to an .m3u8 file. Pure logic: no vita2d and no SCE headers, so it is tested
// on the PC (tests/test_playlist.c). Reaching the files is
// source/playlist_store.c's job.
//
// The file looks like this, UTF-8 without BOM and with \n line ends:
//
//     #EXTM3U
//     #PLAYLIST:Viaje a la costa
//     #EXTINF:-1,Daft Punk - Digital Love
//     #EXTALB:Discovery
//     ux0:music/Daft Punk/Discovery/03 Digital Love.flac

// The most tracks a playlist holds: the same ceiling as the queue and the
// Library index (LIBRARY_MAX_TRACKS), so a whole playlist always fits the queue.
#define PLAYLIST_MAX_ENTRIES 4000

// The field sizes of the Library index (LIBRARY_PATH_MAX, LIBRARY_TAG_MAX), so
// an entry copied from the index never gets cut.
#define PLAYLIST_PATH_MAX 256
#define PLAYLIST_TAG_MAX  64

// A name has at most this many characters. Each one can take four bytes in
// UTF-8, hence the buffer.
#define PLAYLIST_NAME_CHARS 64
#define PLAYLIST_NAME_MAX   (4 * PLAYLIST_NAME_CHARS + 1)

typedef struct {
	char path[PLAYLIST_PATH_MAX];  // absolute path on the console, ux0:...
	char title[PLAYLIST_TAG_MAX];  // what the track was called when it was
	char artist[PLAYLIST_TAG_MAX]; // added, shown only when the Library no
	char album[PLAYLIST_TAG_MAX];  // longer has it; "" when unknown
} Playlist_Entry;

typedef struct {
	char name[PLAYLIST_NAME_MAX]; // the visible name, as the user typed it;
	                              // "" when the file has no #PLAYLIST line
	Playlist_Entry *entries;      // NULL when count is 0 and nothing is reserved
	int count;
	int cap;
} Playlist;

typedef enum {
	PLAYLIST_OK = 0,
	PLAYLIST_INVALID = -1, // not a playlist: the file is ignored
	PLAYLIST_NOMEM = -2
} Playlist_Result;

// Reads `len` bytes of an .m3u8. Takes a BOM and \r\n line ends, and skips
// blank lines and the # lines it does not know. A path without an #EXTINF
// before it gets an empty title; one too long for PLAYLIST_PATH_MAX is skipped,
// and so is every entry past PLAYLIST_MAX_ENTRIES. Text without #EXTM3U as its
// first line, or with neither a #PLAYLIST line nor any entry, is
// PLAYLIST_INVALID.
//
// On anything but PLAYLIST_OK, `out` is left empty with nothing to free.
// `buf` may be NULL when `len` is 0. Free the result with Playlist_Free.
Playlist_Result Playlist_Parse(const char *buf, size_t len, Playlist *out);

// Writes the playlist as .m3u8 text into a new buffer of `*len` bytes (also
// NUL-terminated, the NUL not counted), which the caller frees. Line breaks
// inside a name or a tag are written as spaces, so the file reads back the
// same. PLAYLIST_OK or PLAYLIST_NOMEM.
Playlist_Result Playlist_Serialize(const Playlist *pl, char **out, size_t *len);

// Frees the entries and leaves `pl` empty, its name included. Takes an empty
// one too.
void Playlist_Free(Playlist *pl);

#endif
