#ifndef _ELEVENMPV_PLAYLIST_STORE_H_
#define _ELEVENMPV_PLAYLIST_STORE_H_

#include <psp2/types.h>

#include "playlist.h"

// The playlists on the memory card: one .m3u8 per playlist in PLAYLIST_DIR.
// The format and every rule about names live in playlist.c; this module only
// reaches the files. Every function closes what it opens and frees what it
// reserves before returning, whatever happens.

#define PLAYLIST_DIR "ux0:data/ElevenMPV/playlists"

// What the Playlists tab shows of a playlist, without keeping its entries.
typedef struct {
	char file[PLAYLIST_FILE_MAX];  // file name inside PLAYLIST_DIR
	char name[PLAYLIST_NAME_MAX];
	int count;
	SceUInt64 mtime;               // last modification, the tab's order
	// The albums of the cover (Playlist_MosaicAlbums), as cover keys.
	int mosaic_count;
	SceBool mosaic_pending;        // some album had not been looked at yet
	char mosaic_album[4][PLAYLIST_TAG_MAX];
	char mosaic_path[4][PLAYLIST_PATH_MAX];
} PlaylistSummary;

// Cleans up what an interrupted save left (Playlist_Recover) and reads every
// playlist once to summarize it, newest first. Files that are not playlists
// are skipped. `*out` is NULL when there are none; free it with free(). The
// number of summaries, or a negative value when memory runs out (and then
// `*out` is NULL). A missing folder is no playlists.
int PlaylistStore_List(PlaylistSummary **out);

// Reads the playlist in `file`. A file without a #PLAYLIST line takes its file
// name, without the extension, as its name.
Playlist_Result PlaylistStore_Load(const char *file, Playlist *out);

// Writes `pl` into `file` in the steps that survive a cut (playlist.h). 0, or
// a negative value with the file as it was.
int PlaylistStore_Save(const char *file, const Playlist *pl);

// Writes a new playlist under a free file name made from its name
// (Playlist_FileName), and copies that name into `file_out` (PLAYLIST_FILE_MAX
// bytes). 0 or a negative value.
int PlaylistStore_Create(const Playlist *pl, char *file_out);

// Gives the playlist in `file` the name `new_name`, keeping its entries: the
// content is saved first and the file renamed after, so a cut in between
// leaves a whole playlist that already has the new name. `file_out` receives
// the file name it ends up with. 0 or a negative value.
int PlaylistStore_Rename(const char *file, const char *new_name, char *file_out);

// Deletes the playlist in `file`, and any .tmp or .bak of it. The music it
// lists is not touched. 0 or a negative value.
int PlaylistStore_Delete(const char *file);

#endif
