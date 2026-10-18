#ifndef _ELEVENMPV_VIZ_FFT_H_
#define _ELEVENMPV_VIZ_FFT_H_

#include <stdint.h>

// The visualizer's spectrum: an iterative radix-2 FFT of VIZ_FFT_SIZE points
// in float, with a Hann window. The sine, window and bit-reversal tables are
// built once, on first use, into static arrays.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_viz_fft.c).

#define VIZ_FFT_SIZE 1024
#define VIZ_FFT_BINS (VIZ_FFT_SIZE / 2)

// In-place complex FFT of VIZ_FFT_SIZE points, no window and no scaling.
void VizFFT_Transform(float *re, float *im);

// Average of L and R of `frames` stereo frames, scaled to [-1, 1].
void VizFFT_MonoFromStereo(const int16_t *stereo, unsigned int frames, float *out);

// Windows VIZ_FFT_SIZE samples in [-1, 1] and writes VIZ_FFT_BINS magnitudes
// to `mag`, scaled so a full-scale sine on a bin reads 1.0. `in` is not
// changed.
void VizFFT_Magnitudes(const float *in, float *mag);

// Centre frequency of `bin` at `rate`.
float VizFFT_BinHz(unsigned int bin, unsigned int rate);

#endif
