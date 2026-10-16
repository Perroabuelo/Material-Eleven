// Pruebas en PC de las marcas del modo seleccion (source/selection.c),
// compiladas con el gcc del host. selection.c es logica pura, asi que no hace
// falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <stddef.h>

#include "selection.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// Static, como en menu_library.c: un ListFilter son 16 KB.
static ListFilter f;
static Selection s;

static int match_none(int index, void *ctx) {
	(void)index;
	(void)ctx;
	return 0;
}

// Una lista de 10 canciones donde "queen" son la 1, la 4 y la 7, y "abba" la
// 2, la 5 y la 8.
static int match_mod3(int index, void *ctx) {
	return index % 3 == *(int *)ctx;
}

static void test_mark_and_count(void) {
	Selection_Clear(&s);
	CHECK(Selection_Count(&s) == 0, "recien limpia no cuenta 0");

	Selection_Toggle(&s, 0);
	Selection_Toggle(&s, 3999);
	CHECK(Selection_Has(&s, 0) && Selection_Has(&s, 3999), "no marco la 0 y la 3999");
	CHECK(Selection_Count(&s) == 2, "con la 0 y la 3999 cuenta %d", Selection_Count(&s));

	Selection_Toggle(&s, -1);
	Selection_Toggle(&s, 4000);
	Selection_Set(&s, 4000, 1);
	Selection_Set(&s, -5, 1);
	CHECK(Selection_Count(&s) == 2, "fuera de rango cambio la cuenta a %d", Selection_Count(&s));
	CHECK(!Selection_Has(&s, -1) && !Selection_Has(&s, 4000), "fuera de rango dice que esta marcada");

	Selection_Set(&s, 0, 1);
	CHECK(Selection_Count(&s) == 2, "marcar una ya marcada cambio la cuenta a %d", Selection_Count(&s));

	Selection_Toggle(&s, 0);
	CHECK(!Selection_Has(&s, 0) && Selection_Count(&s) == 1, "desmarcar la 0 dejo cuenta %d", Selection_Count(&s));

	Selection_Set(&s, 3999, 0);
	Selection_Set(&s, 3999, 0);
	CHECK(!Selection_Has(&s, 3999) && Selection_Count(&s) == 0, "desmarcar dos veces dejo cuenta %d", Selection_Count(&s));

	Selection_Toggle(&s, 7);
	Selection_Clear(&s);
	CHECK(!Selection_Has(&s, 7) && Selection_Count(&s) == 0, "Clear no limpio");
}

static void test_all_state(void) {
	Selection_Clear(&s);

	ListFilter_Build(&f, 10, match_none, NULL);
	CHECK(Selection_AllState(&s, &f) == SELECTION_ALL_DISABLED, "filtro vacio no da DISABLED");

	ListFilter_Build(&f, 0, NULL, NULL);
	CHECK(Selection_AllState(&s, &f) == SELECTION_ALL_DISABLED, "lista vacia no da DISABLED");

	ListFilter_Build(&f, 10, NULL, NULL);
	CHECK(Selection_AllState(&s, &f) == SELECTION_ALL_NONE, "sin marcas no da NONE");

	for (int i = 0; i < 4; i++)
		Selection_Set(&s, i, 1);
	CHECK(Selection_AllState(&s, &f) == SELECTION_ALL_SOME, "4 de 10 no da SOME");

	for (int i = 0; i < 10; i++)
		Selection_Set(&s, i, 1);
	CHECK(Selection_AllState(&s, &f) == SELECTION_ALL_ALL, "10 de 10 no da ALL");

	// Marcas ocultas: no cuentan para la casilla, si para el contador.
	Selection_Clear(&s);
	int queen = 1, abba = 2;
	ListFilter_Build(&f, 10, match_mod3, &queen);
	Selection_Set(&s, 1, 1);
	Selection_Set(&s, 4, 1);
	Selection_Set(&s, 7, 1);
	CHECK(Selection_AllState(&s, &f) == SELECTION_ALL_ALL, "las tres de queen visibles no dan ALL");

	ListFilter_Build(&f, 10, match_mod3, &abba);
	CHECK(Selection_AllState(&s, &f) == SELECTION_ALL_NONE, "con solo marcas ocultas no da NONE");
	CHECK(Selection_Count(&s) == 3, "las ocultas no cuentan: %d", Selection_Count(&s));
}

static void test_toggle_all(void) {
	Selection_Clear(&s);

	// Todo sobre una casilla parcial marca todo lo visible.
	ListFilter_Build(&f, 10, NULL, NULL);
	Selection_Set(&s, 3, 1);
	Selection_ToggleAll(&s, &f);
	CHECK(Selection_Count(&s) == 10, "Todo sobre parcial dejo %d", Selection_Count(&s));
	Selection_ToggleAll(&s, &f);
	CHECK(Selection_Count(&s) == 0, "Todo sobre llena dejo %d", Selection_Count(&s));

	// "queen" y luego "abba": Todo no toca lo oculto.
	int queen = 1, abba = 2;
	ListFilter_Build(&f, 10, match_mod3, &queen);
	Selection_Toggle(&s, ListFilter_ToFull(&f, 0));
	Selection_Toggle(&s, ListFilter_ToFull(&f, 2));
	CHECK(Selection_Count(&s) == 2, "dos de queen cuentan %d", Selection_Count(&s));

	ListFilter_Build(&f, 10, match_mod3, &abba);
	Selection_ToggleAll(&s, &f);
	CHECK(Selection_Count(&s) == 5, "queen + Todo en abba cuenta %d", Selection_Count(&s));
	CHECK(Selection_AllState(&s, &f) == SELECTION_ALL_ALL, "abba no quedo llena");
	Selection_ToggleAll(&s, &f);
	CHECK(Selection_Count(&s) == 2, "Todo dos veces en abba cuenta %d", Selection_Count(&s));
	CHECK(Selection_Has(&s, 1) && Selection_Has(&s, 7) && !Selection_Has(&s, 4), "Todo cambio las marcas de queen");

	// Sin filas visibles no hace nada.
	ListFilter_Build(&f, 10, match_none, NULL);
	Selection_ToggleAll(&s, &f);
	CHECK(Selection_Count(&s) == 2, "Todo deshabilitada cambio la cuenta a %d", Selection_Count(&s));
}

static void test_next(void) {
	Selection_Clear(&s);
	CHECK(Selection_Next(&s, 0) == -1, "sin marcas Next no da -1");

	// Marcadas en otro orden: Next las da en el de la lista completa.
	Selection_Set(&s, 9, 1);
	Selection_Set(&s, 1, 1);
	Selection_Set(&s, 3999, 1);
	Selection_Set(&s, 4, 1);
	Selection_Set(&s, 16, 1);

	int expect[] = { 1, 4, 9, 16, 3999 };
	int pos = 0, n = 0;

	for (int full = Selection_Next(&s, 0); full >= 0; full = Selection_Next(&s, full + 1)) {
		CHECK(n < 5 && full == expect[n], "Next dio %d en el puesto %d", full, n);
		n++;
		pos = full;
	}

	CHECK(n == 5 && pos == 3999, "Next recorrio %d marcas", n);
	CHECK(Selection_Next(&s, 4) == 4, "Next desde una marcada no la da");
	CHECK(Selection_Next(&s, -3) == 1, "Next desde negativo no empieza en 0");
	CHECK(Selection_Next(&s, 4000) == -1, "Next fuera de rango no da -1");
}

static const KeyRepeat_Step none = { 0, 0, 0 };
static const KeyRepeat_Step up = { -1, 0, 0 };
static const KeyRepeat_Step down = { 1, 0, 0 };
static const KeyRepeat_Step held_up = { -1, 1, 0 };
static const KeyRepeat_Step held_down = { 1, 1, 0 };
static const KeyRepeat_Step page_up = { -1, 1, 1 };
static const KeyRepeat_Step page_down = { 1, 1, 1 };

// Posiciones: 0 es la casilla Todo y 1..count las filas visibles.
static void test_step(void) {
	// Pulsacion nueva en los dos bordes: pasa por la casilla.
	CHECK(Selection_Step(1, 10, up, 8) == 0, "arriba desde la primera no lleva a la casilla");
	CHECK(Selection_Step(0, 10, up, 8) == 10, "arriba desde la casilla no lleva a la ultima");
	CHECK(Selection_Step(10, 10, down, 8) == 0, "abajo desde la ultima no lleva a la casilla");
	CHECK(Selection_Step(0, 10, down, 8) == 1, "abajo desde la casilla no lleva a la primera");
	CHECK(Selection_Step(5, 10, up, 8) == 4 && Selection_Step(5, 10, down, 8) == 6, "en el medio no mueve una");

	// La vuelta completa hacia abajo y hacia arriba.
	int pos = 1;
	for (int i = 0; i < 11; i++)
		pos = Selection_Step(pos, 10, down, 8);
	CHECK(pos == 1, "11 pulsaciones abajo no dieron la vuelta: %d", pos);
	for (int i = 0; i < 11; i++)
		pos = Selection_Step(pos, 10, up, 8);
	CHECK(pos == 1, "11 pulsaciones arriba no dieron la vuelta: %d", pos);

	// Pasos repetidos: se detienen en 1 y en count, sin entrar en la casilla.
	CHECK(Selection_Step(1, 10, held_up, 8) == 1, "mantener arriba en la primera entro en la casilla");
	CHECK(Selection_Step(10, 10, held_down, 8) == 10, "mantener abajo en la ultima dio la vuelta");
	CHECK(Selection_Step(3, 10, page_up, 8) == 1, "pagina arriba no se detuvo en la primera");
	CHECK(Selection_Step(5, 10, page_down, 8) == 10, "pagina abajo no se detuvo en la ultima");
	CHECK(Selection_Step(1, 20, page_down, 8) == 9, "pagina abajo no salto 8");
	CHECK(Selection_Step(0, 10, held_up, 8) == 0 && Selection_Step(0, 10, held_down, 8) == 0, "mantener desde la casilla salio de ella");

	// La decima fila, arriba mantenido 3 s: termina en la primera, no en la casilla.
	pos = Selection_Step(10, 10, up, 8);
	for (int i = 0; i < 40; i++)
		pos = Selection_Step(pos, 10, i < 30 ? held_up : page_up, 8);
	CHECK(pos == 1, "mantener arriba desde la decima termino en %d", pos);
	CHECK(Selection_Step(pos, 10, up, 8) == 0, "una pulsacion nueva despues no llevo a la casilla");

	// Sin paso no cambia; una fila sola alterna con la casilla.
	CHECK(Selection_Step(4, 10, none, 8) == 4, "sin paso cambio la posicion");
	CHECK(Selection_Step(1, 1, down, 8) == 0 && Selection_Step(0, 1, down, 8) == 1, "con una fila no alterna con la casilla");

	// count == 0: la casilla esta deshabilitada y la posicion no cambia.
	CHECK(Selection_Step(0, 0, up, 8) == 0 && Selection_Step(0, 0, down, 8) == 0, "count 0 movio la posicion");
	CHECK(Selection_Step(0, 0, held_down, 8) == 0, "count 0 con paso repetido movio la posicion");
}

int main(void) {
	test_mark_and_count();
	test_all_state();
	test_toggle_all();
	test_next();
	test_step();

	if (failures == 0)
		printf("test_selection: todo bien\n");

	return failures == 0 ? 0 : 1;
}
