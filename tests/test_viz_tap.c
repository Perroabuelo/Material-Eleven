// Pruebas en PC del buffer circular del visualizador (source/viz_tap.c),
// compiladas con el gcc del host.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <string.h>

#include "viz_tap.h"

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

static VizTap tap;
static int16_t out[VIZ_TAP_FRAMES * 2];

// Escribe un grain estereo cuyo frame i vale (base + i) en L y -(base + i) en R.
static void write_stereo(int base) {
	int16_t buf[GRAIN * 2];
	for (int i = 0; i < GRAIN; i++) {
		buf[i * 2] = (int16_t)(base + i);
		buf[i * 2 + 1] = (int16_t)-(base + i);
	}
	VizTap_Write(&tap, buf, GRAIN, 2, 48000);
}

static void test_empty(void) {
	VizTap_Reset(&tap);
	memset(out, 0x55, sizeof(out));

	unsigned int rate = 123;
	int ok = VizTap_Read(&tap, 0, out, 1024, &rate);
	CHECK(ok, "leer un buffer vacio deberia funcionar");
	CHECK(rate == 0, "sin escrituras la frecuencia deberia ser 0, dio %u", rate);

	int zeros = 1;
	for (int i = 0; i < 2048; i++)
		zeros &= out[i] == 0;
	CHECK(zeros, "un buffer vacio deberia leerse como silencio");
}

static void test_stereo(void) {
	VizTap_Reset(&tap);
	write_stereo(0);

	unsigned int rate = 0;
	int ok = VizTap_Read(&tap, 0, out, 100, &rate);
	CHECK(ok && rate == 48000, "deberia leer con 48000, dio ok=%d rate=%u", ok, rate);
	CHECK(out[0] == GRAIN - 100 && out[1] == -(GRAIN - 100), "la ventana deberia empezar en el frame 860, dio %d %d", out[0], out[1]);
	CHECK(out[198] == GRAIN - 1 && out[199] == -(GRAIN - 1), "la ventana deberia terminar en el ultimo frame, dio %d", out[198]);
}

static void test_mono(void) {
	VizTap_Reset(&tap);

	int16_t buf[GRAIN];
	for (int i = 0; i < GRAIN; i++)
		buf[i] = (int16_t)(i * 3);
	VizTap_Write(&tap, buf, GRAIN, 1, 44100);

	unsigned int rate = 0;
	VizTap_Read(&tap, 0, out, GRAIN, &rate);
	CHECK(rate == 44100, "deberia publicar 44100, dio %u", rate);

	int same = 1;
	for (int i = 0; i < GRAIN; i++)
		same &= out[i * 2] == i * 3 && out[i * 2 + 1] == i * 3;
	CHECK(same, "un frame mono deberia ir a los dos canales");
	CHECK(tap.written == GRAIN, "un grain mono deberia contar %d frames, dio %u", GRAIN, tap.written);
}

static void test_bad_channels(void) {
	VizTap_Reset(&tap);
	int16_t buf[6] = {1, 2, 3, 4, 5, 6};
	VizTap_Write(&tap, buf, 2, 3, 48000);
	VizTap_Write(&tap, buf, 0, 2, 48000);
	CHECK(tap.written == 0, "escrituras invalidas no deberian contar, dio %u", tap.written);
}

static void test_delay(void) {
	VizTap_Reset(&tap);
	for (int g = 0; g < 4; g++)
		write_stereo(g * GRAIN);

	// Cuatro grains escritos: frames 0..3839. Con retraso de dos grains,
	// la ventana termina en el frame 1919.
	int ok = VizTap_Read(&tap, 2 * GRAIN, out, 512, NULL);
	CHECK(ok, "deberia leer con retraso");
	CHECK(out[511 * 2] == 2 * GRAIN - 1, "el ultimo frame deberia ser 1919, dio %d", out[511 * 2]);
	CHECK(out[0] == 2 * GRAIN - 512, "el primero deberia ser 1408, dio %d", out[0]);
}

static void test_delay_before_start(void) {
	VizTap_Reset(&tap);
	write_stereo(1);

	// La ventana cae en parte antes del reinicio: esa parte es silencio.
	int ok = VizTap_Read(&tap, 500, out, 1024, NULL);
	CHECK(ok, "deberia leer");
	CHECK(out[0] == 0 && out[(1024 - 461) * 2 - 2] == 0, "lo anterior al reinicio deberia ser silencio");
	CHECK(out[(1024 - 460) * 2] == 1, "el primer frame escrito deberia estar en su lugar, dio %d", out[(1024 - 460) * 2]);
	CHECK(out[1023 * 2] == 460, "el ultimo deberia ser el frame 459 (valor 460), dio %d", out[1023 * 2]);

	// Con un retraso mayor que todo lo escrito, todo es silencio.
	ok = VizTap_Read(&tap, 5000, out, 1024, NULL);
	int zeros = 1;
	for (int i = 0; i < 2048; i++)
		zeros &= out[i] == 0;
	CHECK(ok && zeros, "un retraso mayor que lo escrito deberia dar silencio");
}

static void test_wrap(void) {
	VizTap_Reset(&tap);

	// Bastante mas que la capacidad: el buffer da varias vueltas.
	for (int g = 0; g < 40; g++)
		write_stereo(g * GRAIN % 30000);

	int ok = VizTap_Read(&tap, 3 * GRAIN, out, 2048, NULL);
	CHECK(ok, "deberia leer despues de varias vueltas");

	// El frame absoluto n vale (n / GRAIN * GRAIN % 30000) + n % GRAIN.
	int good = 1;
	unsigned int end = 40 * GRAIN - 3 * GRAIN;
	for (unsigned int i = 0; i < 2048; i++) {
		unsigned int n = end - 2048 + i;
		int expect = (int)((n / GRAIN) * GRAIN % 30000 + n % GRAIN);
		good &= out[i * 2] == (int16_t)expect;
	}
	CHECK(good, "la ventana deberia ser continua a traves de la vuelta");
}

static void test_overrun_discards(void) {
	VizTap_Reset(&tap);
	for (int g = 0; g < 10; g++)
		write_stereo(g);

	// Un escritor que ya empezo a escribir sobre la ventana: lo que se
	// lee puede estar mezclado y se descarta.
	uint32_t written = tap.written;
	unsigned int delay = 2 * GRAIN;
	uint32_t start = written - delay - 2048;

	tap.begun = start + VIZ_TAP_FRAMES + 1;
	CHECK(!VizTap_Read(&tap, delay, out, 2048, NULL), "un escritor que alcanzo la ventana deberia descartarla");

	tap.begun = start + VIZ_TAP_FRAMES;
	CHECK(VizTap_Read(&tap, delay, out, 2048, NULL), "un escritor justo antes de la ventana no deberia descartarla");

	// El escritor normal, un grain adelante de lo publicado, nunca la alcanza.
	tap.begun = written + GRAIN;
	CHECK(VizTap_Read(&tap, delay, out, 2048, NULL), "un escritor un grain adelante no deberia descartar");
}

static void test_too_long(void) {
	VizTap_Reset(&tap);
	CHECK(!VizTap_Read(&tap, 0, out, VIZ_TAP_FRAMES + 1, NULL), "una ventana mayor que el buffer deberia rechazarse");

	// Una escritura mayor que el buffer guarda solo su final.
	static int16_t big[(VIZ_TAP_FRAMES + 100) * 2];
	for (int i = 0; i < VIZ_TAP_FRAMES + 100; i++) {
		big[i * 2] = (int16_t)(i % 30000);
		big[i * 2 + 1] = 0;
	}
	VizTap_Write(&tap, big, VIZ_TAP_FRAMES + 100, 2, 48000);
	CHECK(tap.written == VIZ_TAP_FRAMES, "deberia contar solo lo guardado, dio %u", tap.written);
	VizTap_Read(&tap, 0, out, 1, NULL);
	CHECK(out[0] == (VIZ_TAP_FRAMES + 99) % 30000, "el ultimo frame deberia ser el final de la escritura, dio %d", out[0]);
}

static void test_reset(void) {
	VizTap_Reset(&tap);
	write_stereo(100);
	VizTap_Reset(&tap);

	unsigned int rate = 1;
	VizTap_Read(&tap, 0, out, 1024, &rate);
	int zeros = 1;
	for (int i = 0; i < 2048; i++)
		zeros &= out[i] == 0;
	CHECK(zeros && rate == 0, "despues de reiniciar no deberia quedar nada de la pista anterior");
}

int main(void) {
	test_empty();
	test_stereo();
	test_mono();
	test_bad_channels();
	test_delay();
	test_delay_before_start();
	test_wrap();
	test_overrun_discards();
	test_too_long();
	test_reset();

	if (failures == 0)
		printf("test_viz_tap: todo bien\n");

	return failures == 0 ? 0 : 1;
}
