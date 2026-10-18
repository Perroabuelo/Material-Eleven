// Pruebas en PC de las bandas y la energia de graves del visualizador
// (source/viz_bands.c), compiladas con el gcc del host.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <math.h>
#include <stdio.h>

#include "viz_bands.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

static const unsigned int rates[] = { 22050, 32000, 44100, 48000 };

static void test_edges(void) {
	CHECK(fabsf(VizBands_EdgeHz(0) - VIZ_BANDS_LOW_HZ) < 0.01f, "el primer borde deberia ser %f", VIZ_BANDS_LOW_HZ);
	CHECK(fabsf(VizBands_EdgeHz(VIZ_BANDS) - VIZ_BANDS_HIGH_HZ) < 1.0f, "el ultimo borde deberia ser %f, dio %f", VIZ_BANDS_HIGH_HZ, VizBands_EdgeHz(VIZ_BANDS));

	float ratio = VizBands_EdgeHz(1) / VizBands_EdgeHz(0);
	for (int i = 1; i <= VIZ_BANDS; i++) {
		float r = VizBands_EdgeHz(i) / VizBands_EdgeHz(i - 1);
		CHECK(r > 1.0f, "los bordes deberian crecer (%d)", i);
		CHECK(fabsf(r - ratio) < 1e-3f, "los bordes deberian ser logaritmicos: razon %f en %d, %f en 1", r, i, ratio);
	}
}

static void test_layout(void) {
	VizBands_Layout l;

	for (int r = 0; r < 4; r++) {
		VizBands_SetRate(&l, rates[r]);
		CHECK(l.rate == rates[r], "deberia guardar la frecuencia");

		for (int i = 0; i < VIZ_BANDS; i++) {
			CHECK(l.lo[i] >= 1 && l.lo[i] <= l.hi[i] && l.hi[i] < VIZ_FFT_BINS,
				"la banda %d a %u no deberia quedar vacia ni fuera: %d..%d", i, rates[r], l.lo[i], l.hi[i]);
			if (i > 0)
				CHECK(l.lo[i] >= l.lo[i - 1] && l.hi[i] >= l.hi[i - 1], "las bandas deberian ir de graves a agudos (%d a %u)", i, rates[r]);
		}

		CHECK(l.bass_bands > 0 && l.bass_bands < VIZ_BANDS / 2, "deberia haber algunas bandas de graves, hay %d", l.bass_bands);
		float centre = sqrtf(VizBands_EdgeHz(l.bass_bands - 1) * VizBands_EdgeHz(l.bass_bands));
		CHECK(centre < VIZ_BASS_HZ, "la ultima banda de graves deberia estar centrada bajo 150 Hz, esta en %f", centre);
	}
}

// Banda que contiene `hz`.
static int band_of(float hz) {
	for (int i = 0; i < VIZ_BANDS; i++)
		if (hz < VizBands_EdgeHz(i + 1))
			return i;
	return VIZ_BANDS - 1;
}

static void tone_levels(float hz, unsigned int rate, float amp, float *db, VizBands_Layout *l) {
	static float in[VIZ_FFT_SIZE], mag[VIZ_FFT_BINS];

	for (int i = 0; i < VIZ_FFT_SIZE; i++)
		in[i] = amp * sinf(2.0f * (float)M_PI * hz * i / rate);

	VizFFT_Magnitudes(in, mag);
	VizBands_SetRate(l, rate);
	VizBands_Levels(l, mag, db);
}

static void test_tone_band(void) {
	const float tones[] = { 100.0f, 1000.0f, 8000.0f };
	VizBands_Layout l;
	float db[VIZ_BANDS];

	for (int r = 2; r < 4; r++) {
		for (int t = 0; t < 3; t++) {
			tone_levels(tones[t], rates[r], 0.8f, db, &l);
			int b = band_of(tones[t]);

			for (int i = 0; i < VIZ_BANDS; i++)
				CHECK(db[b] >= db[i], "%.0f Hz a %u: la banda %d (%f dB) deberia ser la mas alta, la %d da %f", tones[t], rates[r], b, db[b], i, db[i]);
			CHECK(db[b] > -6.0f, "%.0f Hz deberia encender su banda, dio %f dB", tones[t], db[b]);
		}
	}

	// Un tono grave deja la mitad derecha casi en cero, y uno agudo la izquierda.
	tone_levels(100.0f, 44100, 0.8f, db, &l);
	for (int i = VIZ_BANDS / 2; i < VIZ_BANDS; i++)
		CHECK(VizBands_Norm(db[i]) < 0.1f, "con 100 Hz la banda %d deberia quedar casi en cero, dio %f dB", i, db[i]);

	tone_levels(8000.0f, 44100, 0.8f, db, &l);
	for (int i = 0; i < VIZ_BANDS / 4; i++)
		CHECK(VizBands_Norm(db[i]) < 0.1f, "con 8 kHz la banda %d deberia quedar casi en cero, dio %f dB", i, db[i]);
	CHECK(band_of(8000.0f) >= VIZ_BANDS / 2, "8 kHz deberia caer en la mitad derecha");
}

static void test_silence(void) {
	VizBands_Layout l;
	float db[VIZ_BANDS];

	tone_levels(1000.0f, 48000, 0.0f, db, &l);
	for (int i = 0; i < VIZ_BANDS; i++)
		CHECK(db[i] == VIZ_BANDS_FLOOR_DB, "el silencio deberia dar %f dB en la banda %d, dio %f", VIZ_BANDS_FLOOR_DB, i, db[i]);
}

static void test_norm(void) {
	CHECK(VizBands_Norm(VIZ_BANDS_FLOOR_DB) == 0.0f, "el piso deberia ser 0");
	CHECK(VizBands_Norm(0.0f) == 1.0f, "0 dB deberia ser 1");
	CHECK(fabsf(VizBands_Norm(-30.0f) - 0.5f) < 1e-6f, "-30 dB deberia ser 0,5");
	CHECK(VizBands_Norm(-90.0f) == 0.0f && VizBands_Norm(6.0f) == 1.0f, "deberia acotar a [0, 1]");
}

static float bass_frame(VizBass *bass, float amp, float dt) {
	static float in[VIZ_FFT_SIZE], mag[VIZ_FFT_BINS];
	VizBands_Layout l;

	for (int i = 0; i < VIZ_FFT_SIZE; i++)
		in[i] = amp * sinf(2.0f * (float)M_PI * 60.0f * i / 48000);

	VizFFT_Magnitudes(in, mag);
	VizBands_SetRate(&l, 48000);
	return VizBass_Update(bass, &l, mag, dt);
}

static void test_bass(void) {
	VizBass loud, soft;
	VizBass_Reset(&loud);
	VizBass_Reset(&soft);

	// El mismo bombo en un master fuerte y en uno suave late igual.
	float e_loud = bass_frame(&loud, 0.9f, 1.0f / 60);
	float e_soft = bass_frame(&soft, 0.2f, 1.0f / 60);
	CHECK(e_loud > 0.99f && e_soft > 0.99f, "el golpe deberia llegar a 1 en los dos masters: %f %f", e_loud, e_soft);

	// Un golpe a la mitad, enseguida, marca la mitad.
	float half = bass_frame(&loud, 0.45f, 1.0f / 60);
	CHECK(half > 0.4f && half < 0.6f, "un golpe a la mitad deberia dar cerca de 0,5, dio %f", half);

	// El silencio no tiene graves.
	CHECK(bass_frame(&loud, 0.0f, 1.0f / 60) == 0.0f, "el silencio deberia dar 0");

	// Tras unos segundos suaves, la ganancia se adapta.
	float e = 0.0f;
	for (int i = 0; i < 60 * 10; i++)
		e = bass_frame(&loud, 0.2f, 1.0f / 60);
	CHECK(e > 0.99f, "despues de 10 s suaves el golpe suave deberia volver a 1, dio %f", e);
}

static void test_bass_quiet_floor(void) {
	VizBass bass;
	VizBass_Reset(&bass);

	// Un murmullo en los graves no se amplifica hasta parecer un golpe.
	float e = bass_frame(&bass, 0.005f, 1.0f / 60);
	CHECK(e < 0.2f, "un murmullo no deberia latir fuerte, dio %f", e);
}

int main(void) {
	test_edges();
	test_layout();
	test_tone_band();
	test_silence();
	test_norm();
	test_bass();
	test_bass_quiet_floor();

	if (failures == 0)
		printf("test_viz_bands: todo bien\n");

	return failures == 0 ? 0 : 1;
}
