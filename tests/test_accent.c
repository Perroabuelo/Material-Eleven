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

static Accent_Cover classify_rgb(unsigned int *out) {
	return Accent_ClassifyCover(img, IMG, IMG, IMG * 3, 3, out);
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
// el promedio de los pixeles de color, igual con el umbral de v3.2.0 (0.06) que
// con el de ahora.
static void test_twenty_percent_peak(void) {
	unsigned int red = ACCENT_RGBA8(0xD0, 0x20, 0x20, 255);
	unsigned int old_peak = 0, new_peak = 0;

	fill_rgb(0x80, 0x80, 0x80);
	paint_rgb(IMG_PIXELS / 5, 0xD0, 0x20, 0x20);

	CHECK(peak(0.06f, &old_peak) && old_peak == red, "20 %% de rojo con 0.06 dio 0x%08X, se esperaba 0x%08X", old_peak, red);
	CHECK(peak(0.01f, &new_peak) && new_peak == old_peak, "20 %% de rojo con 0.01 dio 0x%08X, con 0.06 0x%08X", new_peak, old_peak);
	new_peak = 0;
	CHECK(classify_rgb(&new_peak) == ACCENT_COVER_CHROMATIC && new_peak == red,
		"20 %% de rojo con UI_HUE_MIN_SHARE dio 0x%08X, se esperaba 0x%08X", new_peak, red);
}

// Un titulo rojo sobre una foto gris, como Entropy (4.1 %).
static void test_small_detail_gives_color(void) {
	unsigned int red = ACCENT_RGBA8(0xD0, 0x20, 0x20, 255);
	unsigned int got = 0;

	fill_rgb(0x80, 0x80, 0x80);
	paint_rgb(IMG_PIXELS * 4 / 100, 0xD0, 0x20, 0x20);

	CHECK(classify_rgb(&got) == ACCENT_COVER_CHROMATIC && got == red,
		"4 %% de rojo deberia dar el rojo 0x%08X, dio 0x%08X", red, got);
}

static void test_no_color(void) {
	static const unsigned char flat[] = { 0xFF, 0x00, 0x80 }; // blanco, negro, gris
	unsigned int got = 0;
	unsigned int seed = 12345;

	// 0.5 % de rojo, como las fotos en blanco y negro de la biblioteca de prueba.
	fill_rgb(0x80, 0x80, 0x80);
	paint_rgb(IMG_PIXELS / 200, 0xD0, 0x20, 0x20);
	CHECK(classify_rgb(&got) == ACCENT_COVER_ACHROMATIC, "0.5 %% de rojo no deberia dar color, dio 0x%08X", got);

	for (unsigned int i = 0; i < sizeof(flat); i++) {
		fill_rgb(flat[i], flat[i], flat[i]);
		CHECK(classify_rgb(&got) == ACCENT_COVER_ACHROMATIC, "0x%02X uniforme no deberia dar color", flat[i]);
	}

	// Ruido de compresion: gris con +-6 por canal.
	for (int i = 0; i < IMG_PIXELS * 3; i++) {
		seed = seed * 1103515245u + 12345u;
		img[i] = (unsigned char)(0x80 - 6 + (int)((seed >> 16) % 13));
	}
	CHECK(classify_rgb(&got) == ACCENT_COVER_ACHROMATIC, "gris con ruido no deberia dar color, dio 0x%08X", got);
}

static void test_invalid_input(void) {
	unsigned int got = 0;
	CHECK(Accent_ClassifyCover(NULL, IMG, IMG, IMG * 3, 3, &got) == ACCENT_COVER_NONE, "sin pixeles deberia ser sin caratula");
	CHECK(Accent_ClassifyCover(img, 0, IMG, IMG * 3, 3, &got) == ACCENT_COVER_NONE, "ancho 0 deberia ser sin caratula");
	CHECK(Accent_ClassifyCover(img, IMG, IMG, IMG * 3, 2, &got) == ACCENT_COVER_NONE, "2 bytes por pixel deberia ser sin caratula");
}

// Un JPEG en escala de grises llega como textura de un solo canal (U8_R): se
// puede leer, asi que es una caratula sin color y no una sin caratula.
static void test_single_channel_is_achromatic(void) {
	unsigned int got = 0;

	for (int i = 0; i < IMG_PIXELS; i++)
		img[i] = (unsigned char)(i % 256);
	CHECK(Accent_ClassifyCover(img, IMG, IMG, IMG, 1, &got) == ACCENT_COVER_ACHROMATIC,
		"una imagen de un canal deberia ser sin color");
}

int main(void) {
	test_legible_pinned();
	test_twenty_percent_peak();
	test_small_detail_gives_color();
	test_no_color();
	test_invalid_input();
	test_single_channel_is_achromatic();

	if (failures == 0)
		printf("test_accent: todo bien\n");

	return failures == 0 ? 0 : 1;
}
