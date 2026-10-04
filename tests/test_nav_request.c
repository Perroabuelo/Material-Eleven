// Pruebas en PC del pedido diferido de pantalla (source/nav_request.c),
// compiladas con el gcc del host. nav_request.c es logica pura, asi que no hace
// falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>

#include "nav_request.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

static void test_no_request(void) {
	UI_Screen s = NavRequest_Take();
	CHECK(s == UI_SCREEN_NONE, "sin pedido deberia devolver UI_SCREEN_NONE, devolvio %d", s);
}

static void test_take_returns_and_clears(void) {
	NavRequest_Set(UI_SCREEN_NOW_PLAYING);

	UI_Screen s = NavRequest_Take();
	CHECK(s == UI_SCREEN_NOW_PLAYING, "Take deberia devolver el pedido, devolvio %d", s);

	s = NavRequest_Take();
	CHECK(s == UI_SCREEN_NONE, "un segundo Take deberia devolver UI_SCREEN_NONE, devolvio %d", s);
}

static void test_last_request_wins(void) {
	NavRequest_Set(UI_SCREEN_LIBRARY);
	NavRequest_Set(UI_SCREEN_NOW_PLAYING);

	UI_Screen s = NavRequest_Take();
	CHECK(s == UI_SCREEN_NOW_PLAYING, "deberia ganar el ultimo pedido, devolvio %d", s);
	CHECK(NavRequest_Take() == UI_SCREEN_NONE, "despues de Take no deberia quedar ningun pedido");
}

int main(void) {
	test_no_request();
	test_take_returns_and_clears();
	test_last_request_wins();

	if (failures == 0)
		printf("test_nav_request: todo bien\n");

	return failures == 0 ? 0 : 1;
}
