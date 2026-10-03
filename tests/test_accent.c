// Pruebas en PC del color de acento (source/accent.c), compiladas con el gcc
// del host. accent.c es logica pura, asi que no hace falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <string.h>

#include "accent.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// El fondo de la interfaz, UI_COLOR_BG en ui_theme.h.
#define BG ACCENT_RGBA8(0x12, 0x0F, 0x17, 255)

// Imagenes sinteticas de 48x48, el tamaño de la grilla de muestreo: con paso 1
// se muestrean los 2304 pixeles y las proporciones son exactas.
#define IMG 48
#define IMG_PIXELS (IMG * IMG)

static unsigned char img[IMG_PIXELS * 4];

// Llena una imagen RGB de un solo color.
static void fill_rgb(unsigned char r, unsigned char g, unsigned char b) {
	for (int i = 0; i < IMG_PIXELS; i++) {
		img[i * 3 + 0] = r;
		img[i * 3 + 1] = g;
		img[i * 3 + 2] = b;
	}
}

// Pinta los primeros `count` pixeles de una imagen RGB.
static void paint_rgb(int count, unsigned char r, unsigned char g, unsigned char b) {
	for (int i = 0; i < count; i++) {
		img[i * 3 + 0] = r;
		img[i * 3 + 1] = g;
		img[i * 3 + 2] = b;
	}
}

static int peak(float min_share, unsigned int *out) {
	Accent_HueHistogram hist;
	memset(&hist, 0, sizeof(hist));
	if (!Accent_SamplePixels(img, IMG, IMG, IMG * 3, 3, &hist))
		return 0;
	return Accent_HistogramPeak(&hist, min_share, out);
}

// Valores calculados con UI_MakeAccentLegible de ui_theme.c (v3.2.0), antes de
// mover el codigo a accent.c. Si cambian, cambio el color de alguna portada.
static void test_legible_pinned(void) {
	static const struct { unsigned int in, out; } cases[] = {
		{ ACCENT_RGBA8(0xFF, 0x91, 0x66, 255), ACCENT_RGBA8(0xFB, 0x93, 0x6A, 255) }, // naranja fijo
		{ ACCENT_RGBA8(0x3A, 0x1F, 0x5C, 255), ACCENT_RGBA8(0x97, 0x64, 0xD8, 255) }, // violeta oscuro
		{ ACCENT_RGBA8(0xA8, 0xD8, 0xF0, 255), ACCENT_RGBA8(0x97, 0xD0, 0xED, 255) }, // azul claro
	};

	for (unsigned int i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		unsigned int got = Accent_MakeLegible(cases[i].in, BG);
		CHECK(got == cases[i].out, "MakeLegible(0x%08X) dio 0x%08X, se esperaba 0x%08X", cases[i].in, got, cases[i].out);
	}
}

// Una portada con 20 % de color ya tenia acento antes del cambio: el pico es
// el promedio de los pixeles de color, igual con el umbral de v3.2.0.
static void test_twenty_percent_peak(void) {
	unsigned int red = ACCENT_RGBA8(0xD0, 0x20, 0x20, 255);
	unsigned int got = 0;

	fill_rgb(0x80, 0x80, 0x80);
	paint_rgb(IMG_PIXELS / 5, 0xD0, 0x20, 0x20);

	CHECK(peak(0.06f, &got) && got == red, "20 %% de rojo con 0.06 dio 0x%08X, se esperaba 0x%08X", got, red);
	got = 0;
	CHECK(Accent_DominantColor(img, IMG, IMG, IMG * 3, 3, &got) && got == red,
		"20 %% de rojo con UI_HUE_MIN_SHARE dio 0x%08X, se esperaba 0x%08X", got, red);
}

static void test_invalid_input(void) {
	unsigned int got = 0;
	CHECK(!Accent_DominantColor(NULL, IMG, IMG, IMG * 3, 3, &got), "sin pixeles no deberia dar color");
	CHECK(!Accent_DominantColor(img, 0, IMG, IMG * 3, 3, &got), "ancho 0 no deberia dar color");
	CHECK(!Accent_DominantColor(img, IMG, IMG, IMG * 3, 2, &got), "2 bytes por pixel no deberia dar color");
}

int main(void) {
	test_legible_pinned();
	test_twenty_percent_peak();
	test_invalid_input();

	if (failures == 0)
		printf("test_accent: todo bien\n");

	return failures == 0 ? 0 : 1;
}
