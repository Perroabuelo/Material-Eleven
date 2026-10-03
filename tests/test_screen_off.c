// Pruebas en PC del filtro de START (source/screen_off.c), compiladas con el gcc
// del host. screen_off.c es logica pura, asi que no hace falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>

#include "screen_off.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// Las mascaras de psp2/ctrl.h, repetidas para no depender de los headers de la
// consola. El filtro solo las compara, asi que cualquier bit distinto serviria.
#define BTN_START    0x00000008
#define BTN_LTRIGGER 0x00000100
#define BTN_RTRIGGER 0x00000200
#define BTN_CROSS    0x00004000
#define BTN_UP       0x00000010

static void fresh(ScreenOff_State *s) {
	ScreenOff_Init(s, BTN_START, BTN_LTRIGGER, BTN_RTRIGGER);
}

static void test_start_turns_off(void) {
	ScreenOff_State s; int off = -1;
	fresh(&s);

	unsigned int out = ScreenOff_Filter(&s, BTN_START, BTN_START, &off);
	CHECK(off == 1, "START solo deberia pedir apagar la pantalla");
	CHECK(out == 0, "START deberia quitarse del flanco, quedo 0x%x", out);
	CHECK(s.armed == 1, "despues de apagar el filtro deberia quedar armado");

	// START junto con otro boton: el otro boton pasa.
	fresh(&s);
	out = ScreenOff_Filter(&s, BTN_START | BTN_UP, BTN_START | BTN_UP, &off);
	CHECK(off == 1, "START con arriba deberia pedir apagar");
	CHECK(out == BTN_UP, "solo START deberia quitarse del flanco, quedo 0x%x", out);

	// START con un solo gatillo mantenido sigue siendo apagar.
	fresh(&s);
	out = ScreenOff_Filter(&s, BTN_START, BTN_START | BTN_LTRIGGER, &off);
	CHECK(off == 1, "START con solo L mantenido deberia apagar");
}

static void test_combo_passes(void) {
	ScreenOff_State s; int off = -1;
	fresh(&s);

	unsigned int held = BTN_LTRIGGER | BTN_RTRIGGER | BTN_START;
	unsigned int out = ScreenOff_Filter(&s, BTN_START, held, &off);
	CHECK(off == 0, "L + R + START no deberia apagar la pantalla");
	CHECK(out == BTN_START, "L + R + START deberia pasar intacto, quedo 0x%x", out);
	CHECK(s.armed == 0, "L + R + START no deberia armar el filtro");
}

static void test_disabled(void) {
	ScreenOff_State s; int off = -1;
	fresh(&s);
	s.enabled = 0;

	unsigned int out = ScreenOff_Filter(&s, BTN_START, BTN_START, &off);
	CHECK(off == 0, "deshabilitado no deberia apagar");
	CHECK(out == BTN_START, "deshabilitado el flanco deberia pasar intacto, quedo 0x%x", out);
	CHECK(s.armed == 0, "deshabilitado no deberia armarse");

	// Armado y luego deshabilitado: tampoco descarta nada.
	fresh(&s);
	s.armed = 1;
	s.enabled = 0;
	out = ScreenOff_Filter(&s, BTN_CROSS, BTN_CROSS, &off);
	CHECK(out == BTN_CROSS, "deshabilitado no deberia descartar el despertar, quedo 0x%x", out);
}

static void test_wake_press_is_dropped(void) {
	ScreenOff_State s; int off = -1;
	fresh(&s);
	ScreenOff_Filter(&s, BTN_START, BTN_START, &off);

	// Fotogramas sin pulsaciones nuevas mientras la pantalla esta apagada.
	unsigned int out = ScreenOff_Filter(&s, 0, 0, &off);
	CHECK(out == 0 && s.armed == 1, "sin pulsacion el filtro deberia seguir armado");

	out = ScreenOff_Filter(&s, BTN_CROSS, BTN_CROSS, &off);
	CHECK(out == 0, "la pulsacion que enciende la pantalla deberia descartarse, quedo 0x%x", out);
	CHECK(off == 0, "encender no deberia pedir apagar");
	CHECK(s.armed == 0, "despues de encender el filtro deberia desarmarse");

	out = ScreenOff_Filter(&s, BTN_CROSS, BTN_CROSS, &off);
	CHECK(out == BTN_CROSS, "la pulsacion siguiente deberia pasar, quedo 0x%x", out);

	// Encender con START no vuelve a apagar.
	fresh(&s);
	ScreenOff_Filter(&s, BTN_START, BTN_START, &off);
	out = ScreenOff_Filter(&s, BTN_START, BTN_START, &off);
	CHECK(out == 0 && off == 0, "encender con START no deberia volver a apagar");
}

static void test_other_presses_pass(void) {
	ScreenOff_State s; int off = -1;
	fresh(&s);

	unsigned int out = ScreenOff_Filter(&s, BTN_CROSS | BTN_UP, BTN_CROSS | BTN_UP, &off);
	CHECK(out == (BTN_CROSS | BTN_UP), "un flanco sin START deberia pasar intacto, quedo 0x%x", out);
	CHECK(off == 0 && s.armed == 0, "un flanco sin START no deberia apagar ni armar");
}

int main(void) {
	test_start_turns_off();
	test_combo_passes();
	test_disabled();
	test_wake_press_is_dropped();
	test_other_presses_pass();

	if (failures == 0)
		printf("test_screen_off: todo bien\n");

	return failures == 0 ? 0 : 1;
}
