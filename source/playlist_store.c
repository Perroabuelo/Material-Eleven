#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <psp2/rtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cover.h"
#include "debug_log.h"
#include "playlist.h"
#include "playlist_store.h"

// A whole playlist of PLAYLIST_MAX_ENTRIES with long paths and tags is under
// 3 MB; anything bigger is not one of ours and is not read.
#define STORE_MAX_FILE_BYTES (4 * 1024 * 1024)

// Room for PLAYLIST_DIR, a file name and the .tmp or .bak after it.
#define STORE_PATH_MAX 160

// Bits of a file's set of leftovers, as Playlist_Recover takes them.
#define STORE_HAS_MAIN 1
#define STORE_HAS_TMP  2
#define STORE_HAS_BAK  4

static void Store_Path(char *dst, const char *file, const char *ext) {
	snprintf(dst, STORE_PATH_MAX, "%s/%s%s", PLAYLIST_DIR, file, ext);
}

static SceBool Store_Exists(const char *file, const char *ext) {
	char path[STORE_PATH_MAX];
	SceIoStat stat;

	Store_Path(path, file, ext);
	return sceIoGetstat(path, &stat) >= 0 ? SCE_TRUE : SCE_FALSE;
}

// Whether `name` ends in `suffix`, ignoring case.
static SceBool Store_EndsWith(const char *name, const char *suffix) {
	size_t n = strlen(name);
	size_t s = strlen(suffix);
	return n > s && strcasecmp(name + n - s, suffix) == 0;
}

// ---------------------------------------------------------------------------
// reading and writing one playlist

Playlist_Result PlaylistStore_Load(const char *file, Playlist *out) {
	char path[STORE_PATH_MAX];
	Playlist_Result result = PLAYLIST_INVALID;
	char *buf = NULL;

	memset(out, 0, sizeof(*out));
	Store_Path(path, file, "");

	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0)
		return PLAYLIST_INVALID;

	SceOff size = sceIoLseek(fd, 0, SCE_SEEK_END);

	if (size >= 0 && size <= STORE_MAX_FILE_BYTES && sceIoLseek(fd, 0, SCE_SEEK_SET) == 0) {
		buf = malloc(size > 0 ? (size_t)size : 1);

		if (buf == NULL) {
			result = PLAYLIST_NOMEM;
		} else {
			SceOff done = 0;
			int got = 1;

			while (done < size && (got = sceIoRead(fd, buf + done, (SceSize)(size - done))) > 0)
				done += got;

			if (done == size)
				result = Playlist_Parse(buf, (size_t)size, out);
		}
	}

	sceIoClose(fd);
	free(buf);

	// A file someone wrote by hand without #PLAYLIST goes by its file name.
	if (result == PLAYLIST_OK && out->name[0] == '\0') {
		size_t len = strlen(file);

		if (Store_EndsWith(file, PLAYLIST_FILE_EXT))
			len -= strlen(PLAYLIST_FILE_EXT);

		snprintf(out->name, sizeof(out->name), "%.*s", (int)len, file);
	}

	return result;
}

static int Store_WriteAll(const char *path, const char *text, size_t len) {
	SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return fd;

	size_t done = 0;
	int ret = 0;

	while (done < len) {
		int wrote = sceIoWrite(fd, text + done, (SceSize)(len - done));

		if (wrote <= 0) {
			ret = wrote < 0 ? wrote : -1;
			break;
		}

		done += (size_t)wrote;
	}

	int closed = sceIoClose(fd);
	return ret < 0 ? ret : (closed < 0 ? closed : 0);
}

static int Store_Save(const char *file, const Playlist *pl) {
	char *text = NULL;
	size_t len = 0;

	if (Playlist_Serialize(pl, &text, &len) != PLAYLIST_OK)
		return -1;

	char main_path[STORE_PATH_MAX], tmp_path[STORE_PATH_MAX], bak_path[STORE_PATH_MAX];
	Store_Path(main_path, file, "");
	Store_Path(tmp_path, file, PLAYLIST_TMP_EXT);
	Store_Path(bak_path, file, PLAYLIST_BAK_EXT);

	sceIoMkdir(PLAYLIST_DIR, 0777);

	// 1. The new content, whole, beside the old one.
	int ret = Store_WriteAll(tmp_path, text, len);
	free(text);

	if (ret < 0) {
		sceIoRemove(tmp_path);
		return ret;
	}

	// 2. The old one out of the way, since a rename does not replace.
	SceBool had_main = Store_Exists(file, "");

	if (had_main) {
		sceIoRemove(bak_path);

		if ((ret = sceIoRename(main_path, bak_path)) < 0) {
			sceIoRemove(tmp_path);
			return ret;
		}
	}

	// 3. The new one in place.
	if ((ret = sceIoRename(tmp_path, main_path)) < 0) {
		if (had_main)
			sceIoRename(bak_path, main_path);

		sceIoRemove(tmp_path);
		return ret;
	}

	// 4. The old one gone.
	if (had_main)
		sceIoRemove(bak_path);

	return 0;
}

int PlaylistStore_Save(const char *file, const Playlist *pl) {
	int ret = Store_Save(file, pl);

	if (DebugLog_On()) {
		LogLine line;
		DebugLog_Begin(&line, "playlist");
		LogLine_Str(&line, "name", pl->name);
		LogLine_Int(&line, "entries", pl->count);
		LogLine_Str(&line, "result", ret >= 0 ? "ok" : "fail");
		DebugLog_Commit(&line);
	}

	return ret;
}

// Whether a file name is taken: by a playlist, or by the .bak that listing
// would turn back into one.
static int Store_Taken(const char *file, void *ctx) {
	(void)ctx;
	return Store_Exists(file, "") || Store_Exists(file, PLAYLIST_BAK_EXT);
}

// The same, except for the playlist's own file `ctx`, which it may keep.
static int Store_TakenByOther(const char *file, void *ctx) {
	return strcasecmp(file, (const char *)ctx) != 0 && Store_Taken(file, NULL);
}

int PlaylistStore_Create(const Playlist *pl, char *file_out) {
	if (Playlist_FileName(pl->name, Store_Taken, NULL, file_out, PLAYLIST_FILE_MAX) < 0)
		return -1;

	return PlaylistStore_Save(file_out, pl);
}

int PlaylistStore_Rename(const char *file, const char *new_name, char *file_out) {
	Playlist pl;

	snprintf(file_out, PLAYLIST_FILE_MAX, "%s", file);

	if (PlaylistStore_Load(file, &pl) != PLAYLIST_OK)
		return -1;

	snprintf(pl.name, sizeof(pl.name), "%s", new_name);
	int ret = PlaylistStore_Save(file, &pl);
	Playlist_Free(&pl);

	if (ret < 0)
		return ret;

	// The playlist already has its new name. The file name follows it when it
	// can; when it cannot, the old file name is harmless.
	char wanted[PLAYLIST_FILE_MAX];

	if (Playlist_FileName(new_name, Store_TakenByOther, (void *)file, wanted, sizeof(wanted)) < 0 || strcasecmp(wanted, file) == 0)
		return 0;

	char old_path[STORE_PATH_MAX], new_path[STORE_PATH_MAX];
	Store_Path(old_path, file, "");
	Store_Path(new_path, wanted, "");

	if (sceIoRename(old_path, new_path) >= 0)
		snprintf(file_out, PLAYLIST_FILE_MAX, "%s", wanted);

	return 0;
}

int PlaylistStore_Delete(const char *file) {
	char path[STORE_PATH_MAX];

	Store_Path(path, file, PLAYLIST_TMP_EXT);
	sceIoRemove(path);
	Store_Path(path, file, PLAYLIST_BAK_EXT);
	sceIoRemove(path);
	Store_Path(path, file, "");
	return sceIoRemove(path);
}

// ---------------------------------------------------------------------------
// listing

typedef struct {
	char file[PLAYLIST_FILE_MAX];
	int has;
} Store_Group;

static int Store_Probe(const char *album, const char *path, void *ctx) {
	(void)ctx;
	return (int)Cover_Probe(album, path);
}

static void Store_Summarize(const char *file, const Playlist *pl, SceUInt64 mtime, PlaylistSummary *s) {
	memset(s, 0, sizeof(*s));
	snprintf(s->file, sizeof(s->file), "%s", file);
	snprintf(s->name, sizeof(s->name), "%s", pl->name);
	s->count = pl->count;
	s->mtime = mtime;

	int picks[4];
	int pending = 0;
	s->mosaic_count = Playlist_MosaicAlbums(pl, Store_Probe, NULL, picks, &pending);
	s->mosaic_pending = pending ? SCE_TRUE : SCE_FALSE;

	for (int i = 0; i < s->mosaic_count; i++) {
		snprintf(s->mosaic_album[i], PLAYLIST_TAG_MAX, "%s", pl->entries[picks[i]].album);
		snprintf(s->mosaic_path[i], PLAYLIST_PATH_MAX, "%s", pl->entries[picks[i]].path);
	}
}

static int Store_CompareNewest(const void *a, const void *b) {
	const PlaylistSummary *sa = a;
	const PlaylistSummary *sb = b;

	if (sa->mtime != sb->mtime)
		return sa->mtime > sb->mtime ? -1 : 1;

	return strcmp(sa->file, sb->file);
}

// Gathers the files of the folder by playlist: X.m3u8, X.m3u8.tmp and
// X.m3u8.bak go to the group of X.m3u8. Returns how many groups, or -1 when
// memory runs out.
static int Store_ReadGroups(Store_Group **out) {
	*out = NULL;

	SceUID dir = sceIoDopen(PLAYLIST_DIR);
	if (dir < 0)
		return 0;

	Store_Group *groups = NULL;
	int count = 0, cap = 0, failed = 0;
	SceIoDirent entry;

	memset(&entry, 0, sizeof(entry));

	while (!failed && sceIoDread(dir, &entry) > 0) {
		char base[PLAYLIST_FILE_MAX];
		size_t len = strlen(entry.d_name);
		int has;

		if (SCE_S_ISDIR(entry.d_stat.st_mode))
			continue;

		if (Store_EndsWith(entry.d_name, PLAYLIST_FILE_EXT PLAYLIST_TMP_EXT)) {
			has = STORE_HAS_TMP;
			len -= strlen(PLAYLIST_TMP_EXT);
		} else if (Store_EndsWith(entry.d_name, PLAYLIST_FILE_EXT PLAYLIST_BAK_EXT)) {
			has = STORE_HAS_BAK;
			len -= strlen(PLAYLIST_BAK_EXT);
		} else if (Store_EndsWith(entry.d_name, PLAYLIST_FILE_EXT)) {
			has = STORE_HAS_MAIN;
		} else {
			continue;
		}

		// Longer than any name this module makes: not one of ours.
		if (len >= sizeof(base))
			continue;

		snprintf(base, sizeof(base), "%.*s", (int)len, entry.d_name);

		int g = 0;
		while (g < count && strcasecmp(groups[g].file, base) != 0)
			g++;

		if (g == count) {
			if (count == cap) {
				int grown_cap = cap > 0 ? cap * 2 : 16;
				Store_Group *grown = realloc(groups, (size_t)grown_cap * sizeof(Store_Group));

				if (grown == NULL) {
					failed = 1;
					continue;
				}

				groups = grown;
				cap = grown_cap;
			}

			snprintf(groups[g].file, sizeof(groups[g].file), "%s", base);
			groups[g].has = 0;
			count++;
		}

		groups[g].has |= has;
	}

	sceIoDclose(dir);

	if (failed) {
		free(groups);
		return -1;
	}

	*out = groups;
	return count;
}

static void Store_Recover(const Store_Group *g) {
	int actions = Playlist_Recover(g->has & STORE_HAS_MAIN, g->has & STORE_HAS_TMP, g->has & STORE_HAS_BAK);
	char main_path[STORE_PATH_MAX], other[STORE_PATH_MAX];

	Store_Path(main_path, g->file, "");

	if (actions & PLAYLIST_RECOVER_RESTORE_BAK) {
		Store_Path(other, g->file, PLAYLIST_BAK_EXT);
		sceIoRename(other, main_path);
	}

	if (actions & PLAYLIST_RECOVER_DELETE_TMP) {
		Store_Path(other, g->file, PLAYLIST_TMP_EXT);
		sceIoRemove(other);
	}

	if (actions & PLAYLIST_RECOVER_DELETE_BAK) {
		Store_Path(other, g->file, PLAYLIST_BAK_EXT);
		sceIoRemove(other);
	}
}

int PlaylistStore_List(PlaylistSummary **out) {
	Store_Group *groups = NULL;
	int n_groups = Store_ReadGroups(&groups);

	*out = NULL;
	if (n_groups <= 0)
		return n_groups;

	PlaylistSummary *summaries = malloc((size_t)n_groups * sizeof(PlaylistSummary));
	int count = 0;

	if (summaries == NULL) {
		free(groups);
		return -1;
	}

	for (int g = 0; g < n_groups; g++) {
		char path[STORE_PATH_MAX];
		SceIoStat stat;
		Playlist pl;

		Store_Recover(&groups[g]);

		// One file at a time: its entries go as soon as its summary is out.
		// A file that cannot be read, or is not a playlist, is left out.
		Store_Path(path, groups[g].file, "");
		if (sceIoGetstat(path, &stat) < 0 || PlaylistStore_Load(groups[g].file, &pl) != PLAYLIST_OK)
			continue;

		SceUInt64 mtime = 0;
		sceRtcGetTime64_t(&stat.st_mtime, &mtime);
		Store_Summarize(groups[g].file, &pl, mtime, &summaries[count++]);
		Playlist_Free(&pl);
	}

	free(groups);

	if (count == 0) {
		free(summaries);
		return 0;
	}

	qsort(summaries, (size_t)count, sizeof(PlaylistSummary), Store_CompareNewest);
	*out = summaries;
	return count;
}
