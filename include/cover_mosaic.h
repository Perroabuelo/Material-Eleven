#ifndef _ELEVENMPV_COVER_MOSAIC_H_
#define _ELEVENMPV_COVER_MOSAIC_H_

// The pixel work of a playlist's cover: four album covers, each halved, in
// the four quarters of one. Pure logic: no vita2d and no SCE headers, so it is
// tested on the PC (tests/test_cover_mosaic.c). Reading and caching the covers
// is source/cover.c's job.
//
// Images are `size` x `size` RGBA, 4 bytes per pixel, rows packed, as the
// cover cache keeps them. `size` is even.

// Quarters in reading order: 0 top left, 1 top right, 2 bottom left,
// 3 bottom right.
#define COVER_MOSAIC_QUARTERS 4

// Halves `src` by averaging each 2x2 block, and writes it into `quarter` of
// `dst`.
void CoverMosaic_Reduce(const unsigned char *src, unsigned char *dst, int size, int quarter);

// Fills `quarter` of `dst` with one color, `rgba` being 4 bytes: what a cover
// that could not be read leaves.
void CoverMosaic_Fill(unsigned char *dst, int size, int quarter, const unsigned char rgba[4]);

#endif
