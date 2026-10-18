#include <math.h>

#include "viz_bands.h"

// Below this the bass is silence: the gain would otherwise blow up noise.
#define VIZ_BASS_MIN_REFERENCE 0.05f

float VizBands_EdgeHz(int i) {
	return VIZ_BANDS_LOW_HZ * powf(VIZ_BANDS_HIGH_HZ / VIZ_BANDS_LOW_HZ, (float)i / VIZ_BANDS);
}

static int VizBands_Clamp(int bin) {
	if (bin < 1)
		return 1;   // bin 0 is DC, never music
	if (bin > VIZ_FFT_BINS - 1)
		return VIZ_FFT_BINS - 1;
	return bin;
}

void VizBands_SetRate(VizBands_Layout *layout, unsigned int rate) {
	layout->rate = rate;
	layout->bass_bands = 0;

	if (rate == 0)
		rate = 48000;

	float bin_hz = (float)rate / VIZ_FFT_SIZE;

	for (int i = 0; i < VIZ_BANDS; i++) {
		float low = VizBands_EdgeHz(i), high = VizBands_EdgeHz(i + 1);

		// Bins whose centre falls in [low, high).
		int lo = VizBands_Clamp((int)ceilf(low / bin_hz));
		int hi = VizBands_Clamp((int)ceilf(high / bin_hz) - 1);

		if (hi < lo)
			lo = hi = VizBands_Clamp((int)lroundf(sqrtf(low * high) / bin_hz));

		layout->lo[i] = lo;
		layout->hi[i] = hi;

		if (sqrtf(low * high) < VIZ_BASS_HZ)
			layout->bass_bands = i + 1;
	}
}

// Loudest bin of band `i`, linear.
static float VizBands_Peak(const VizBands_Layout *layout, const float *mag, int i) {
	float peak = 0.0f;

	for (int k = layout->lo[i]; k <= layout->hi[i]; k++)
		if (mag[k] > peak)
			peak = mag[k];

	return peak;
}

void VizBands_Levels(const VizBands_Layout *layout, const float *mag, float *db) {
	const float floor_linear = powf(10.0f, VIZ_BANDS_FLOOR_DB / 20.0f);

	for (int i = 0; i < VIZ_BANDS; i++) {
		float peak = VizBands_Peak(layout, mag, i);

		if (peak <= floor_linear)
			db[i] = VIZ_BANDS_FLOOR_DB;
		else if (peak >= 1.0f)
			db[i] = 0.0f;
		else
			db[i] = 20.0f * log10f(peak);
	}
}

float VizBands_Norm(float db) {
	float n = 1.0f - db / VIZ_BANDS_FLOOR_DB;
	return n < 0.0f ? 0.0f : (n > 1.0f ? 1.0f : n);
}

void VizBass_Reset(VizBass *bass) {
	bass->reference = 0.0f;
}

float VizBass_Update(VizBass *bass, const VizBands_Layout *layout, const float *mag, float dt) {
	float energy = 0.0f;

	// The mean, so the floor below compares with a single bin's magnitude.
	for (int i = 0; i < layout->bass_bands; i++)
		energy += VizBands_Peak(layout, mag, i);
	if (layout->bass_bands > 0)
		energy /= layout->bass_bands;

	if (dt > 0.0f)
		bass->reference *= expf(-dt / VIZ_BASS_GAIN_FALL);
	if (energy > bass->reference)
		bass->reference = energy;

	float reference = bass->reference > VIZ_BASS_MIN_REFERENCE ? bass->reference : VIZ_BASS_MIN_REFERENCE;
	float e = energy / reference;
	return e > 1.0f ? 1.0f : e;
}
