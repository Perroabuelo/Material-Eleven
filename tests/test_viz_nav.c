// Pruebas en PC de las vistas y los estilos de Reproduciendo
// (source/viz_nav.c), compiladas con el gcc del host. Cada caso sigue un
// escenario de specs/ui/visualizer o specs/ui/now-playing.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <string.h>

#include "viz_nav.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// Lo que la app tiene al arrancar.
static NowPlaying_View view;
static Viz_Style style;

static void start(void) {
	view = NP_VIEW_NORMAL;
	style = VIZ_STYLE_SPECTRUM;
}

static void press(VizNav_Key key) {
	VizNav_Apply(&view, &style, key);
}

static void test_open_close_visualizer(void) {
	start();
	press(VIZ_NAV_DOWN);
	CHECK(view == NP_VIEW_VISUALIZER && style == VIZ_STYLE_SPECTRUM, "abajo deberia abrir el visualizador en Espectro");
	press(VIZ_NAV_DOWN);
	CHECK(view == NP_VIEW_NORMAL, "abajo otra vez deberia volver a la vista normal");
}

static void test_open_close_lyrics(void) {
	start();
	press(VIZ_NAV_UP);
	CHECK(view == NP_VIEW_LYRICS, "arriba deberia abrir las letras");
	press(VIZ_NAV_UP);
	CHECK(view == NP_VIEW_NORMAL, "arriba otra vez deberia volver a la vista normal");
}

static void test_jump_between(void) {
	start();
	press(VIZ_NAV_UP);
	press(VIZ_NAV_DOWN);
	CHECK(view == NP_VIEW_VISUALIZER, "abajo desde las letras deberia abrir el visualizador");
	press(VIZ_NAV_UP);
	CHECK(view == NP_VIEW_LYRICS, "arriba desde el visualizador deberia abrir las letras");

	// Cerrar el visualizador al que se llego desde las letras.
	press(VIZ_NAV_DOWN);
	press(VIZ_NAV_DOWN);
	CHECK(view == NP_VIEW_NORMAL, "cerrar el visualizador deberia dar la vista normal, no las letras");
}

static void test_walk_forward(void) {
	const Viz_Style order[] = { VIZ_STYLE_SCOPE, VIZ_STYLE_VU, VIZ_STYLE_DISC, VIZ_STYLE_PULSE, VIZ_STYLE_SPECTRUM };

	start();
	press(VIZ_NAV_DOWN);
	for (int i = 0; i < 5; i++) {
		press(VIZ_NAV_RIGHT);
		CHECK(style == order[i], "derecha %d deberia dar %s, dio %s", i + 1, VizNav_StyleName(order[i]), VizNav_StyleName(style));
		CHECK(view == NP_VIEW_VISUALIZER, "cambiar de estilo no deberia cambiar la vista");
	}
}

static void test_back_from_first(void) {
	start();
	press(VIZ_NAV_DOWN);
	press(VIZ_NAV_LEFT);
	CHECK(style == VIZ_STYLE_PULSE, "izquierda desde Espectro deberia dar Disco que respira, dio %s", VizNav_StyleName(style));
	press(VIZ_NAV_RIGHT);
	CHECK(style == VIZ_STYLE_SPECTRUM, "derecha desde el ultimo deberia volver a Espectro");
}

static void test_arrows_outside(void) {
	start();
	press(VIZ_NAV_RIGHT);
	CHECK(view == NP_VIEW_NORMAL && style == VIZ_STYLE_SPECTRUM, "las flechas laterales no deberian hacer nada en la vista normal");
	press(VIZ_NAV_UP);
	press(VIZ_NAV_LEFT);
	CHECK(view == NP_VIEW_LYRICS && style == VIZ_STYLE_SPECTRUM, "las flechas laterales no deberian hacer nada en las letras");
	press(VIZ_NAV_DOWN);
	CHECK(view == NP_VIEW_VISUALIZER && style == VIZ_STYLE_SPECTRUM, "el visualizador deberia abrirse en el estilo que tenia");
}

static void test_style_kept_when_closed(void) {
	start();
	press(VIZ_NAV_DOWN);
	press(VIZ_NAV_RIGHT);
	press(VIZ_NAV_DOWN);
	press(VIZ_NAV_DOWN);
	CHECK(view == NP_VIEW_VISUALIZER && style == VIZ_STYLE_SCOPE, "el estilo deberia recordarse al cerrar y abrir");
}

static void test_debug_log_scenario(void) {
	// specs/dev/debug-log: abajo, derecha dos veces, arriba, arriba.
	static const char *expect[] = {
		"normal spectrum", "visualizer spectrum", "visualizer scope",
		"visualizer vu", "lyrics vu", "normal vu"
	};
	const VizNav_Key keys[] = { VIZ_NAV_DOWN, VIZ_NAV_RIGHT, VIZ_NAV_RIGHT, VIZ_NAV_UP, VIZ_NAV_UP };
	char got[64];

	start();
	for (int i = 0; i < 6; i++) {
		if (i > 0)
			press(keys[i - 1]);
		snprintf(got, sizeof(got), "%s %s", VizNav_ViewName(view), VizNav_StyleName(style));
		CHECK(strcmp(got, expect[i]) == 0, "paso %d deberia ser \"%s\", es \"%s\"", i, expect[i], got);
	}
}

static void test_disc_styles(void) {
	CHECK(!VizNav_StyleHasDisc(VIZ_STYLE_SPECTRUM) && !VizNav_StyleHasDisc(VIZ_STYLE_SCOPE) && !VizNav_StyleHasDisc(VIZ_STYLE_VU),
		"Espectro, Osciloscopio y VU no dibujan el vinilo");
	CHECK(VizNav_StyleHasDisc(VIZ_STYLE_DISC) && VizNav_StyleHasDisc(VIZ_STYLE_PULSE), "los dos estilos de disco dibujan el vinilo");
}

static void test_names(void) {
	CHECK(strcmp(VizNav_ViewName(NP_VIEW_NORMAL), "normal") == 0, "nombre de la vista normal");
	CHECK(strcmp(VizNav_ViewName(NP_VIEW_LYRICS), "lyrics") == 0, "nombre de las letras");
	CHECK(strcmp(VizNav_ViewName(NP_VIEW_VISUALIZER), "visualizer") == 0, "nombre del visualizador");
	CHECK(strcmp(VizNav_StyleName(VIZ_STYLE_DISC), "disc") == 0 && strcmp(VizNav_StyleName(VIZ_STYLE_PULSE), "pulse") == 0, "nombres de los discos");
}

int main(void) {
	test_open_close_visualizer();
	test_open_close_lyrics();
	test_jump_between();
	test_walk_forward();
	test_back_from_first();
	test_arrows_outside();
	test_style_kept_when_closed();
	test_debug_log_scenario();
	test_disc_styles();
	test_names();

	if (failures == 0)
		printf("test_viz_nav: todo bien\n");

	return failures == 0 ? 0 : 1;
}
