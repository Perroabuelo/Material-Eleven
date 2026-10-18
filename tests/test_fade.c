// Pruebas en PC de la rampa de ganancia (source/fade.c), compiladas con el gcc
// del host. fade.c es logica pura, asi que no hace falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fade.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

#define GRAIN 960
#define LEVEL 30000

static void fill(int16_t *buf, unsigned int samples, int16_t value) {
	for (unsigned int i = 0; i < samples; i++)
		buf[i] = value;
}

// Desde ganancia 1, la rampa a 0 de rate / 10 frames (100 ms) termina en
// exactamente ese frame, aplicada en grains como en Audio_Decode.
static void check_reaches_zero(unsigned int rate, unsigned int channels) {
	unsigned int n = rate / 10;
	Fade_Ramp r;
	Fade_Reset(&r, 1.0f);
	Fade_Start(&r, 0.0f, n);

	int16_t buf[GRAIN * 2];
	unsigned int done_at = 0, first_zero = 0, total = 0;
	int prev = LEVEL, max_drop = 0;

	while (total < n + 2 * GRAIN) {
		fill(buf, GRAIN * channels, LEVEL);
		Fade_Apply(&r, buf, GRAIN, channels, 1.0f, 1.0f);

		for (unsigned int i = 0; i < GRAIN; i++) {
			int v = buf[i * channels];
			for (unsigned int c = 1; c < channels; c++)
				CHECK(buf[i * channels + c] == v, "los canales deberian ir juntos (%u Hz, %u canales)", rate, channels);

			if (prev - v > max_drop)
				max_drop = prev - v;
			prev = v;

			if (v == 0 && first_zero == 0)
				first_zero = total + i + 1;
		}

		total += GRAIN;
		if (Fade_Done(&r) && done_at == 0)
			done_at = total;
	}

	CHECK(first_zero == n, "deberia llegar a 0 en el frame %u, llego en el %u (%u Hz, %u canales)", n, first_zero, rate, channels);
	CHECK(done_at > 0 && done_at - GRAIN < n && n <= done_at, "deberia terminar en el grain del frame %u, termino al %u (%u Hz)", n, done_at, rate);
	CHECK(r.gain == 0.0f, "la ganancia deberia quedar en 0, quedo en %f", r.gain);

	// Un frame de 30000 a 0 en n pasos: cada paso baja LEVEL / n, mas el redondeo.
	int allowed = LEVEL / (int)n + 2;
	CHECK(max_drop <= allowed, "la bajada por frame no deberia pasar de %d, fue %d (%u Hz)", allowed, max_drop, rate);
}

static void test_reaches_zero(void) {
	check_reaches_zero(44100, 1);
	check_reaches_zero(44100, 2);
	check_reaches_zero(48000, 1);
	check_reaches_zero(48000, 2);
}

// Invertir a medias sigue desde la ganancia actual: sin saltos y en la mitad
// de los frames.
static void test_reverse_halfway(void) {
	unsigned int n = 4800;
	Fade_Ramp r;
	Fade_Reset(&r, 1.0f);
	Fade_Start(&r, 0.0f, n);

	int16_t *buf = malloc(sizeof(int16_t) * n * 2);
	fill(buf, n / 2, LEVEL);
	Fade_Apply(&r, buf, n / 2, 1, 1.0f, 1.0f);

	float at_turn = r.gain;
	CHECK(at_turn > 0.45f && at_turn < 0.55f, "a medias la ganancia deberia rondar 0.5, es %f", at_turn);

	int prev = buf[n / 2 - 1];
	Fade_Start(&r, 1.0f, n);
	CHECK(r.gain == at_turn, "invertir no deberia mover la ganancia, paso de %f a %f", at_turn, r.gain);

	fill(buf, n, LEVEL);
	Fade_Apply(&r, buf, n, 1, 1.0f, 1.0f);

	int allowed = LEVEL / (int)n + 2;
	int done_frame = 0;
	for (unsigned int i = 0; i < n; i++) {
		int d = abs(buf[i] - prev);
		CHECK(d <= allowed, "frame %u: salto de %d, no deberia pasar de %d", i, d, allowed);
		prev = buf[i];

		if (buf[i] == LEVEL && done_frame == 0)
			done_frame = (int)i + 1;
	}

	CHECK(Fade_Done(&r), "la vuelta deberia terminar");
	CHECK(r.gain == 1.0f, "deberia volver a 1, quedo en %f", r.gain);
	CHECK(abs(done_frame - (int)n / 2) <= 2, "la vuelta deberia tardar unos %u frames, tardo %d", n / 2, done_frame);

	// Ida y vuelta enseguida: unos pocos frames hacia 0 y de nuevo a 1.
	Fade_Reset(&r, 1.0f);
	Fade_Start(&r, 0.0f, n);
	fill(buf, 10, LEVEL);
	Fade_Apply(&r, buf, 10, 1, 1.0f, 1.0f);
	Fade_Start(&r, 1.0f, n);
	fill(buf, 20, LEVEL);
	Fade_Apply(&r, buf, 20, 1, 1.0f, 1.0f);
	CHECK(r.gain == 1.0f && Fade_Done(&r), "una ida y vuelta corta deberia volver a 1, quedo en %f", r.gain);

	free(buf);
}

static void test_saturation(void) {
	Fade_Ramp r;
	Fade_Reset(&r, 1.0f);

	int16_t buf[4] = { 20000, -20000, 32767, -32768 };
	Fade_Apply(&r, buf, 2, 2, 2.0f, 2.0f);

	CHECK(buf[0] == 32767, "deberia saturar en 32767, dio %d", buf[0]);
	CHECK(buf[1] == -32768, "deberia saturar en -32768, dio %d", buf[1]);
	CHECK(buf[2] == 32767, "deberia saturar en 32767, dio %d", buf[2]);
	CHECK(buf[3] == -32768, "deberia saturar en -32768, dio %d", buf[3]);

	// Redondeo al mas cercano, en los dos signos.
	int16_t half[2] = { 3, -3 };
	Fade_Apply(&r, half, 1, 2, 0.5f, 0.5f);
	CHECK(half[0] == 2 && half[1] == -2, "3 x 0.5 deberia redondear a 2 y -2, dio %d y %d", half[0], half[1]);
}

static void test_unity_shortcut(void) {
	Fade_Ramp r;
	Fade_Reset(&r, 1.0f);

	int16_t buf[GRAIN * 2], copy[GRAIN * 2];
	for (int i = 0; i < GRAIN * 2; i++)
		buf[i] = (int16_t)((i * 7919) % 65536 - 32768);
	memcpy(copy, buf, sizeof(buf));

	Fade_Apply(&r, buf, GRAIN, 2, 1.0f, 1.0f);
	CHECK(memcmp(buf, copy, sizeof(buf)) == 0, "con ganancia 1 el buffer deberia quedar identico");

	// Una rampa que ya volvio a 1 tambien.
	Fade_Start(&r, 0.0f, 100);
	Fade_Start(&r, 1.0f, 100);
	CHECK(Fade_Done(&r), "ir y volver sin aplicar nada deberia dejar la rampa en 1");
	Fade_Apply(&r, buf, GRAIN, 2, 1.0f, 1.0f);
	CHECK(memcmp(buf, copy, sizeof(buf)) == 0, "con la rampa de vuelta en 1 el buffer deberia quedar identico");

	// Sin frames, Fade_Start salta al objetivo.
	Fade_Start(&r, 0.0f, 0);
	CHECK(r.gain == 0.0f && Fade_Done(&r), "con 0 frames deberia saltar a 0");
}

// El master se interpola de forma lineal a lo largo del buffer, y dos buffers
// seguidos empalman sin salto.
static void test_master_continuous(void) {
	Fade_Ramp r;
	Fade_Reset(&r, 1.0f);

	int16_t a[GRAIN], b[GRAIN];
	fill(a, GRAIN, LEVEL);
	fill(b, GRAIN, LEVEL);

	Fade_Apply(&r, a, GRAIN, 1, 1.0f, 0.5f);
	Fade_Apply(&r, b, GRAIN, 1, 0.5f, 0.0f);

	CHECK(a[GRAIN - 1] == LEVEL / 2, "el primer buffer deberia terminar en %d, termino en %d", LEVEL / 2, a[GRAIN - 1]);
	CHECK(b[GRAIN - 1] == 0, "el segundo buffer deberia terminar en 0, termino en %d", b[GRAIN - 1]);

	// Cada frame baja lo mismo, tambien al pasar de un buffer al otro.
	double expected = (double)LEVEL / 2 / GRAIN;
	int prev = LEVEL;
	for (int i = 0; i < 2 * GRAIN; i++) {
		int v = (i < GRAIN) ? a[i] : b[i - GRAIN];
		double d = prev - v;
		CHECK(d >= expected - 1.0 && d <= expected + 1.0, "frame %d: bajo %.1f, deberia bajar %.1f", i, d, expected);
		prev = v;
	}

	// El master se multiplica con la rampa.
	Fade_Reset(&r, 0.5f);
	fill(a, GRAIN, LEVEL);
	Fade_Apply(&r, a, GRAIN, 1, 0.5f, 0.5f);
	CHECK(a[0] == LEVEL / 4, "ganancia 0.5 por master 0.5 deberia dar %d, dio %d", LEVEL / 4, a[0]);
}

int main(void) {
	test_reaches_zero();
	test_reverse_halfway();
	test_saturation();
	test_unity_shortcut();
	test_master_continuous();

	if (failures == 0)
		printf("test_fade: todo bien\n");

	return failures == 0 ? 0 : 1;
}
