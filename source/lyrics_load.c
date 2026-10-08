#include <psp2/io/fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "lyrics_load.h"
#include "text_encoding.h"

// "dir/name.flac" -> "dir/name.lrc"; a name without an extension gets one.
static char *LyricsLoad_LrcPath(const char *path) {
	const char *slash = strrchr(path, '/');
	const char *dot = strrchr(path, '.');
	size_t stem = (dot != NULL && (slash == NULL || dot > slash)) ? (size_t)(dot - path) : strlen(path);
	char *lrc = malloc(stem + sizeof(".lrc"));

	if (lrc == NULL)
		return NULL;

	memcpy(lrc, path, stem);
	memcpy(lrc + stem, ".lrc", sizeof(".lrc"));
	return lrc;
}

// The whole file, or NULL when it is missing, empty, larger than
// LYRICS_MAX_BYTES or cannot be read. The size is checked before reading.
static unsigned char *LyricsLoad_ReadFile(const char *path, size_t *len) {
	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);

	if (fd < 0)
		return NULL;

	SceOff size = sceIoLseek(fd, 0, SCE_SEEK_END);
	unsigned char *buf = NULL;

	if (size > 0 && size <= LYRICS_MAX_BYTES && sceIoLseek(fd, 0, SCE_SEEK_SET) == 0)
		buf = malloc(size);

	size_t got = 0;

	while (buf != NULL && got < (size_t)size) {
		int n = sceIoRead(fd, buf + got, size - got);

		if (n <= 0) {
			free(buf);
			buf = NULL;
			break;
		}

		got += n;
	}

	sceIoClose(fd);
	*len = got;
	return buf;
}

static void LyricsLoad_FromLrc(const char *path, Lyrics *out) {
	char *lrc_path = LyricsLoad_LrcPath(path);

	if (lrc_path == NULL)
		return;

	size_t raw_len = 0;
	unsigned char *raw = LyricsLoad_ReadFile(lrc_path, &raw_len);
	free(lrc_path);

	if (raw == NULL)
		return;

	char *utf8 = NULL;
	size_t utf8_len = 0;
	int converted = TextEncoding_ToUtf8(raw, raw_len, &utf8, &utf8_len);
	free(raw);

	if (converted == 0)
		Lyrics_Parse(utf8, utf8_len, out);

	free(utf8);
}

static void LyricsLoad_FromEmbedded(char *embedded, size_t embedded_len, Lyrics *out) {
	if (embedded == NULL)
		return;

	// The formats promise UTF-8, but the file is not to be trusted: anything
	// invalid becomes U+FFFD before it reaches the parser.
	char *utf8 = NULL;
	size_t utf8_len = 0;

	if (TextEncoding_SanitizeUtf8((const unsigned char *)embedded, embedded_len, &utf8, &utf8_len) == 0)
		Lyrics_Parse(utf8, utf8_len, out);

	free(utf8);
}

void LyricsLoad_ForTrack(const char *path, char *embedded, size_t embedded_len, Lyrics *out) {
	Lyrics from_lrc = { 0 }, from_tag = { 0 };

	LyricsLoad_FromLrc(path, &from_lrc);
	LyricsLoad_FromEmbedded(embedded, embedded_len, &from_tag);
	free(embedded);

	Lyrics_Choose(&from_lrc, &from_tag, out);
}
