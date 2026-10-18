// Pruebas en PC de los medidores del visualizador (source/viz_meter.c),
// compiladas con el gcc del host.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <math.h>
#include <stdio.h>

#include "viz_meter.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// Corre `seconds` a `fps` con el objetivo fijo.
static void run(VizMeter *m, float target, float seconds, int fps) {
	int frames = (int)lroundf(seconds * fps);
	for (int i = 0; i < frames; i++)
		VizMeter_Update(m, target, 1.0f / fps);
}

static void test_attack(void) {
	VizMeter m;
	VizMeter_Reset(&m);

	VizMeter_Update(&m, 0.7f, 1.0f / 60);
	CHECK(m.level == 0.7f, "deberia subir de inmediato, dio %f", m.level);
	CHECK(m.peak == 0.7f, "el pico deberia subir con el nivel, dio %f", m.peak);

	VizMeter_Update(&m, 2.0f, 1.0f / 60);
	CHECK(m.level == 1.0f, "deberia acotar a 1, dio %f", m.level);
	VizMeter_Update(&m, -1.0f, 0.0f);
	CHECK(m.level == 1.0f, "sin tiempo no deberia caer, dio %f", m.level);
}

static void test_fall_independent_of_fps(void) {
	VizMeter a, b;
	VizMeter_Reset(&a);
	VizMeter_Reset(&b);

	VizMeter_Update(&a, 1.0f, 1.0f / 60);
	VizMeter_Update(&b, 1.0f, 1.0f / 30);

	run(&a, 0.0f, 1.0f, 60);
	run(&b, 0.0f, 1.0f, 30);

	CHECK(fabsf(a.level - b.level) < 1e-4f, "a 60 y a 30 fps deberia caer igual en 1 s: %f y %f", a.level, b.level);
	CHECK(a.level > 0.0f && a.level < 0.1f, "en 1 s deberia haber caido casi del todo, quedo %f", a.level);
	CHECK(fabsf(a.peak - b.peak) < 0.03f, "el pico deberia caer igual a 60 y 30 fps: %f y %f", a.peak, b.peak);
}

static void test_falls_to_zero(void) {
	VizMeter m;
	VizMeter_Reset(&m);
	VizMeter_Update(&m, 1.0f, 1.0f / 60);

	// La pausa: dos segundos de silencio dejan todo en cero, sin congelarse.
	float prev = m.level;
	int continuous = 1;
	for (int i = 0; i < 120; i++) {
		VizMeter_Update(&m, 0.0f, 1.0f / 60);
		continuous &= m.level < prev && prev - m.level < 0.1f;
		prev = m.level;
	}
	CHECK(continuous, "deberia caer de forma continua");
	CHECK(m.level < 0.001f, "en 2 s deberia llegar a cero, quedo %f", m.level);
	CHECK(m.peak < 0.001f, "el pico tambien deberia llegar a cero, quedo %f", m.peak);
}

static void test_peak_hold(void) {
	VizMeter m;
	VizMeter_Reset(&m);
	VizMeter_Update(&m, 0.9f, 1.0f / 60);

	run(&m, 0.0f, 0.7f, 60);
	CHECK(m.peak == 0.9f, "a los 0,7 s el pico deberia seguir en 0,9, esta en %f", m.peak);

	run(&m, 0.0f, 0.2f, 60);
	CHECK(m.peak < 0.9f, "pasados 0,8 s el pico deberia empezar a caer, esta en %f", m.peak);
	CHECK(m.peak >= m.level, "el pico no deberia quedar bajo el nivel");

	// Un golpe nuevo sobre el pico lo sube y reinicia la retencion.
	VizMeter_Update(&m, 0.95f, 1.0f / 60);
	run(&m, 0.0f, 0.5f, 60);
	CHECK(m.peak == 0.95f, "un golpe nuevo deberia reiniciar la retencion, esta en %f", m.peak);
}

static void test_rms(void) {
	static int16_t st[2400 * 2];

	// Tono de amplitud completa en L, silencio en R.
	for (int i = 0; i < 2400; i++) {
		st[i * 2] = (int16_t)(32767 * sin(2 * M_PI * 440 * i / 48000.0));
		st[i * 2 + 1] = 0;
	}

	float l = VizMeter_RmsDb(st, 2400, 0);
	float r = VizMeter_RmsDb(st, 2400, 1);
	CHECK(fabsf(l + 3.01f) < 0.1f, "un seno a escala completa deberia dar -3 dB, dio %f", l);
	CHECK(r == VIZ_VU_FLOOR_DB, "un canal en silencio deberia dar el piso, dio %f", r);
	CHECK(VizMeter_RmsDb(st, 0, 0) == VIZ_VU_FLOOR_DB, "sin frames deberia dar el piso");

	// Mono convertido a estereo: los dos canales iguales.
	for (int i = 0; i < 2400; i++)
		st[i * 2 + 1] = st[i * 2];
	CHECK(VizMeter_RmsDb(st, 2400, 0) == VizMeter_RmsDb(st, 2400, 1), "mono deberia dar el mismo nivel en L y R");

	// -24 dB queda a mitad de la escala: la musica normal no queda pegada abajo.
	CHECK(fabsf(VizMeter_VuNorm(-24.0f) - 0.5f) < 1e-6f, "-24 dB deberia ser la mitad");
	CHECK(VizMeter_VuNorm(VIZ_VU_FLOOR_DB) == 0.0f && VizMeter_VuNorm(0.0f) == 1.0f, "los extremos deberian ser 0 y 1");
}

int main(void) {
	test_attack();
	test_fall_independent_of_fps();
	test_falls_to_zero();
	test_peak_hold();
	test_rms();

	if (failures == 0)
		printf("test_viz_meter: todo bien\n");

	return failures == 0 ? 0 : 1;
}
