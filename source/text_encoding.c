#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "text_encoding.h"

#define REPLACEMENT 0xFFFD

// Windows-1252 from 0x80 to 0x9F; from 0xA0 on it matches Latin-1, which is
// the code point itself. The five unassigned bytes map to U+FFFD. Source: the
// Unicode mapping table for CP1252.
static const uint16_t cp1252_high[32] = {
	0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
	0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
	0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
	0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178,
};

size_t TextEncoding_Utf8SeqLen(const unsigned char *s, size_t avail) {
	if (avail == 0)
		return 0;

	unsigned char c = s[0];

	if (c < 0x80)
		return 1;

	// Lead byte, and the range allowed for the second byte: the narrower
	// ranges after E0, ED, F0 and F4 are what rule out overlong forms,
	// surrogates and values above U+10FFFF.
	size_t need;
	unsigned char lo = 0x80, hi = 0xBF;

	if (c >= 0xC2 && c <= 0xDF)
		need = 2;
	else if (c >= 0xE0 && c <= 0xEF) {
		need = 3;
		if (c == 0xE0)
			lo = 0xA0;
		else if (c == 0xED)
			hi = 0x9F;
	}
	else if (c >= 0xF0 && c <= 0xF4) {
		need = 4;
		if (c == 0xF0)
			lo = 0x90;
		else if (c == 0xF4)
			hi = 0x8F;
	}
	else
		return 0;

	if (avail < need)
		return 0;

	if (s[1] < lo || s[1] > hi)
		return 0;

	for (size_t i = 2; i < need; i++) {
		if (s[i] < 0x80 || s[i] > 0xBF)
			return 0;
	}

	return need;
}

static int TextEncoding_IsValidUtf8(const unsigned char *s, size_t len) {
	size_t i = 0;

	while (i < len) {
		size_t n = TextEncoding_Utf8SeqLen(s + i, len - i);

		if (n == 0)
			return 0;

		i += n;
	}

	return 1;
}

static size_t TextEncoding_PutCodePoint(char *dst, uint32_t cp) {
	unsigned char *d = (unsigned char *)dst;

	if (cp < 0x80) {
		d[0] = cp;
		return 1;
	}

	if (cp < 0x800) {
		d[0] = 0xC0 | (cp >> 6);
		d[1] = 0x80 | (cp & 0x3F);
		return 2;
	}

	if (cp < 0x10000) {
		d[0] = 0xE0 | (cp >> 12);
		d[1] = 0x80 | ((cp >> 6) & 0x3F);
		d[2] = 0x80 | (cp & 0x3F);
		return 3;
	}

	d[0] = 0xF0 | (cp >> 18);
	d[1] = 0x80 | ((cp >> 12) & 0x3F);
	d[2] = 0x80 | ((cp >> 6) & 0x3F);
	d[3] = 0x80 | (cp & 0x3F);
	return 4;
}

// Room for `units` input units that expand to at most 3 bytes each, plus the
// NUL. NULL when the size would overflow or malloc fails.
static char *TextEncoding_Alloc(size_t units) {
	if (units > (SIZE_MAX - 1) / 3)
		return NULL;

	return malloc(units * 3 + 1);
}

static int TextEncoding_Finish(char *buf, size_t used, char **out, size_t *out_len) {
	buf[used] = '\0';
	*out = buf;
	*out_len = used;
	return 0;
}

static int TextEncoding_Fail(char **out, size_t *out_len) {
	*out = NULL;
	*out_len = 0;
	return -1;
}

int TextEncoding_SanitizeUtf8(const unsigned char *in, size_t len, char **out, size_t *out_len) {
	// An invalid byte becomes U+FFFD, 3 bytes; a valid sequence never grows.
	char *buf = TextEncoding_Alloc(len);

	if (buf == NULL)
		return TextEncoding_Fail(out, out_len);

	size_t used = 0, i = 0;

	while (i < len) {
		size_t n = TextEncoding_Utf8SeqLen(in + i, len - i);

		if (n == 0) {
			used += TextEncoding_PutCodePoint(buf + used, REPLACEMENT);
			i++;
		}
		else {
			memcpy(buf + used, in + i, n);
			used += n;
			i += n;
		}
	}

	return TextEncoding_Finish(buf, used, out, out_len);
}

static int TextEncoding_FromUtf16(const unsigned char *in, size_t len, int big_endian, char **out, size_t *out_len) {
	// A code unit gives at most 3 bytes, and a surrogate pair (two units) 4.
	// An odd trailing byte counts as one more unit for its U+FFFD.
	char *buf = TextEncoding_Alloc(len / 2 + 1);

	if (buf == NULL)
		return TextEncoding_Fail(out, out_len);

	size_t used = 0, i = 0;

	while (i + 1 < len) {
		uint32_t u = big_endian ? ((uint32_t)in[i] << 8 | in[i + 1]) : ((uint32_t)in[i + 1] << 8 | in[i]);
		i += 2;

		if (u >= 0xD800 && u <= 0xDBFF) {
			uint32_t lo = 0;

			if (i + 1 < len)
				lo = big_endian ? ((uint32_t)in[i] << 8 | in[i + 1]) : ((uint32_t)in[i + 1] << 8 | in[i]);

			if (lo >= 0xDC00 && lo <= 0xDFFF) {
				u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00);
				i += 2;
			}
			else
				u = REPLACEMENT; // high surrogate without its pair; the next unit is read on its own
		}
		else if (u >= 0xDC00 && u <= 0xDFFF)
			u = REPLACEMENT;

		used += TextEncoding_PutCodePoint(buf + used, u);
	}

	if (i < len)
		used += TextEncoding_PutCodePoint(buf + used, REPLACEMENT);

	return TextEncoding_Finish(buf, used, out, out_len);
}

static int TextEncoding_FromCp1252(const unsigned char *in, size_t len, char **out, size_t *out_len) {
	char *buf = TextEncoding_Alloc(len);

	if (buf == NULL)
		return TextEncoding_Fail(out, out_len);

	size_t used = 0;

	for (size_t i = 0; i < len; i++) {
		uint32_t cp = in[i];

		if (cp >= 0x80 && cp <= 0x9F)
			cp = cp1252_high[cp - 0x80];

		used += TextEncoding_PutCodePoint(buf + used, cp);
	}

	return TextEncoding_Finish(buf, used, out, out_len);
}

int TextEncoding_ToUtf8(const unsigned char *in, size_t len, char **out, size_t *out_len) {
	if (len >= 3 && in[0] == 0xEF && in[1] == 0xBB && in[2] == 0xBF)
		return TextEncoding_SanitizeUtf8(in + 3, len - 3, out, out_len);

	if (len >= 2 && in[0] == 0xFF && in[1] == 0xFE)
		return TextEncoding_FromUtf16(in + 2, len - 2, 0, out, out_len);

	if (len >= 2 && in[0] == 0xFE && in[1] == 0xFF)
		return TextEncoding_FromUtf16(in + 2, len - 2, 1, out, out_len);

	if (TextEncoding_IsValidUtf8(in, len))
		return TextEncoding_SanitizeUtf8(in, len, out, out_len);

	return TextEncoding_FromCp1252(in, len, out, out_len);
}
