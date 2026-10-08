#include "capture_bmp.h"

static void CaptureBmp_Put16(unsigned char *p, unsigned int v) {
	p[0] = v & 0xFF;
	p[1] = (v >> 8) & 0xFF;
}

static void CaptureBmp_Put32(unsigned char *p, unsigned int v) {
	p[0] = v & 0xFF;
	p[1] = (v >> 8) & 0xFF;
	p[2] = (v >> 16) & 0xFF;
	p[3] = (v >> 24) & 0xFF;
}

unsigned int CaptureBmp_FileSize(unsigned int width, unsigned int height) {
	if (width == 0 || height == 0 || width > CAPTURE_BMP_MAX_SIDE || height > CAPTURE_BMP_MAX_SIDE)
		return 0;

	return CAPTURE_BMP_HEADER_SIZE + width * height * 4;
}

int CaptureBmp_WriteHeader(unsigned char *dst, unsigned int width, unsigned int height) {
	unsigned int file_size = CaptureBmp_FileSize(width, height);

	if (dst == 0 || file_size == 0)
		return 0;

	for (int i = 0; i < CAPTURE_BMP_HEADER_SIZE; i++)
		dst[i] = 0;

	// File header.
	dst[0] = 'B';
	dst[1] = 'M';
	CaptureBmp_Put32(dst + 2, file_size);
	CaptureBmp_Put32(dst + 10, CAPTURE_BMP_HEADER_SIZE);

	// BITMAPINFOHEADER. A positive height means bottom-up rows, which every
	// viewer reads; top-down is a negative height that a few still get wrong.
	CaptureBmp_Put32(dst + 14, 40);
	CaptureBmp_Put32(dst + 18, width);
	CaptureBmp_Put32(dst + 22, height);
	CaptureBmp_Put16(dst + 26, 1);   // planes
	CaptureBmp_Put16(dst + 28, 32);  // bits per pixel
	CaptureBmp_Put32(dst + 30, 0);   // BI_RGB, uncompressed
	CaptureBmp_Put32(dst + 34, width * height * 4);
	CaptureBmp_Put32(dst + 38, 2835); // 72 dpi, in pixels per metre
	CaptureBmp_Put32(dst + 42, 2835);

	return 1;
}

unsigned int CaptureBmp_SourceRow(unsigned int file_row, unsigned int height) {
	return height - 1 - file_row;
}

void CaptureBmp_ConvertRow(const unsigned char *src, unsigned char *dst, unsigned int width) {
	for (unsigned int x = 0; x < width; x++) {
		const unsigned char *s = src + x * 4;
		unsigned char *d = dst + x * 4;

		d[0] = s[2];
		d[1] = s[1];
		d[2] = s[0];
		d[3] = 255;
	}
}
