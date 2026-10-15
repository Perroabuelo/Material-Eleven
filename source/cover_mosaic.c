#include <stddef.h>

#include "cover_mosaic.h"

// Where `quarter` starts in `dst`, in bytes.
static size_t CoverMosaic_Origin(int size, int quarter) {
	int half = size / 2;
	int x = (quarter % 2) * half;
	int y = (quarter / 2) * half;

	return ((size_t)y * (size_t)size + (size_t)x) * 4;
}

void CoverMosaic_Reduce(const unsigned char *src, unsigned char *dst, int size, int quarter) {
	int half = size / 2;
	size_t stride = (size_t)size * 4;
	unsigned char *out = dst + CoverMosaic_Origin(size, quarter);

	for (int y = 0; y < half; y++) {
		const unsigned char *row0 = src + (size_t)(2 * y) * stride;
		const unsigned char *row1 = row0 + stride;
		unsigned char *o = out + (size_t)y * stride;

		for (int x = 0; x < half; x++) {
			const unsigned char *a = row0 + (size_t)(2 * x) * 4;
			const unsigned char *b = row1 + (size_t)(2 * x) * 4;

			// Rounded to the nearest, not down, so a flat color stays itself.
			for (int c = 0; c < 4; c++)
				o[(size_t)x * 4 + c] = (unsigned char)((a[c] + a[4 + c] + b[c] + b[4 + c] + 2) / 4);
		}
	}
}

void CoverMosaic_Fill(unsigned char *dst, int size, int quarter, const unsigned char rgba[4]) {
	int half = size / 2;
	size_t stride = (size_t)size * 4;
	unsigned char *out = dst + CoverMosaic_Origin(size, quarter);

	for (int y = 0; y < half; y++) {
		unsigned char *o = out + (size_t)y * stride;

		for (int x = 0; x < half; x++) {
			for (int c = 0; c < 4; c++)
				o[(size_t)x * 4 + c] = rgba[c];
		}
	}
}
