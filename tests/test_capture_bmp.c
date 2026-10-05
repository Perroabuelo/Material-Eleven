// Pruebas en PC del armado de BMP de la captura (source/capture_bmp.c),
// compiladas con el gcc del host. capture_bmp.c es logica pura, asi que no
// hace falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <string.h>

#include "capture_bmp.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

static unsigned int get16(const unsigned char *p) {
	return p[0] | (p[1] << 8);
}

static unsigned int get32(const unsigned char *p) {
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((unsigned int)p[3] << 24);
}

static void test_header(void) {
	unsigned char h[CAPTURE_BMP_HEADER_SIZE];

	CHECK(CaptureBmp_WriteHeader(h, 3, 2) == 1, "una imagen de 3x2 deberia aceptarse");
	CHECK(h[0] == 'B' && h[1] == 'M', "la firma deberia ser BM");
	CHECK(get32(h + 2) == 54 + 3 * 2 * 4, "tamano de archivo %u", get32(h + 2));
	CHECK(get32(h + 10) == 54, "offset de pixeles %u", get32(h + 10));
	CHECK(get32(h + 14) == 40, "tamano de BITMAPINFOHEADER %u", get32(h + 14));
	CHECK(get32(h + 18) == 3, "ancho %u", get32(h + 18));
	CHECK(get32(h + 22) == 2, "alto %u (positivo: filas de abajo hacia arriba)", get32(h + 22));
	CHECK(get16(h + 26) == 1, "planos %u", get16(h + 26));
	CHECK(get16(h + 28) == 32, "bits por pixel %u", get16(h + 28));
	CHECK(get32(h + 30) == 0, "compresion %u, deberia ser BI_RGB", get32(h + 30));
	CHECK(get32(h + 34) == 3 * 2 * 4, "tamano de imagen %u", get32(h + 34));

	// El tamano real de la pantalla de la consola.
	CHECK(CaptureBmp_FileSize(960, 544) == 54 + 960 * 544 * 4, "960x544 deberia medir %u", 54 + 960 * 544 * 4);
}

static void test_invalid_sizes(void) {
	unsigned char h[CAPTURE_BMP_HEADER_SIZE];

	memset(h, 0xAB, sizeof(h));
	CHECK(CaptureBmp_WriteHeader(h, 0, 2) == 0, "ancho 0 deberia rechazarse");
	CHECK(CaptureBmp_WriteHeader(h, 3, 0) == 0, "alto 0 deberia rechazarse");
	CHECK(CaptureBmp_WriteHeader(h, CAPTURE_BMP_MAX_SIDE + 1, 2) == 0, "un ancho excesivo deberia rechazarse");
	CHECK(CaptureBmp_WriteHeader(h, 3, CAPTURE_BMP_MAX_SIDE + 1) == 0, "un alto excesivo deberia rechazarse");
	CHECK(CaptureBmp_WriteHeader(NULL, 3, 2) == 0, "sin destino deberia rechazarse");
	CHECK(h[0] == 0xAB && h[CAPTURE_BMP_HEADER_SIZE - 1] == 0xAB, "un rechazo no deberia tocar el destino");

	CHECK(CaptureBmp_FileSize(0, 0) == 0, "0x0 no tiene tamano");
	CHECK(CaptureBmp_FileSize(CAPTURE_BMP_MAX_SIDE, CAPTURE_BMP_MAX_SIDE) != 0, "el maximo deberia aceptarse");
}

static void test_rows_and_channels(void) {
	// 3x2 en el orden del framebuffer: R, G, B, A. La alfa varia a proposito.
	const unsigned char src[2][3 * 4] = {
		{ 10, 20, 30, 0,     40, 50, 60, 128,   70, 80, 90, 255 },
		{ 11, 21, 31, 7,     41, 51, 61, 7,     71, 81, 91, 7 },
	};
	unsigned char file[2][3 * 4];

	CHECK(CaptureBmp_SourceRow(0, 2) == 1, "la primera fila del archivo es la ultima de la imagen");
	CHECK(CaptureBmp_SourceRow(1, 2) == 0, "la ultima fila del archivo es la primera de la imagen");

	for (unsigned int row = 0; row < 2; row++)
		CaptureBmp_ConvertRow(src[CaptureBmp_SourceRow(row, 2)], file[row], 3);

	// Fila 0 del archivo = fila 1 de la imagen, en B, G, R, 255.
	const unsigned char want0[3 * 4] = { 31, 21, 11, 255,   61, 51, 41, 255,   91, 81, 71, 255 };
	const unsigned char want1[3 * 4] = { 30, 20, 10, 255,   60, 50, 40, 255,   90, 80, 70, 255 };

	CHECK(memcmp(file[0], want0, sizeof(want0)) == 0, "fila 0 del archivo mal convertida");
	CHECK(memcmp(file[1], want1, sizeof(want1)) == 0, "fila 1 del archivo mal convertida");
}

int main(void) {
	test_header();
	test_invalid_sizes();
	test_rows_and_channels();

	if (failures == 0)
		printf("test_capture_bmp: todo bien\n");

	return failures == 0 ? 0 : 1;
}
