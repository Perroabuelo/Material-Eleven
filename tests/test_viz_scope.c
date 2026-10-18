// Pruebas en PC del disparo del osciloscopio (source/viz_scope.c), compiladas
// con el gcc del host.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <math.h>
#include <stdio.h>

#include "viz_scope.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

static float win[VIZ_SCOPE_WINDOW];

// La ventana de una senoide de `hz` que empieza en el frame absoluto `from`.
static void sine_window(float hz, unsigned int rate, long from) {
	for (int i = 0; i < VIZ_SCOPE_WINDOW; i++)
		win[i] = 0.6f * sinf(2.0f * (float)M_PI * hz * (float)(from + i) / rate);
}

static void test_still_wave(void) {
	static float first[VIZ_SCOPE_SPAN];

	// Ventanas sucesivas como las de cada fotograma (unos 800 frames entre
	// una y otra a 48 kHz y 60 fps), con un tono que no cae en un ciclo
	// entero por fotograma.
	for (int f = 0; f < 10; f++) {
		sine_window(440.0f, 48000, f * 800L + 37);
		unsigned int t = VizScope_Trigger(win, VIZ_SCOPE_WINDOW, VIZ_SCOPE_SPAN);

		CHECK(t > 0 && t <= VIZ_SCOPE_WINDOW / 2, "deberia encontrar un cruce en la primera mitad, dio %u", t);
		CHECK(win[t - 1] < 0.0f && win[t] >= 0.0f, "el disparo deberia ser un cruce ascendente");

		if (f == 0) {
			for (int i = 0; i < VIZ_SCOPE_SPAN; i++)
				first[i] = win[t + i];
			continue;
		}

		// La misma fase: el tramo dibujado casi no cambia. Un cruce cae
		// entre dos muestras, asi que la diferencia es de a lo sumo un
		// paso de muestra de la senoide.
		float worst = 0.0f;
		for (int i = 0; i < VIZ_SCOPE_SPAN; i++) {
			float d = fabsf(win[t + i] - first[i]);
			if (d > worst)
				worst = d;
		}
		float step = 0.6f * 2.0f * (float)M_PI * 440.0f / 48000;
		CHECK(worst <= step * 1.01f, "fotograma %d: la onda se desplazo %f (un paso es %f)", f, worst, step);
	}
}

static void test_no_crossing(void) {
	for (int i = 0; i < VIZ_SCOPE_WINDOW; i++)
		win[i] = 0.0f;
	CHECK(VizScope_Trigger(win, VIZ_SCOPE_WINDOW, VIZ_SCOPE_SPAN) == 0, "el silencio deberia dibujarse desde el principio");

	for (int i = 0; i < VIZ_SCOPE_WINDOW; i++)
		win[i] = -0.3f;
	CHECK(VizScope_Trigger(win, VIZ_SCOPE_WINDOW, VIZ_SCOPE_SPAN) == 0, "sin cruce deberia dibujarse desde el principio");

	// Un unico cruce en la segunda mitad no cuenta.
	win[1500] = 0.5f;
	CHECK(VizScope_Trigger(win, VIZ_SCOPE_WINDOW, VIZ_SCOPE_SPAN) == 0, "un cruce en la segunda mitad no deberia contar");

	// Un cruce descendente tampoco.
	for (int i = 0; i < VIZ_SCOPE_WINDOW; i++)
		win[i] = i < 100 ? 0.5f : -0.5f;
	CHECK(VizScope_Trigger(win, VIZ_SCOPE_WINDOW, VIZ_SCOPE_SPAN) == 0, "un cruce descendente no deberia contar");
}

static void test_span_fits(void) {
	// Ventana corta: el tramo siempre entra despues del disparo.
	for (int i = 0; i < 600; i++)
		win[i] = i == 200 ? 0.5f : -0.5f;
	CHECK(VizScope_Trigger(win, 600, VIZ_SCOPE_SPAN) == 0, "un cruce sin lugar para el tramo no deberia contar");
	CHECK(VizScope_Trigger(win, 100, VIZ_SCOPE_SPAN) == 0, "una ventana menor que el tramo deberia dar 0");

	win[50] = 0.5f;
	CHECK(VizScope_Trigger(win, 600, VIZ_SCOPE_SPAN) == 50, "un cruce con lugar deberia contar");
}

int main(void) {
	test_still_wave();
	test_no_crossing();
	test_span_fits();

	if (failures == 0)
		printf("test_viz_scope: todo bien\n");

	return failures == 0 ? 0 : 1;
}
