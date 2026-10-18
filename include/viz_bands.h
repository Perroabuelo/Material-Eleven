#ifndef _ELEVENMPV_VIZ_BANDS_H_
#define _ELEVENMPV_VIZ_BANDS_H_

// Groups the FFT bins (viz_fft.h) into VIZ_BANDS bands with logarithmic edges,
// so the bass and the mids take as much of the width as the treble, and gives
// each one a level in dB. Also tracks the bass energy that makes the vinyl
// pulse, with a slow automatic gain so a loud master and a quiet one pulse
// alike.
//
// Pure logic: no vita2d and no SCE headers, so it can be tested on a PC
// (tests/test_viz_bands.c).

#include "viz_fft.h"

#define VIZ_BANDS          32
#define VIZ_BANDS_LOW_HZ   40.0f
#define VIZ_BANDS_HIGH_HZ  16000.0f
#define VIZ_BANDS_FLOOR_DB -60.0f

// Bands centred below this count as bass.
#define VIZ_BASS_HZ        150.0f

// How fast the bass reference falls after a loud passage, in seconds.
#define VIZ_BASS_GAIN_FALL 3.0f

typedef struct {
	unsigned int rate;
	int lo[VIZ_BANDS], hi[VIZ_BANDS];   // inclusive bin range of each band
	int bass_bands;                     // the first bass_bands bands are bass
} VizBands_Layout;

typedef struct {
	float reference;    // loudest recent bass, falling slowly
} VizBass;

// Lower edge of band `i`, in Hz; i = VIZ_BANDS is the top edge of the last one.
float VizBands_EdgeHz(int i);

// The bins of each band at `rate`. A band narrower than a bin takes the bin
// its centre falls in, so no band is ever empty.
void VizBands_SetRate(VizBands_Layout *layout, unsigned int rate);

// Level of each band in dB from the magnitudes of VizFFT_Magnitudes, clamped
// to [VIZ_BANDS_FLOOR_DB, 0].
void VizBands_Levels(const VizBands_Layout *layout, const float *mag, float *db);

// A level in dB as a share of the range: 0 at the floor, 1 at 0 dBFS.
float VizBands_Norm(float db);

void VizBass_Reset(VizBass *bass);

// Bass energy of this frame (the mean level of the bass bands) in [0, 1],
// relative to the loudest recent bass.
// `dt` is the time since the previous frame, in seconds.
float VizBass_Update(VizBass *bass, const VizBands_Layout *layout, const float *mag, float dt);

#endif
