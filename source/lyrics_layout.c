#include <stdlib.h>
#include <string.h>

#include "lyrics_layout.h"
#include "text_encoding.h"

typedef struct {
	const char *s;
	uint32_t base; // offset of s in Lyrics.text
	uint32_t line;
	float width;
	LyricsLayout_Measure measure;
	void *ctx;
	LyricsLayout *out;
	uint32_t capacity;
} LyricsLayout_Builder;

// Bytes of the code point at s[i], 1 for a stray byte so the walk always moves.
static size_t LyricsLayout_Step(const char *s, size_t i, size_t n) {
	size_t step = TextEncoding_Utf8SeqLen((const unsigned char *)s + i, n - i);
	return step ? step : 1;
}

static uint32_t LyricsLayout_CodePoint(const char *s, size_t i, size_t step) {
	const unsigned char *u = (const unsigned char *)s + i;

	switch (step) {
		case 2: return (u[0] & 0x1F) << 6 | (u[1] & 0x3F);
		case 3: return (u[0] & 0x0F) << 12 | (u[1] & 0x3F) << 6 | (u[2] & 0x3F);
		case 4: return (u[0] & 0x07) << 18 | (u[1] & 0x3F) << 12 | (u[2] & 0x3F) << 6 | (u[3] & 0x3F);
		default: return u[0];
	}
}

static int LyricsLayout_IsCjk(uint32_t cp) {
	return (cp >= 0x1100 && cp <= 0x11FF)    // Hangul Jamo
		|| (cp >= 0x3000 && cp <= 0x30FF)    // CJK punctuation, hiragana, katakana
		|| (cp >= 0x3130 && cp <= 0x318F)    // Hangul compatibility Jamo
		|| (cp >= 0x3400 && cp <= 0x4DBF)    // CJK extension A
		|| (cp >= 0x4E00 && cp <= 0x9FFF)    // CJK unified ideographs
		|| (cp >= 0xAC00 && cp <= 0xD7A3)    // Hangul syllables
		|| (cp >= 0xF900 && cp <= 0xFAFF)    // CJK compatibility ideographs
		|| (cp >= 0xFF00 && cp <= 0xFFEF);   // full and half width forms
}

static int LyricsLayout_Push(LyricsLayout_Builder *b, size_t start, size_t end) {
	LyricsLayout *out = b->out;

	if (out->count == b->capacity) {
		uint32_t capacity = b->capacity ? b->capacity * 2 : 64;
		LyricsLayout_Row *rows = realloc(out->rows, capacity * sizeof(LyricsLayout_Row));

		if (rows == NULL)
			return -1;

		out->rows = rows;
		b->capacity = capacity;
	}

	out->rows[out->count++] = (LyricsLayout_Row){ b->line, b->base + (uint32_t)start, (uint32_t)(end - start) };
	return 0;
}

static int LyricsLayout_Fits(LyricsLayout_Builder *b, size_t start, size_t end) {
	if (end - start > LYRICS_LAYOUT_MAX_ROW_BYTES)
		return 0;

	return b->measure(b->s + start, end - start, b->line, b->ctx) <= b->width;
}

// Lays out s[0..n) into rows.
static int LyricsLayout_Line(LyricsLayout_Builder *b, size_t n) {
	const char *s = b->s;

	if (n == 0)
		return LyricsLayout_Push(b, 0, 0);

	size_t start = 0;

	while (start < n) {
		// Walk the break opportunities after `start`, keeping the last one
		// whose row still fits: before a space, between two CJK characters,
		// and the end of the line.
		size_t fit_end = 0, fit_next = 0;
		uint32_t prev_cp = 0;
		size_t i = start;

		while (i < n) {
			size_t step = LyricsLayout_Step(s, i, n);
			uint32_t cp = LyricsLayout_CodePoint(s, i, step);
			size_t end = 0, next = 0;

			if (cp == ' ' && i > start && s[i - 1] != ' ') {
				end = i;
				next = i;
				while (next < n && s[next] == ' ')
					next++;
			}
			else if (i > start && LyricsLayout_IsCjk(cp) && LyricsLayout_IsCjk(prev_cp)) {
				end = i;
				next = i;
			}

			if (end > 0) {
				if (!LyricsLayout_Fits(b, start, end))
					break;
				fit_end = end;
				fit_next = next;
			}

			prev_cp = cp;
			i += step;
		}

		if (i >= n && LyricsLayout_Fits(b, start, n)) {
			fit_end = n;
			fit_next = n;
		}

		// Not even the first word fits: cut it at the last code point that
		// does, keeping at least one.
		if (fit_end == 0) {
			size_t end = start + LyricsLayout_Step(s, start, n);

			while (end < n) {
				size_t step = LyricsLayout_Step(s, end, n);

				if (!LyricsLayout_Fits(b, start, end + step))
					break;
				end += step;
			}

			fit_end = end;
			fit_next = end;
			while (fit_next < n && s[fit_next] == ' ')
				fit_next++;
		}

		if (LyricsLayout_Push(b, start, fit_end) < 0)
			return -1;

		start = fit_next;
	}

	return 0;
}

int LyricsLayout_Build(const Lyrics *lyrics, float width, LyricsLayout_Measure measure, void *ctx, LyricsLayout *out) {
	memset(out, 0, sizeof(*out));

	LyricsLayout_Builder b = { 0 };
	b.width = width;
	b.measure = measure;
	b.ctx = ctx;
	b.out = out;

	for (uint32_t line = 0; line < lyrics->count; line++) {
		const Lyrics_Line *l = &lyrics->lines[line];

		b.s = lyrics->text + l->off;
		b.base = l->off;
		b.line = line;

		if (LyricsLayout_Line(&b, l->len) < 0) {
			LyricsLayout_Free(out);
			return -1;
		}
	}

	return 0;
}

uint32_t LyricsLayout_FirstRow(const LyricsLayout *layout, uint32_t line) {
	// Rows are sorted by line: the first one whose line is >= `line`.
	uint32_t lo = 0, hi = layout->count;

	while (lo < hi) {
		uint32_t mid = lo + (hi - lo) / 2;

		if (layout->rows[mid].line < line)
			lo = mid + 1;
		else
			hi = mid;
	}

	return (lo < layout->count && layout->rows[lo].line == line) ? lo : layout->count;
}

void LyricsLayout_Free(LyricsLayout *layout) {
	free(layout->rows);
	memset(layout, 0, sizeof(*layout));
}
