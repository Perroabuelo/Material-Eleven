#ifndef _ELEVENMPV_CAPTURE_BMP_H_
#define _ELEVENMPV_CAPTURE_BMP_H_

// Turns a framebuffer copy into a 32-bit BMP, for the debug overlay's capture
// mode (source/ui_gpu.c). Pure logic: no vita2d and no SCE headers, so it is
// tested on the PC (tests/test_capture_bmp.c). Writing the file is the
// caller's job; this only lays out the bytes.

// File header (14) plus BITMAPINFOHEADER (40).
#define CAPTURE_BMP_HEADER_SIZE 54

// Larger than anything the console can display, and small enough that the
// file size always fits the header's 32-bit field.
#define CAPTURE_BMP_MAX_SIDE 4096

// Total size of the file for a width x height image, or 0 when either side is
// 0 or above CAPTURE_BMP_MAX_SIDE.
unsigned int CaptureBmp_FileSize(unsigned int width, unsigned int height);

// Fills the CAPTURE_BMP_HEADER_SIZE bytes at `dst`. Returns 0 and leaves `dst`
// untouched when the size is invalid (see CaptureBmp_FileSize).
int CaptureBmp_WriteHeader(unsigned char *dst, unsigned int width, unsigned int height);

// BMP rows run bottom-up: row `file_row` of the file holds this row of the
// source image.
unsigned int CaptureBmp_SourceRow(unsigned int file_row, unsigned int height);

// Converts `width` pixels from the framebuffer's byte order (R, G, B, A) to
// the BMP's (B, G, R, X). The fourth byte is written as 255: the framebuffer
// alpha is whatever the last draw left there, and some viewers honour it.
void CaptureBmp_ConvertRow(const unsigned char *src, unsigned char *dst, unsigned int width);

#endif
