#include <math.h>

#include "viz_fft.h"

#define PI_F 3.14159265358979f

static int tables_ready = 0;
static float sin_table[VIZ_FFT_SIZE / 2];   // sin(2*pi*k/N); cos is a quarter later
static float hann[VIZ_FFT_SIZE];
static uint16_t bit_reverse[VIZ_FFT_SIZE];

static void VizFFT_BuildTables(void) {
	if (tables_ready)
		return;

	int bits = 0;
	while ((1 << bits) < VIZ_FFT_SIZE)
		bits++;

	for (int i = 0; i < VIZ_FFT_SIZE; i++) {
		int r = 0;
		for (int b = 0; b < bits; b++)
			if (i & (1 << b))
				r |= 1 << (bits - 1 - b);
		bit_reverse[i] = (uint16_t)r;

		hann[i] = 0.5f - 0.5f * cosf(2.0f * PI_F * i / VIZ_FFT_SIZE);
	}

	for (int k = 0; k < VIZ_FFT_SIZE / 2; k++)
		sin_table[k] = sinf(2.0f * PI_F * k / VIZ_FFT_SIZE);

	tables_ready = 1;
}

// cos(2*pi*k/N) for k in [0, N/2), from the sine table.
static float VizFFT_Cos(int k) {
	int q = k + VIZ_FFT_SIZE / 4;
	return q < VIZ_FFT_SIZE / 2 ? sin_table[q] : -sin_table[q - VIZ_FFT_SIZE / 2];
}

void VizFFT_Transform(float *re, float *im) {
	VizFFT_BuildTables();

	for (int i = 0; i < VIZ_FFT_SIZE; i++) {
		int j = bit_reverse[i];
		if (j > i) {
			float t = re[i]; re[i] = re[j]; re[j] = t;
			t = im[i]; im[i] = im[j]; im[j] = t;
		}
	}

	for (int len = 2; len <= VIZ_FFT_SIZE; len <<= 1) {
		int half = len / 2;
		int step = VIZ_FFT_SIZE / len;

		for (int base = 0; base < VIZ_FFT_SIZE; base += len) {
			for (int k = 0; k < half; k++) {
				// Twiddle e^(-2*pi*i*k/len).
				float wr = VizFFT_Cos(k * step);
				float wi = -sin_table[k * step];

				int a = base + k, b = a + half;
				float tr = re[b] * wr - im[b] * wi;
				float ti = re[b] * wi + im[b] * wr;

				re[b] = re[a] - tr;
				im[b] = im[a] - ti;
				re[a] += tr;
				im[a] += ti;
			}
		}
	}
}

void VizFFT_MonoFromStereo(const int16_t *stereo, unsigned int frames, float *out) {
	for (unsigned int i = 0; i < frames; i++)
		out[i] = ((float)stereo[i * 2] + (float)stereo[i * 2 + 1]) * (0.5f / 32768.0f);
}

void VizFFT_Magnitudes(const float *in, float *mag) {
	static float re[VIZ_FFT_SIZE], im[VIZ_FFT_SIZE];

	VizFFT_BuildTables();

	for (int i = 0; i < VIZ_FFT_SIZE; i++) {
		re[i] = in[i] * hann[i];
		im[i] = 0.0f;
	}

	VizFFT_Transform(re, im);

	// A sine of amplitude A peaks at A*N/2 with no window, and Hann halves
	// that: A*N/4.
	const float scale = 4.0f / VIZ_FFT_SIZE;

	for (int k = 0; k < VIZ_FFT_BINS; k++)
		mag[k] = sqrtf(re[k] * re[k] + im[k] * im[k]) * scale;
}

float VizFFT_BinHz(unsigned int bin, unsigned int rate) {
	return (float)bin * (float)rate / VIZ_FFT_SIZE;
}
