#include <stdlib.h>
#include <string.h>

#include "playlist.h"

// The tests swap realloc for one that fails on demand, to walk the paths where
// memory runs out (tests/test_playlist.c). Every allocation here goes through it.
#ifdef PLAYLIST_TEST_ALLOC
void *Playlist_TestRealloc(void *ptr, size_t size);
#define playlist_realloc Playlist_TestRealloc
#else
#define playlist_realloc realloc
#endif

#define TAG_EXTM3U   "#EXTM3U"
#define TAG_PLAYLIST "#PLAYLIST:"
#define TAG_EXTINF   "#EXTINF:"
#define TAG_EXTALB   "#EXTALB:"

// Between artist and title in #EXTINF, the usual "Artist - Title".
#define ARTIST_SEP " - "

typedef struct {
	const char *start;
	size_t len;
} Line;

static int StartsWith(const Line *line, const char *prefix) {
	size_t n = strlen(prefix);
	return line->len >= n && memcmp(line->start, prefix, n) == 0;
}

static int IsBlank(const Line *line) {
	for (size_t i = 0; i < line->len; i++) {
		if (line->start[i] != ' ' && line->start[i] != '\t')
			return 0;
	}

	return 1;
}

// Takes the next line from *p, without its \n or \r\n. 0 at the end of the text.
static int NextLine(const char **p, const char *end, Line *line) {
	if (*p >= end)
		return 0;

	const char *nl = memchr(*p, '\n', (size_t)(end - *p));
	const char *stop = nl != NULL ? nl : end;

	line->start = *p;
	line->len = (size_t)(stop - *p);
	if (line->len > 0 && line->start[line->len - 1] == '\r')
		line->len--;

	*p = nl != NULL ? nl + 1 : end;
	return 1;
}

// Copies `n` bytes of `src` into `dst` of `cap` bytes, NUL-terminated. What does
// not fit is dropped at a character boundary, so a cut never leaves half a
// UTF-8 sequence.
static void CopyField(char *dst, size_t cap, const char *src, size_t n) {
	if (n >= cap) {
		n = cap - 1;
		while (n > 0 && ((unsigned char)src[n] & 0xC0) == 0x80)
			n--;
	}

	memcpy(dst, src, n);
	dst[n] = '\0';
}

// Whether the line is a path, as opposed to a blank or a # line.
static int IsPathLine(const Line *line) {
	return !IsBlank(line) && line->start[0] != '#';
}

static void ParseExtinf(const Line *line, Playlist_Entry *e) {
	const char *p = line->start + strlen(TAG_EXTINF);
	const char *end = line->start + line->len;
	const char *comma = memchr(p, ',', (size_t)(end - p));

	e->artist[0] = '\0';
	e->title[0] = '\0';
	if (comma == NULL)
		return;

	const char *text = comma + 1;
	size_t text_len = (size_t)(end - text);
	size_t sep_len = strlen(ARTIST_SEP);

	for (size_t i = 0; i + sep_len <= text_len; i++) {
		if (memcmp(text + i, ARTIST_SEP, sep_len) == 0) {
			CopyField(e->artist, sizeof(e->artist), text, i);
			CopyField(e->title, sizeof(e->title), text + i + sep_len, text_len - i - sep_len);
			return;
		}
	}

	CopyField(e->title, sizeof(e->title), text, text_len);
}

Playlist_Result Playlist_Parse(const char *buf, size_t len, Playlist *out) {
	memset(out, 0, sizeof(*out));

	const char *p = buf;
	const char *end = buf + len;
	Line line;

	if (len >= 3 && memcmp(buf, "\xEF\xBB\xBF", 3) == 0)
		p += 3;

	// The header must be the first line that is not blank.
	int header = 0;
	while (NextLine(&p, end, &line)) {
		if (IsBlank(&line))
			continue;

		header = StartsWith(&line, TAG_EXTM3U);
		break;
	}

	if (!header)
		return PLAYLIST_INVALID;

	// First pass: how many entries, so the array is reserved once and exactly.
	const char *body = p;
	int entries = 0;
	int named = 0;

	while (NextLine(&p, end, &line)) {
		if (StartsWith(&line, TAG_PLAYLIST))
			named = 1;
		else if (IsPathLine(&line) && line.len < PLAYLIST_PATH_MAX && entries < PLAYLIST_MAX_ENTRIES)
			entries++;
	}

	if (!named && entries == 0)
		return PLAYLIST_INVALID;

	if (entries > 0) {
		out->entries = playlist_realloc(NULL, (size_t)entries * sizeof(Playlist_Entry));
		if (out->entries == NULL)
			return PLAYLIST_NOMEM;

		out->cap = entries;
	}

	// Second pass: #EXTINF and #EXTALB belong to the path that follows them.
	Playlist_Entry pending;
	memset(&pending, 0, sizeof(pending));
	p = body;

	while (NextLine(&p, end, &line)) {
		if (StartsWith(&line, TAG_PLAYLIST)) {
			size_t n = strlen(TAG_PLAYLIST);
			CopyField(out->name, sizeof(out->name), line.start + n, line.len - n);
		} else if (StartsWith(&line, TAG_EXTINF)) {
			ParseExtinf(&line, &pending);
		} else if (StartsWith(&line, TAG_EXTALB)) {
			size_t n = strlen(TAG_EXTALB);
			CopyField(pending.album, sizeof(pending.album), line.start + n, line.len - n);
		} else if (IsPathLine(&line)) {
			if (line.len < PLAYLIST_PATH_MAX && out->count < out->cap) {
				Playlist_Entry *e = &out->entries[out->count++];

				*e = pending;
				CopyField(e->path, sizeof(e->path), line.start, line.len);
			}

			memset(&pending, 0, sizeof(pending));
		}
	}

	return PLAYLIST_OK;
}

// Appends `s` at *w, with line breaks as spaces.
static void Put(char **w, const char *s) {
	for (; *s != '\0'; s++)
		*(*w)++ = (*s == '\n' || *s == '\r') ? ' ' : *s;
}

// Whether the title would be cut in two when read back without an artist.
static int TitleHasSep(const Playlist_Entry *e) {
	return strstr(e->title, ARTIST_SEP) != NULL;
}

Playlist_Result Playlist_Serialize(const Playlist *pl, char **out, size_t *len) {
	size_t size = strlen(TAG_EXTM3U "\n") + strlen(TAG_PLAYLIST) + strlen(pl->name) + 1;

	for (int i = 0; i < pl->count; i++) {
		const Playlist_Entry *e = &pl->entries[i];

		size += strlen(TAG_EXTINF "-1,") + strlen(e->title) + 1;
		// Without an artist the separator still goes first when the title has
		// one, so reading it back gives an empty artist and the whole title.
		if (e->artist[0] != '\0' || TitleHasSep(e))
			size += strlen(e->artist) + strlen(ARTIST_SEP);
		if (e->album[0] != '\0')
			size += strlen(TAG_EXTALB) + strlen(e->album) + 1;
		size += strlen(e->path) + 1;
	}

	char *text = playlist_realloc(NULL, size + 1);
	if (text == NULL)
		return PLAYLIST_NOMEM;

	char *w = text;

	Put(&w, TAG_EXTM3U);
	*w++ = '\n';
	Put(&w, TAG_PLAYLIST);
	Put(&w, pl->name);
	*w++ = '\n';

	for (int i = 0; i < pl->count; i++) {
		const Playlist_Entry *e = &pl->entries[i];

		Put(&w, TAG_EXTINF "-1,");
		if (e->artist[0] != '\0' || TitleHasSep(e)) {
			Put(&w, e->artist);
			Put(&w, ARTIST_SEP);
		}
		Put(&w, e->title);
		*w++ = '\n';

		if (e->album[0] != '\0') {
			Put(&w, TAG_EXTALB);
			Put(&w, e->album);
			*w++ = '\n';
		}

		Put(&w, e->path);
		*w++ = '\n';
	}

	*w = '\0';
	*out = text;
	*len = size;
	return PLAYLIST_OK;
}

static int HasPath(const Playlist *pl, const char *path) {
	for (int i = 0; i < pl->count; i++) {
		if (strcmp(pl->entries[i].path, path) == 0)
			return 1;
	}

	return 0;
}

Playlist_Result Playlist_Append(Playlist *pl, const Playlist_Entry *src, int n, int *added, int *skipped, int *full) {
	int n_added = 0, n_skipped = 0, n_full = 0;

	// Room for all of `src` up front, so running out of memory happens before
	// anything changes. A realloc that fails leaves the old block alone.
	int want = pl->count + (n > 0 ? n : 0);
	if (want > PLAYLIST_MAX_ENTRIES)
		want = PLAYLIST_MAX_ENTRIES;

	if (want > pl->cap) {
		Playlist_Entry *grown = playlist_realloc(pl->entries, (size_t)want * sizeof(Playlist_Entry));
		if (grown == NULL)
			return PLAYLIST_NOMEM;

		pl->entries = grown;
		pl->cap = want;
	}

	for (int i = 0; i < n; i++) {
		if (HasPath(pl, src[i].path)) {
			n_skipped++;
		} else if (pl->count >= PLAYLIST_MAX_ENTRIES) {
			// Keeps counting the ones that were already there, so the notice
			// says the same whether the playlist filled up or not.
			n_full = 1;
		} else {
			pl->entries[pl->count++] = src[i];
			n_added++;
		}
	}

	if (added != NULL)
		*added = n_added;
	if (skipped != NULL)
		*skipped = n_skipped;
	if (full != NULL)
		*full = n_full;

	return PLAYLIST_OK;
}

int Playlist_Remove(Playlist *pl, int index) {
	if (index < 0 || index >= pl->count)
		return -1;

	memmove(&pl->entries[index], &pl->entries[index + 1], (size_t)(pl->count - index - 1) * sizeof(Playlist_Entry));
	pl->count--;
	return 0;
}

int Playlist_Move(Playlist *pl, int from, int to) {
	if (from < 0 || from >= pl->count || to < 0 || to >= pl->count)
		return -1;

	if (from == to)
		return 0;

	Playlist_Entry moving = pl->entries[from];

	if (from < to)
		memmove(&pl->entries[from], &pl->entries[from + 1], (size_t)(to - from) * sizeof(Playlist_Entry));
	else
		memmove(&pl->entries[to + 1], &pl->entries[to], (size_t)(from - to) * sizeof(Playlist_Entry));

	pl->entries[to] = moving;
	return 0;
}

void Playlist_Free(Playlist *pl) {
	free(pl->entries);
	memset(pl, 0, sizeof(*pl));
}
