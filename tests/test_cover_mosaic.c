// Pruebas en PC del mosaico de caratulas (source/cover_mosaic.c), compiladas
// con el gcc del host. cover_mosaic.c es logica pura, asi que no hace falta el
// VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <string.h>

#include "cover_mosaic.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// El tamaño del cache de caratulas (COVER_SIZE).
#define SIZE 128

static unsigned char src[SIZE * SIZE * 4];
static unsigned char dst[SIZE * SIZE * 4];

static unsigned char *px(unsigned char *img, int x, int y) {
	return img + ((size_t)y * SIZE + (size_t)x) * 4;
}

static void test_reduce_pattern(void) {
	// Un patron conocido: cada bloque 2x2 de origen tiene los valores 0, 10,
	// 20 y 30 en el rojo, su x en el verde, su y en el azul y 255 de alfa.
	for (int y = 0; y < SIZE; y++) {
		for (int x = 0; x < SIZE; x++) {
			unsigned char *p = px(src, x, y);
			p[0] = (unsigned char)(((y % 2) * 2 + (x % 2)) * 10);
			p[1] = (unsigned char)(x / 2);
			p[2] = (unsigned char)(y / 2);
			p[3] = 255;
		}
	}

	memset(dst, 0, sizeof(dst));
	CoverMosaic_Reduce(src, dst, SIZE, 0);

	int wrong = 0;
	for (int y = 0; y < SIZE / 2; y++) {
		for (int x = 0; x < SIZE / 2; x++) {
			unsigned char *p = px(dst, x, y);
			// (0 + 10 + 20 + 30) / 4 = 15
			wrong += p[0] != 15 || p[1] != x || p[2] != y || p[3] != 255;
		}
	}
	CHECK(wrong == 0, "%d pixeles de la reduccion no son el promedio de su bloque", wrong);

	// El resto de dst no se toco.
	int touched = 0;
	for (int y = 0; y < SIZE; y++) {
		for (int x = 0; x < SIZE; x++) {
			if (x < SIZE / 2 && y < SIZE / 2)
				continue;
			unsigned char *p = px(dst, x, y);
			touched += p[0] | p[1] | p[2] | p[3];
		}
	}
	CHECK(touched == 0, "la reduccion escribio fuera de su cuadrante");
}

static void test_rounding(void) {
	// 1 + 1 + 2 + 2 = 6, y 6/4 = 1,5 redondea a 2; un color plano sigue igual.
	for (int y = 0; y < SIZE; y++) {
		for (int x = 0; x < SIZE; x++) {
			unsigned char *p = px(src, x, y);
			p[0] = (unsigned char)(y % 2 ? 2 : 1);
			p[1] = 200;
			p[2] = 7;
			p[3] = 255;
		}
	}

	CoverMosaic_Reduce(src, dst, SIZE, 0);
	unsigned char *p = px(dst, 5, 5);
	CHECK(p[0] == 2 && p[1] == 200 && p[2] == 7 && p[3] == 255, "redondeo: %d %d %d %d", p[0], p[1], p[2], p[3]);
}

static void test_quarters(void) {
	// Cuatro caratulas de un color cada una, una por cuadrante.
	const unsigned char colors[4][4] = {
		{ 255, 0, 0, 255 }, { 0, 255, 0, 255 }, { 0, 0, 255, 255 }, { 255, 255, 0, 255 },
	};

	memset(dst, 0, sizeof(dst));
	for (int q = 0; q < COVER_MOSAIC_QUARTERS; q++) {
		for (int i = 0; i < SIZE * SIZE; i++)
			memcpy(src + (size_t)i * 4, colors[q], 4);
		CoverMosaic_Reduce(src, dst, SIZE, q);
	}

	// Esquinas y bordes de cada cuadrante, donde se notaria un desfase.
	const int probes[][2] = {
		{ 0, 0 }, { 63, 0 }, { 0, 63 }, { 63, 63 },
		{ 64, 0 }, { 127, 0 }, { 64, 63 }, { 127, 63 },
		{ 0, 64 }, { 63, 64 }, { 0, 127 }, { 63, 127 },
		{ 64, 64 }, { 127, 64 }, { 64, 127 }, { 127, 127 },
	};

	for (size_t i = 0; i < sizeof(probes) / sizeof(probes[0]); i++) {
		int x = probes[i][0], y = probes[i][1];
		int q = (y >= SIZE / 2) * 2 + (x >= SIZE / 2);
		CHECK(memcmp(px(dst, x, y), colors[q], 4) == 0, "(%d,%d) no tiene el color del cuadrante %d", x, y, q);
	}
}

static void test_fill(void) {
	const unsigned char surface[4] = { 0x1C, 0x18, 0x26, 255 };
	const unsigned char other[4] = { 9, 9, 9, 9 };

	for (int q = 0; q < COVER_MOSAIC_QUARTERS; q++)
		CoverMosaic_Fill(dst, SIZE, q, other);
	CoverMosaic_Fill(dst, SIZE, 3, surface);

	CHECK(memcmp(px(dst, 64, 64), surface, 4) == 0 && memcmp(px(dst, 127, 127), surface, 4) == 0, "el cuadrante 3 no se lleno");
	CHECK(memcmp(px(dst, 63, 127), other, 4) == 0 && memcmp(px(dst, 127, 63), other, 4) == 0, "llenar el 3 toco a sus vecinos");
}

int main(void) {
	test_reduce_pattern();
	test_rounding();
	test_quarters();
	test_fill();

	if (failures == 0)
		printf("test_cover_mosaic: todo bien\n");

	return failures == 0 ? 0 : 1;
}
