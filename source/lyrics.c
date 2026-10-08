#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "lyrics.h"

// More minutes than this is not a timestamp, and keeps the milliseconds of
// the largest one well inside 32 bits.
#define LYRICS_MAX_MINUTES 9999

// [offset:N] beyond this many milliseconds (about 11 days) is not a number
// anyone meant; it is ignored like any other value that is not a number.
#define LYRICS_MAX_OFFSET 999999999L

static const char *const lyrics_tags[] = { "ti", "ar", "al", "au", "by", "length", "offset", "re", "ve" };

typedef struct {
	const char *s;
	size_t len;
	size_t pos;
} Lyrics_Cursor;

static int Lyrics_IsDigit(char c) {
	return c >= '0' && c <= '9';
}

static int Lyrics_IsBlank(char c) {
	return c == ' ' || c == '\t';
}

// Next line, without its terminator: \n, \r\n, \r and NUL all end a line. A
// terminator at the very end does not start one more, empty line.
static int Lyrics_NextLine(Lyrics_Cursor *c, const char **start, const char **end) {
	if (c->pos >= c->len)
		return 0;

	size_t i = c->pos;

	while (i < c->len && c->s[i] != '\n' && c->s[i] != '\r' && c->s[i] != '\0')
		i++;

	*start = c->s + c->pos;
	*end = c->s + i;

	if (i < c->len && c->s[i] == '\r' && i + 1 < c->len && c->s[i + 1] == '\n')
		i++;

	c->pos = i + 1;

	while (*end > *start && Lyrics_IsBlank((*end)[-1]))
		(*end)--;

	return 1;
}

// A timestamp at `p`, between `open` and `close`: minutes (one or more
// digits), ':', seconds below 60 (one or two digits), and optionally '.' and
// at least one digit, of which only the first three count. Returns how many
// bytes it takes, or 0 when it is not one.
static size_t Lyrics_ParseStamp(const char *p, const char *end, char open, char close, uint32_t *ms) {
	const char *q = p;

	if (q >= end || *q != open)
		return 0;
	q++;

	uint32_t min = 0;
	const char *digits = q;

	while (q < end && Lyrics_IsDigit(*q)) {
		min = min * 10 + (*q - '0');
		if (min > LYRICS_MAX_MINUTES)
			return 0;
		q++;
	}

	if (q == digits || q >= end || *q != ':')
		return 0;
	q++;

	uint32_t sec = 0;
	digits = q;

	while (q < end && Lyrics_IsDigit(*q) && q - digits < 2)
		sec = sec * 10 + (*q++ - '0');

	if (q == digits || sec >= 60)
		return 0;

	uint32_t frac = 0;

	if (q < end && *q == '.') {
		q++;
		digits = q;

		for (uint32_t scale = 100; q < end && Lyrics_IsDigit(*q); q++) {
			frac += (*q - '0') * scale;
			scale /= 10;
		}

		if (q == digits)
			return 0;
	}

	if (q >= end || *q != close)
		return 0;

	*ms = (min * 60 + sec) * 1000 + frac;
	return q + 1 - p;
}

// [offset:N] with an optional sign. Returns 1 and stores N when it is a number.
static int Lyrics_ParseOffset(const char *p, const char *end, long *offset) {
	int sign = 1;

	if (p < end && (*p == '+' || *p == '-')) {
		sign = (*p == '-') ? -1 : 1;
		p++;
	}

	if (p >= end)
		return 0;

	long n = 0;

	for (; p < end; p++) {
		if (!Lyrics_IsDigit(*p))
			return 0;

		n = n * 10 + (*p - '0');
		if (n > LYRICS_MAX_OFFSET)
			return 0;
	}

	*offset = sign * n;
	return 1;
}

// A line that is exactly one known tag, [key:value]. For [offset:N] with a
// number, also stores it.
static int Lyrics_IsTagLine(const char *p, const char *end, long *offset) {
	while (p < end && Lyrics_IsBlank(*p))
		p++;

	if (end - p < 3 || *p != '[' || end[-1] != ']')
		return 0;

	const char *key = p + 1;
	const char *colon = memchr(key, ':', end - key);

	if (colon == NULL)
		return 0;

	const char *value = colon + 1;
	const char *close = memchr(value, ']', end - value);

	if (close != end - 1)
		return 0;

	size_t key_len = colon - key;

	for (unsigned int i = 0; i < sizeof(lyrics_tags) / sizeof(lyrics_tags[0]); i++) {
		if (strlen(lyrics_tags[i]) == key_len && strncasecmp(key, lyrics_tags[i], key_len) == 0) {
			if (i == 6) {
				long n;
				if (Lyrics_ParseOffset(value, close, &n))
					*offset = n;
			}

			return 1;
		}
	}

	return 0;
}

// Timestamps at the start of the line, one after the other.
static uint32_t Lyrics_CountStamps(const char *p, const char *end, const char **text) {
	uint32_t count = 0, ms;
	size_t n;

	while (p < end && Lyrics_IsBlank(*p))
		p++;

	while ((n = Lyrics_ParseStamp(p, end, '[', ']', &ms)) > 0) {
		p += n;
		count++;
	}

	*text = p;
	return count;
}

static uint32_t Lyrics_ApplyOffset(uint32_t ms, long offset) {
	if (offset >= 0)
		return (ms > (uint32_t)offset) ? ms - (uint32_t)offset : 0;

	uint32_t later = (uint32_t)(-offset);
	return (ms > UINT32_MAX - later) ? UINT32_MAX : ms + later;
}

static int Lyrics_CompareLines(const void *a, const void *b) {
	const Lyrics_Line *la = a, *lb = b;

	if (la->ms != lb->ms)
		return (la->ms < lb->ms) ? -1 : 1;

	// Same time: file order. The text offset grows with it, and two entries
	// with the same offset are the same line, so qsort gives a stable result.
	if (la->off != lb->off)
		return (la->off < lb->off) ? -1 : 1;

	return 0;
}

// Copies a line's text to `dst` without word marks and trailing blanks.
static uint32_t Lyrics_CopyText(char *dst, const char *p, const char *end) {
	uint32_t n = 0, ms;

	while (p < end) {
		size_t mark = Lyrics_ParseStamp(p, end, '<', '>', &ms);

		if (mark > 0) {
			p += mark;
			continue;
		}

		dst[n++] = *p++;
	}

	while (n > 0 && Lyrics_IsBlank(dst[n - 1]))
		n--;

	dst[n] = '\0';
	return n;
}

int Lyrics_Parse(const char *utf8, size_t len, Lyrics *out) {
	memset(out, 0, sizeof(*out));

	if (utf8 == NULL || len == 0 || len > LYRICS_MAX_BYTES)
		return 0;

	// First pass: how many lines and timestamps there are, and the offset.
	Lyrics_Cursor c = { utf8, len, 0 };
	const char *start, *end, *text;
	uint32_t lines = 0, stamps = 0;
	long offset = 0;

	while (Lyrics_NextLine(&c, &start, &end)) {
		if (Lyrics_IsTagLine(start, end, &offset))
			continue;

		lines++;
		stamps += Lyrics_CountStamps(start, end, &text);
	}

	int synced = (stamps > 0);
	uint32_t entries = synced ? stamps : lines;

	if (entries == 0)
		return 0;

	// Each line's text is at most its raw length, plus a NUL in place of its
	// terminator: the whole buffer fits in len + 1.
	out->text = malloc(len + 1);
	out->lines = malloc(entries * sizeof(Lyrics_Line));

	if (out->text == NULL || out->lines == NULL) {
		Lyrics_Free(out);
		return -1;
	}

	// Second pass: copy the text and fill the lines.
	c.pos = 0;
	uint32_t used = 0, count = 0;
	int any_text = 0;

	while (Lyrics_NextLine(&c, &start, &end)) {
		long ignored;

		if (Lyrics_IsTagLine(start, end, &ignored))
			continue;

		uint32_t line_stamps = Lyrics_CountStamps(start, end, &text);

		if (synced && line_stamps == 0)
			continue;

		if (synced) {
			while (text < end && Lyrics_IsBlank(*text))
				text++;
		}
		else
			text = start;

		uint32_t off = used;
		uint32_t n = Lyrics_CopyText(out->text + off, text, end);
		used += n + 1;

		uint8_t flags = (n >= 2 && out->text[off] == '[' && out->text[off + n - 1] == ']') ? LYRICS_LINE_HEADER : 0;

		for (uint32_t s = 0; s < line_stamps || (!synced && s == 0); s++)
			out->lines[count++] = (Lyrics_Line){ 0, off, n, flags };

		if (synced) {
			const char *p = start;

			while (p < end && Lyrics_IsBlank(*p))
				p++;
			uint32_t ms;
			size_t step;

			for (uint32_t s = count - line_stamps; (step = Lyrics_ParseStamp(p, end, '[', ']', &ms)) > 0; s++) {
				out->lines[s].ms = Lyrics_ApplyOffset(ms, offset);
				p += step;
			}
		}

		for (uint32_t i = 0; i < n; i++) {
			if (!Lyrics_IsBlank(out->text[off + i])) {
				any_text = 1;
				break;
			}
		}
	}

	if (!any_text) {
		Lyrics_Free(out);
		return 0;
	}

	if (synced)
		qsort(out->lines, count, sizeof(Lyrics_Line), Lyrics_CompareLines);

	out->count = count;
	out->synced = synced;
	return 0;
}

int Lyrics_LineAt(const Lyrics *lyrics, uint32_t ms) {
	if (!lyrics->synced || lyrics->count == 0 || lyrics->lines[0].ms > ms)
		return -1;

	// Last line with time <= ms: lines[lo] always qualifies.
	uint32_t lo = 0, hi = lyrics->count - 1;

	while (lo < hi) {
		uint32_t mid = lo + (hi - lo + 1) / 2;

		if (lyrics->lines[mid].ms <= ms)
			lo = mid;
		else
			hi = mid - 1;
	}

	return (int)lo;
}

void Lyrics_Choose(Lyrics *from_lrc, Lyrics *embedded, Lyrics *out) {
	int use_lrc;

	if (from_lrc->count == 0)
		use_lrc = 0;
	else if (embedded->count == 0)
		use_lrc = 1;
	else
		use_lrc = from_lrc->synced || !embedded->synced;

	if (use_lrc) {
		*out = *from_lrc;
		Lyrics_Free(embedded);
	}
	else {
		*out = *embedded;
		Lyrics_Free(from_lrc);
	}

	memset(from_lrc, 0, sizeof(*from_lrc));
	memset(embedded, 0, sizeof(*embedded));
}

void Lyrics_Free(Lyrics *lyrics) {
	free(lyrics->text);
	free(lyrics->lines);
	memset(lyrics, 0, sizeof(*lyrics));
}
