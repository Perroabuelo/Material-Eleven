// Pruebas en PC del indice por ruta (source/path_index.c), compiladas con el
// gcc del host. path_index.c es logica pura, asi que no hace falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "path_index.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// path_index.c se compila con -DPATH_INDEX_TEST_ALLOC y reserva por aqui.
static int fail_next = 0;

void *PathIndex_TestMalloc(size_t size) {
	if (fail_next) {
		fail_next = 0;
		return NULL;
	}

	return malloc(size);
}

static const char *get_path(int index, void *ctx) {
	return ((const char **)ctx)[index];
}

static void test_find(void) {
	const char *paths[] = {
		"ux0:music/b.flac",
		"ux0:music/a.flac",
		"ux0:music/Ñandú/c.mp3",
		"ux0:music/A.flac",
		"ux0:music/d/e.ogg",
	};
	int n = 5;
	PathIndex idx;

	CHECK(PathIndex_Build(&idx, n, get_path, paths) == 0, "construir fallo");
	for (int i = 0; i < n; i++)
		CHECK(PathIndex_Find(&idx, paths[i]) == i, "'%s' no dio %d", paths[i], i);

	// Ausentes: antes de todas, despues de todas, en medio y por prefijo.
	CHECK(PathIndex_Find(&idx, "ux0:a") == -1, "una ausente antes de todas aparecio");
	CHECK(PathIndex_Find(&idx, "uz0:") == -1, "una ausente despues de todas aparecio");
	CHECK(PathIndex_Find(&idx, "ux0:music/c.flac") == -1, "una ausente en medio aparecio");
	CHECK(PathIndex_Find(&idx, "ux0:music/a.fla") == -1, "un prefijo aparecio");
	CHECK(PathIndex_Find(&idx, "ux0:music/a.flac ") == -1, "una mas larga aparecio");
	// La comparacion es exacta: mayusculas cuentan, como en el indice.
	CHECK(PathIndex_Find(&idx, "UX0:MUSIC/B.FLAC") == -1, "encontro sin distinguir mayusculas");

	PathIndex_Free(&idx);
	CHECK(idx.order == NULL && idx.count == 0, "liberar no lo dejo vacio");
	PathIndex_Free(&idx);
}

static void test_repeated(void) {
	const char *paths[] = { "z", "x", "y", "x", "z", "x" };
	PathIndex idx;

	CHECK(PathIndex_Build(&idx, 6, get_path, paths) == 0, "construir fallo");
	CHECK(PathIndex_Find(&idx, "x") == 1, "con repetidas no dio la primera x (%d)", PathIndex_Find(&idx, "x"));
	CHECK(PathIndex_Find(&idx, "z") == 0, "con repetidas no dio la primera z");
	CHECK(PathIndex_Find(&idx, "y") == 2, "y no aparecio");
	PathIndex_Free(&idx);
}

static void test_empty(void) {
	PathIndex idx;

	CHECK(PathIndex_Build(&idx, 0, get_path, NULL) == 0, "construir uno vacio fallo");
	CHECK(PathIndex_Find(&idx, "x") == -1, "el vacio encontro algo");
	PathIndex_Free(&idx);

	// Uno que nunca se construyo, a cero, tampoco encuentra nada.
	PathIndex zero;
	memset(&zero, 0, sizeof(zero));
	CHECK(PathIndex_Find(&zero, "x") == -1, "el de ceros encontro algo");
}

static void test_nomem(void) {
	const char *paths[] = { "a", "b" };
	PathIndex idx;

	fail_next = 1;
	CHECK(PathIndex_Build(&idx, 2, get_path, paths) == -1, "sin memoria no dio -1");
	CHECK(idx.order == NULL && idx.count == 0, "sin memoria no quedo vacio");
	CHECK(PathIndex_Find(&idx, "a") == -1, "sin memoria encontro algo");
	PathIndex_Free(&idx);
}

static void test_big(void) {
	// 4000 rutas, el techo de la biblioteca, en orden inverso.
	enum { N = 4000 };
	static char storage[N][32];
	static const char *paths[N];

	for (int i = 0; i < N; i++) {
		snprintf(storage[i], sizeof(storage[i]), "ux0:music/%05d.flac", N - i);
		paths[i] = storage[i];
	}

	PathIndex idx;
	CHECK(PathIndex_Build(&idx, N, get_path, paths) == 0, "construir 4000 fallo");

	int wrong = 0;
	for (int i = 0; i < N; i++)
		wrong += PathIndex_Find(&idx, paths[i]) != i;
	CHECK(wrong == 0, "%d de 4000 no se encontraron en su puesto", wrong);
	PathIndex_Free(&idx);
}

int main(void) {
	test_find();
	test_repeated();
	test_empty();
	test_nomem();
	test_big();

	if (failures == 0)
		printf("test_path_index: todo bien\n");

	return failures == 0 ? 0 : 1;
}
