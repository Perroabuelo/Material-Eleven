// Pruebas en PC de la FFT del visualizador (source/viz_fft.c), compiladas con
// el gcc del host.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "viz_fft.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

#define N VIZ_FFT_SIZE

static void test_matches_dft(void) {
	static float re[N], im[N];
	static double dre[N], dim[N];

	srand(7);
	for (int i = 0; i < N; i++) {
		re[i] = (float)rand() / RAND_MAX * 2.0f - 1.0f;
		im[i] = (float)rand() / RAND_MAX * 2.0f - 1.0f;
	}

	for (int k = 0; k < N; k++) {
		dre[k] = dim[k] = 0.0;
		for (int n = 0; n < N; n++) {
			double a = -2.0 * M_PI * k * n / N;
			dre[k] += re[n] * cos(a) - im[n] * sin(a);
			dim[k] += re[n] * sin(a) + im[n] * cos(a);
		}
	}

	VizFFT_Transform(re, im);

	double worst = 0.0;
	for (int k = 0; k < N; k++) {
		double e = fabs(re[k] - dre[k]) + fabs(im[k] - dim[k]);
		if (e > worst)
			worst = e;
	}

	// Las salidas llegan a unas decenas; un error de float de 1e-2 es ruido.
	CHECK(worst < 0.02, "la FFT deberia coincidir con la DFT directa, error maximo %f", worst);
}

static int peak_bin(const float *mag) {
	int best = 1;
	for (int k = 1; k < VIZ_FFT_BINS; k++)
		if (mag[k] > mag[best])
			best = k;
	return best;
}

static void check_tone(float hz, unsigned int rate) {
	static float in[N], mag[VIZ_FFT_BINS];

	for (int i = 0; i < N; i++)
		in[i] = 0.8f * sinf(2.0f * (float)M_PI * hz * i / rate);

	VizFFT_Magnitudes(in, mag);

	int expect = (int)lroundf(hz * N / rate);
	int got = peak_bin(mag);
	CHECK(got == expect, "%.0f Hz a %u deberia caer en el bin %d, cayo en %d", hz, rate, expect, got);
	CHECK(fabsf(VizFFT_BinHz(got, rate) - hz) <= (float)rate / N, "el bin %d deberia estar a menos de un bin de %.0f Hz", got, hz);

	// Entre el bin exacto y el peor caso a medio bin (Hann pierde ~1,4 dB).
	CHECK(mag[got] > 0.8f * 0.7f && mag[got] < 0.8f * 1.05f, "un seno de 0,8 deberia medir cerca de 0,8, midio %f", mag[got]);
}

static void test_tones(void) {
	const unsigned int rates[] = { 44100, 48000 };
	const float tones[] = { 100.0f, 1000.0f, 8000.0f };

	for (int r = 0; r < 2; r++)
		for (int t = 0; t < 3; t++)
			check_tone(tones[t], rates[r]);
}

static void test_full_scale_on_bin(void) {
	static float in[N], mag[VIZ_FFT_BINS];

	// Un seno de amplitud 1 justo en el bin 64.
	for (int i = 0; i < N; i++)
		in[i] = sinf(2.0f * (float)M_PI * 64 * i / N);

	VizFFT_Magnitudes(in, mag);
	CHECK(fabsf(mag[64] - 1.0f) < 0.01f, "un seno a escala completa en un bin deberia medir 1, midio %f", mag[64]);
	CHECK(mag[200] < 1e-3f, "lejos del tono no deberia haber energia, midio %f", mag[200]);
	CHECK(in[1] != 0.0f, "Magnitudes no deberia cambiar la entrada");
}

static void test_silence(void) {
	static float in[N], mag[VIZ_FFT_BINS];

	for (int i = 0; i < N; i++)
		in[i] = 0.0f;
	VizFFT_Magnitudes(in, mag);

	int zero = 1;
	for (int k = 0; k < VIZ_FFT_BINS; k++)
		zero &= mag[k] == 0.0f;
	CHECK(zero, "el silencio deberia dar magnitudes 0");
}

static void test_mono_from_stereo(void) {
	int16_t st[6] = { 32767, 32767, -32768, -32768, 16384, -16384 };
	float out[3];

	VizFFT_MonoFromStereo(st, 3, out);
	CHECK(fabsf(out[0] - 1.0f) < 1e-3f, "L=R=max deberia dar 1, dio %f", out[0]);
	CHECK(fabsf(out[1] + 1.0f) < 1e-6f, "L=R=min deberia dar -1, dio %f", out[1]);
	CHECK(out[2] == 0.0f, "L y R opuestos deberian dar 0, dio %f", out[2]);
}

int main(void) {
	test_matches_dft();
	test_tones();
	test_full_scale_on_bin();
	test_silence();
	test_mono_from_stereo();

	if (failures == 0)
		printf("test_viz_fft: todo bien\n");

	return failures == 0 ? 0 : 1;
}
