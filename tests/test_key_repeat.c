// Pruebas en PC de la repeticion de arriba/abajo (source/key_repeat.c),
// compiladas con el gcc del host. key_repeat.c es logica pura, asi que no hace
// falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla. Los tiempos salen de las constantes del header, no de numeros
// literales, para poder ajustar el ritmo en consola sin tocar las pruebas.

#include <stdio.h>

#include "key_repeat.h"

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
// consola.
#define BTN_UP    0x00000010
#define BTN_DOWN  0x00000040
#define BTN_CROSS 0x00004000

// Un fotograma a 60 fps, en microsegundos.
#define FRAME_US 16667ULL

static void fresh(KeyRepeat_State *s) {
	KeyRepeat_Init(s, BTN_UP, BTN_DOWN);
}

// Simula mantener `held` desde `*now` (exclusivo) hasta `until` (inclusivo), a
// 60 fps, y cuenta los pasos repetidos y los de pagina. Deja *now en `until`.
static void hold_until(KeyRepeat_State *s, unsigned int held, unsigned long long *now,
	unsigned long long until, int *steps, int *pages) {
	*steps = 0;
	*pages = 0;
	while (*now + FRAME_US <= until) {
		*now += FRAME_US;
		KeyRepeat_Step st = KeyRepeat_Update(s, held, 0, *now);
		if (st.dir != 0) {
			(*steps)++;
			if (st.page)
				(*pages)++;
		}
	}
}

static void test_press_steps_once(void) {
	KeyRepeat_State s; fresh(&s);
	unsigned long long now = 1000000;

	KeyRepeat_Step st = KeyRepeat_Update(&s, BTN_DOWN, BTN_DOWN, now);
	CHECK(st.dir == 1 && !st.repeat && !st.page, "pulsar abajo deberia dar un paso nuevo hacia abajo (dir %d repeat %d)", st.dir, st.repeat);

	st = KeyRepeat_Update(&s, BTN_UP, BTN_UP, now + 5 * FRAME_US);
	CHECK(st.dir == -1 && !st.repeat, "pulsar arriba deberia dar un paso nuevo hacia arriba");

	// Nada mas hasta cumplirse el retardo.
	fresh(&s);
	now = 1000000;
	KeyRepeat_Update(&s, BTN_DOWN, BTN_DOWN, now);
	int steps, pages;
	hold_until(&s, BTN_DOWN, &now, 1000000 + KEY_REPEAT_DELAY_US - 1, &steps, &pages);
	CHECK(steps == 0, "antes del retardo no deberia haber pasos repetidos, hubo %d", steps);

	// Un toque corto: pulsar, mantener menos del retardo y soltar.
	KeyRepeat_Step rel = KeyRepeat_Update(&s, 0, 0, now + FRAME_US);
	CHECK(rel.dir == 0, "soltar no deberia dar paso");
}

static void test_repeat_after_delay(void) {
	KeyRepeat_State s; fresh(&s);
	unsigned long long t0 = 1000000, now = t0;

	KeyRepeat_Update(&s, BTN_DOWN, BTN_DOWN, now);

	// Primer paso repetido justo al cumplirse el retardo.
	KeyRepeat_Step st = KeyRepeat_Update(&s, BTN_DOWN, 0, t0 + KEY_REPEAT_DELAY_US);
	CHECK(st.dir == 1 && st.repeat && !st.page, "al cumplirse el retardo deberia haber un paso repetido de una fila");
	now = t0 + KEY_REPEAT_DELAY_US;

	// Luego uno por intervalo: no antes, si al cumplirse.
	st = KeyRepeat_Update(&s, BTN_DOWN, 0, now + KEY_REPEAT_INTERVAL_US - 1);
	CHECK(st.dir == 0, "antes del intervalo no deberia haber otro paso");
	st = KeyRepeat_Update(&s, BTN_DOWN, 0, now + KEY_REPEAT_INTERVAL_US);
	CHECK(st.dir == 1 && st.repeat, "al cumplirse el intervalo deberia haber otro paso");

	// Mantener 1,5 s a 60 fps: entre 10 y 25 pasos en total (escenario de la spec).
	fresh(&s);
	now = t0;
	KeyRepeat_Step first = KeyRepeat_Update(&s, BTN_DOWN, BTN_DOWN, now);
	int steps, pages;
	hold_until(&s, BTN_DOWN, &now, t0 + 1500000, &steps, &pages);
	int total = steps + (first.dir != 0);
	CHECK(total >= 10 && total <= 25, "mantener 1,5 s deberia dar entre 10 y 25 pasos, dio %d", total);
	CHECK(pages == 0, "en 1,5 s no deberia haber pasos de pagina, hubo %d", pages);
}

static void test_pages_after_threshold(void) {
	KeyRepeat_State s; fresh(&s);
	unsigned long long t0 = 1000000, now = t0;
	int steps, pages;

	KeyRepeat_Update(&s, BTN_DOWN, BTN_DOWN, now);
	hold_until(&s, BTN_DOWN, &now, t0 + KEY_REPEAT_PAGE_US - 1, &steps, &pages);
	CHECK(steps > 0 && pages == 0, "antes del umbral los pasos deberian ser de una fila (pasos %d, paginas %d)", steps, pages);

	hold_until(&s, BTN_DOWN, &now, t0 + KEY_REPEAT_PAGE_US + 1000000, &steps, &pages);
	CHECK(steps > 0 && pages == steps, "pasado el umbral todos los pasos deberian ser de pagina (pasos %d, paginas %d)", steps, pages);
}

static void test_release_resets(void) {
	KeyRepeat_State s; fresh(&s);
	unsigned long long t0 = 1000000, now = t0;
	int steps, pages;

	KeyRepeat_Update(&s, BTN_DOWN, BTN_DOWN, now);
	hold_until(&s, BTN_DOWN, &now, t0 + KEY_REPEAT_PAGE_US + 500000, &steps, &pages);
	CHECK(pages > 0, "deberia haber llegado a saltar por paginas");

	now += FRAME_US;
	KeyRepeat_Update(&s, 0, 0, now);

	// Volver a pulsar: un paso nuevo de una fila, y el retardo empieza de cero.
	now += FRAME_US;
	unsigned long long t1 = now;
	KeyRepeat_Step st = KeyRepeat_Update(&s, BTN_DOWN, BTN_DOWN, now);
	CHECK(st.dir == 1 && !st.repeat && !st.page, "volver a pulsar deberia dar un paso nuevo de una fila");
	hold_until(&s, BTN_DOWN, &now, t1 + KEY_REPEAT_DELAY_US - 1, &steps, &pages);
	CHECK(steps == 0, "tras volver a pulsar, el retardo deberia empezar de cero (hubo %d pasos)", steps);
	hold_until(&s, BTN_DOWN, &now, t1 + KEY_REPEAT_PAGE_US - 1, &steps, &pages);
	CHECK(steps > 0 && pages == 0, "tras volver a pulsar, los pasos deberian volver a ser de una fila (paginas %d)", pages);
}

static void test_both_and_direction_change(void) {
	KeyRepeat_State s; fresh(&s);
	unsigned long long t0 = 1000000, now = t0;
	int steps, pages;

	// Arriba y abajo a la vez: el flanco pasa, pero no hay repeticion.
	KeyRepeat_Step st = KeyRepeat_Update(&s, BTN_UP | BTN_DOWN, BTN_UP | BTN_DOWN, now);
	CHECK(st.dir == -1 && !st.repeat, "el flanco con los dos deberia ser arriba, como siempre");
	hold_until(&s, BTN_UP | BTN_DOWN, &now, t0 + 3000000, &steps, &pages);
	CHECK(steps == 0, "arriba y abajo a la vez no deberian repetir, hubo %d pasos", steps);

	// Soltar arriba y quedarse con abajo cuenta como cambio de direccion: el
	// retardo empieza ahi.
	now += FRAME_US;
	unsigned long long t1 = now;
	st = KeyRepeat_Update(&s, BTN_DOWN, 0, now);
	CHECK(st.dir == 0, "quedarse con un solo boton no deberia dar un paso por si solo");
	hold_until(&s, BTN_DOWN, &now, t1 + KEY_REPEAT_DELAY_US - 1, &steps, &pages);
	CHECK(steps == 0, "el retardo deberia contar desde el cambio, hubo %d pasos", steps);

	// Cambiar de direccion con una pulsacion nueva reinicia el retardo.
	fresh(&s);
	now = t0;
	KeyRepeat_Update(&s, BTN_DOWN, BTN_DOWN, now);
	hold_until(&s, BTN_DOWN, &now, t0 + 1000000, &steps, &pages);
	now += FRAME_US;
	KeyRepeat_Update(&s, 0, 0, now);
	now += FRAME_US;
	unsigned long long t2 = now;
	st = KeyRepeat_Update(&s, BTN_UP, BTN_UP, now);
	CHECK(st.dir == -1 && !st.repeat, "pulsar arriba deberia dar un paso nuevo hacia arriba");
	hold_until(&s, BTN_UP, &now, t2 + KEY_REPEAT_DELAY_US - 1, &steps, &pages);
	CHECK(steps == 0, "cambiar de direccion deberia reiniciar el retardo, hubo %d pasos", steps);
	st = KeyRepeat_Update(&s, BTN_UP, 0, t2 + KEY_REPEAT_DELAY_US);
	CHECK(st.dir == -1 && st.repeat, "la repeticion deberia seguir hacia arriba");

	// Otros botones no afectan.
	fresh(&s);
	st = KeyRepeat_Update(&s, BTN_CROSS, BTN_CROSS, t0);
	CHECK(st.dir == 0, "confirmar no deberia dar paso");
	st = KeyRepeat_Update(&s, BTN_CROSS, 0, t0 + 3000000);
	CHECK(st.dir == 0, "mantener confirmar no deberia repetir");
}

static void test_slow_frame_single_step(void) {
	KeyRepeat_State s; fresh(&s);
	unsigned long long t0 = 1000000;

	KeyRepeat_Update(&s, BTN_DOWN, BTN_DOWN, t0);
	KeyRepeat_Step st = KeyRepeat_Update(&s, BTN_DOWN, 0, t0 + KEY_REPEAT_DELAY_US);
	CHECK(st.dir == 1, "deberia haber un primer paso repetido");

	// Un fotograma de 1 s: un solo paso, y el siguiente se programa desde ahi.
	unsigned long long slow = t0 + KEY_REPEAT_DELAY_US + 1000000;
	st = KeyRepeat_Update(&s, BTN_DOWN, 0, slow);
	CHECK(st.dir == 1 && st.repeat, "tras un fotograma lento deberia haber un paso");
	st = KeyRepeat_Update(&s, BTN_DOWN, 0, slow + FRAME_US);
	CHECK(st.dir == 0, "el fotograma siguiente no deberia ponerse al dia con otro paso");
	st = KeyRepeat_Update(&s, BTN_DOWN, 0, slow + KEY_REPEAT_INTERVAL_US);
	CHECK(st.dir == 1, "el siguiente paso deberia llegar un intervalo despues del fotograma lento");
}

static KeyRepeat_Step mk(int dir, int repeat, int page) {
	KeyRepeat_Step st = { dir, repeat, page };
	return st;
}

static void test_move(void) {
	// Pulsacion nueva: una fila, y da la vuelta en los bordes.
	CHECK(KeyRepeat_Move(0, 10, mk(1, 0, 0), 5) == 1, "abajo desde 0 deberia ir a 1");
	CHECK(KeyRepeat_Move(9, 10, mk(1, 0, 0), 5) == 0, "abajo desde la ultima deberia dar la vuelta a 0");
	CHECK(KeyRepeat_Move(0, 10, mk(-1, 0, 0), 5) == 9, "arriba desde 0 deberia dar la vuelta a la ultima");

	// Paso repetido: una fila, y se recorta en los bordes.
	CHECK(KeyRepeat_Move(3, 10, mk(1, 1, 0), 5) == 4, "repetido abajo desde 3 deberia ir a 4");
	CHECK(KeyRepeat_Move(9, 10, mk(1, 1, 0), 5) == 9, "repetido abajo desde la ultima deberia quedarse");
	CHECK(KeyRepeat_Move(0, 10, mk(-1, 1, 0), 5) == 0, "repetido arriba desde 0 deberia quedarse");

	// Paso de pagina: `page` filas, y se recorta en los bordes.
	CHECK(KeyRepeat_Move(0, 10, mk(1, 1, 1), 5) == 5, "pagina abajo desde 0 deberia ir a 5");
	CHECK(KeyRepeat_Move(7, 10, mk(1, 1, 1), 5) == 9, "pagina abajo desde 7 deberia quedar en la ultima");
	CHECK(KeyRepeat_Move(2, 10, mk(-1, 1, 1), 5) == 0, "pagina arriba desde 2 deberia quedar en 0");
	CHECK(KeyRepeat_Move(3, 10, mk(1, 1, 1), 0) == 4, "una pagina de 0 filas deberia mover una");

	// Sin paso: no se mueve, salvo corregir una seleccion fuera de la lista.
	CHECK(KeyRepeat_Move(4, 10, mk(0, 0, 0), 5) == 4, "sin paso no deberia moverse");
	CHECK(KeyRepeat_Move(12, 10, mk(0, 0, 0), 5) == 0, "una seleccion pasada del final deberia volver a 0");
	CHECK(KeyRepeat_Move(-1, 10, mk(0, 0, 0), 5) == 9, "una seleccion antes del inicio deberia ir a la ultima");

	// count 0 y 1.
	CHECK(KeyRepeat_Move(3, 0, mk(1, 0, 0), 5) == 0, "con count 0 deberia dar 0");
	CHECK(KeyRepeat_Move(0, 0, mk(-1, 1, 1), 5) == 0, "con count 0 deberia dar 0 tambien en paginas");
	CHECK(KeyRepeat_Move(0, -2, mk(0, 0, 0), 5) == 0, "con count negativo deberia dar 0");
	CHECK(KeyRepeat_Move(0, 1, mk(1, 0, 0), 5) == 0, "con count 1 abajo deberia quedarse en 0");
	CHECK(KeyRepeat_Move(0, 1, mk(-1, 0, 0), 5) == 0, "con count 1 arriba deberia quedarse en 0");
	CHECK(KeyRepeat_Move(0, 1, mk(1, 1, 1), 5) == 0, "con count 1 una pagina deberia quedarse en 0");
}

// Recorre una lista de 500 filas manteniendo abajo 5 s, como el escenario
// "Cruzar una lista larga": debe terminar mas de 100 filas abajo.
static void test_cross_long_list(void) {
	KeyRepeat_State s; fresh(&s);
	unsigned long long t0 = 1000000, now = t0;
	int sel = 0;

	sel = KeyRepeat_Move(sel, 500, KeyRepeat_Update(&s, BTN_DOWN, BTN_DOWN, now), 5);
	while (now < t0 + 5000000) {
		now += FRAME_US;
		sel = KeyRepeat_Move(sel, 500, KeyRepeat_Update(&s, BTN_DOWN, 0, now), 5);
	}
	CHECK(sel > 100, "mantener abajo 5 s deberia pasar de la fila 100, quedo en %d", sel);

	// Mantenerlo hasta el final: se queda en la ultima, no da la vuelta.
	while (now < t0 + 60000000) {
		now += FRAME_US;
		sel = KeyRepeat_Move(sel, 500, KeyRepeat_Update(&s, BTN_DOWN, 0, now), 5);
	}
	CHECK(sel == 499, "mantener abajo deberia detenerse en la ultima, quedo en %d", sel);

	// Soltar y pulsar una vez da la vuelta.
	now += FRAME_US;
	sel = KeyRepeat_Move(sel, 500, KeyRepeat_Update(&s, 0, 0, now), 5);
	now += FRAME_US;
	sel = KeyRepeat_Move(sel, 500, KeyRepeat_Update(&s, BTN_DOWN, BTN_DOWN, now), 5);
	CHECK(sel == 0, "una pulsacion nueva en el borde deberia dar la vuelta, quedo en %d", sel);
}

int main(void) {
	test_press_steps_once();
	test_repeat_after_delay();
	test_pages_after_threshold();
	test_release_resets();
	test_both_and_direction_change();
	test_slow_frame_single_step();
	test_move();
	test_cross_long_list();

	if (failures == 0)
		printf("test_key_repeat: todo bien\n");

	return failures == 0 ? 0 : 1;
}
