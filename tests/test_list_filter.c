// Pruebas en PC de la lista filtrada de la biblioteca (source/list_filter.c),
// compiladas con el gcc del host. list_filter.c es logica pura, asi que no hace
// falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <stddef.h>

#include "list_filter.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// Static: a ListFilter is 16 KB, and that is how menu_library.c keeps it too.
static ListFilter f;

static int match_none(int index, void *ctx) {
	(void)index;
	(void)ctx;
	return 0;
}

static int match_multiple(int index, void *ctx) {
	return index % *(int *)ctx == 0;
}

static int match_odd_and_count(int index, void *ctx) {
	(*(int *)ctx)++;
	return index % 2 == 1;
}

static void test_identity(void) {
	CHECK(ListFilter_Build(&f, 10, NULL, NULL) == 10 && ListFilter_Count(&f) == 10, "sin termino no dejo las 10");

	for (int i = 0; i < 10; i++) {
		CHECK(ListFilter_ToFull(&f, i) == i, "la identidad no lleva %d a si mismo", i);
		CHECK(ListFilter_ToVisible(&f, i) == i, "la identidad no vuelve %d a si mismo", i);
	}
}

static void test_none(void) {
	CHECK(ListFilter_Build(&f, 10, match_none, NULL) == 0 && ListFilter_Count(&f) == 0, "ninguna coincidencia dejo algo");
	CHECK(ListFilter_ToFull(&f, 0) == -1, "ToFull en una lista vacia no dio -1");
	CHECK(ListFilter_ToVisible(&f, 0) == -1, "ToVisible en una lista vacia no dio -1");
}

static void test_order_and_edges(void) {
	int every = 3;
	// 0 3 6 9 de 10 entradas, en ese orden.
	CHECK(ListFilter_Build(&f, 10, match_multiple, &every) == 4, "no quedaron 4 multiplos de 3");
	CHECK(ListFilter_ToFull(&f, 0) == 0 && ListFilter_ToFull(&f, 1) == 3
		&& ListFilter_ToFull(&f, 2) == 6 && ListFilter_ToFull(&f, 3) == 9, "no conservo el orden");

	CHECK(ListFilter_ToFull(&f, -1) == -1, "ToFull de -1 no dio -1");
	CHECK(ListFilter_ToFull(&f, 4) == -1, "ToFull pasado el final no dio -1");

	CHECK(ListFilter_ToVisible(&f, 0) == 0 && ListFilter_ToVisible(&f, 9) == 3, "ToVisible fallo en los bordes");
	CHECK(ListFilter_ToVisible(&f, 6) == 2, "ToVisible fallo en el medio");
	CHECK(ListFilter_ToVisible(&f, 4) == -1, "una entrada filtrada tuvo posicion visible");
	CHECK(ListFilter_ToVisible(&f, -1) == -1 && ListFilter_ToVisible(&f, 10) == -1, "ToVisible fuera de rango no dio -1");

	for (int v = 0; v < ListFilter_Count(&f); v++)
		CHECK(ListFilter_ToVisible(&f, ListFilter_ToFull(&f, v)) == v, "ida y vuelta fallo en %d", v);

	// La funcion recibe el contexto y se llama una vez por entrada.
	int calls = 0;
	CHECK(ListFilter_Build(&f, 7, match_odd_and_count, &calls) == 3 && calls == 7, "no llamo una vez por entrada");
}

static void test_counts(void) {
	CHECK(ListFilter_Build(&f, 0, NULL, NULL) == 0 && ListFilter_ToFull(&f, 0) == -1, "count 0 dejo algo");
	CHECK(ListFilter_Build(&f, -5, NULL, NULL) == 0, "un count negativo dejo algo");

	CHECK(ListFilter_Build(&f, LIST_FILTER_MAX, NULL, NULL) == LIST_FILTER_MAX, "no cupo la lista maxima");
	CHECK(ListFilter_ToFull(&f, LIST_FILTER_MAX - 1) == LIST_FILTER_MAX - 1, "ToFull de la ultima fallo");
	CHECK(ListFilter_ToVisible(&f, LIST_FILTER_MAX - 1) == LIST_FILTER_MAX - 1, "ToVisible de la ultima fallo");
	CHECK(ListFilter_ToFull(&f, LIST_FILTER_MAX) == -1, "ToFull paso del maximo");

	// Una lista mas larga se recorta al maximo en vez de escribir fuera.
	CHECK(ListFilter_Build(&f, LIST_FILTER_MAX + 10, NULL, NULL) == LIST_FILTER_MAX, "no recorto al maximo");
}

int main(void) {
	test_identity();
	test_none();
	test_order_and_edges();
	test_counts();

	if (failures == 0)
		printf("test_list_filter: todo bien\n");

	return failures == 0 ? 0 : 1;
}
