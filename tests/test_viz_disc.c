// Pruebas en PC de la geometria de la etiqueta del vinilo (source/viz_disc.c),
// compiladas con el gcc del host.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <math.h>
#include <stdio.h>

#include "viz_disc.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

#define SEG VIZ_DISC_SEGMENTS
#define COUNT VIZ_DISC_FAN_VERTICES(SEG)

static VizDisc_Vertex a[COUNT], b[COUNT];

static void test_shape(void) {
	unsigned int n = VizDisc_Fan(a, 480.0f, 272.0f, 100.0f, SEG, 0.0f, 256.0f, 256.0f);
	CHECK(n == COUNT, "deberia escribir %d vertices, escribio %u", COUNT, n);

	CHECK(a[0].x == 480.0f && a[0].y == 272.0f, "el primero deberia ser el centro");
	CHECK(a[0].u == 0.5f && a[0].v == 0.5f, "el centro deberia tomar el centro de la caratula");

	int on_rim = 1, uv_in = 1;
	for (int i = 1; i < COUNT; i++) {
		float r = hypotf(a[i].x - 480.0f, a[i].y - 272.0f);
		on_rim &= fabsf(r - 100.0f) < 1e-3f;
		uv_in &= a[i].u >= 0.0f && a[i].u <= 1.0f && a[i].v >= 0.0f && a[i].v <= 1.0f;
		CHECK(a[i].z == 0.5f, "z deberia ser 0,5");
	}
	CHECK(on_rim, "el borde deberia estar a radio exacto");
	CHECK(uv_in, "las UV deberian quedar en [0, 1]");

	CHECK(a[1].x == a[COUNT - 1].x && a[1].y == a[COUNT - 1].y && a[1].u == a[COUNT - 1].u,
		"el ultimo vertice deberia repetir el primero del borde para cerrar el abanico");

	// La caratula entera inscripta: el borde toca los cuatro lados.
	float umin = 1, umax = 0;
	for (int i = 1; i < COUNT; i++) {
		umin = fminf(umin, a[i].u);
		umax = fmaxf(umax, a[i].u);
	}
	CHECK(umin < 0.001f && umax > 0.999f, "sin giro el circulo deberia tocar los bordes de la caratula: %f..%f", umin, umax);
}

static void test_rotation_moves_uv_only(void) {
	VizDisc_Fan(a, 480.0f, 272.0f, 100.0f, SEG, 0.0f, 256.0f, 256.0f);
	VizDisc_Fan(b, 480.0f, 272.0f, 100.0f, SEG, 1.0f, 256.0f, 256.0f);

	int same_pos = 1, moved_uv = 0, uv_in = 1;
	for (int i = 0; i < COUNT; i++) {
		same_pos &= a[i].x == b[i].x && a[i].y == b[i].y;
		moved_uv |= fabsf(a[i].u - b[i].u) > 0.01f;
		uv_in &= b[i].u >= 0.0f && b[i].u <= 1.0f && b[i].v >= 0.0f && b[i].v <= 1.0f;
	}
	CHECK(same_pos, "girar no deberia mover las posiciones");
	CHECK(moved_uv, "girar deberia desplazar las UV");
	CHECK(uv_in, "giradas, las UV deberian seguir en [0, 1]");

	// Un giro de un segmento entero equivale a correr las UV un vertice.
	VizDisc_Fan(b, 480.0f, 272.0f, 100.0f, SEG, 6.28318530718f / SEG, 256.0f, 256.0f);
	CHECK(fabsf(b[1].u - a[2].u) < 1e-4f && fabsf(b[1].v - a[2].v) < 1e-4f, "un segmento de giro deberia correr las UV un vertice");
}

static void test_crop(void) {
	// Caratula apaisada: se recorta al cuadrado central.
	VizDisc_Fan(a, 0.0f, 0.0f, 50.0f, SEG, 0.3f, 400.0f, 200.0f);
	float umin = 1, umax = 0, vmin = 1, vmax = 0;
	for (int i = 1; i < COUNT; i++) {
		umin = fminf(umin, a[i].u); umax = fmaxf(umax, a[i].u);
		vmin = fminf(vmin, a[i].v); vmax = fmaxf(vmax, a[i].v);
	}
	CHECK(umin > 0.24f && umax < 0.76f, "a lo ancho deberia tomar solo el centro: %f..%f", umin, umax);
	CHECK(vmin < 0.01f && vmax > 0.99f, "a lo alto deberia tomar todo: %f..%f", vmin, vmax);

	// Vertical: al reves.
	VizDisc_Fan(a, 0.0f, 0.0f, 50.0f, SEG, 0.0f, 100.0f, 300.0f);
	vmin = 1; vmax = 0;
	for (int i = 1; i < COUNT; i++) {
		vmin = fminf(vmin, a[i].v); vmax = fmaxf(vmax, a[i].v);
	}
	CHECK(vmin > 0.32f && vmax < 0.68f, "a lo alto deberia tomar solo el centro: %f..%f", vmin, vmax);
}

static void test_min_segments(void) {
	VizDisc_Vertex few[VIZ_DISC_FAN_VERTICES(3)];
	CHECK(VizDisc_Fan(few, 0.0f, 0.0f, 10.0f, 1, 0.0f, 1.0f, 1.0f) == VIZ_DISC_FAN_VERTICES(3), "menos de 3 segmentos deberia dar 3");
}

int main(void) {
	test_shape();
	test_rotation_moves_uv_only();
	test_crop();
	test_min_segments();

	if (failures == 0)
		printf("test_viz_disc: todo bien\n");

	return failures == 0 ? 0 : 1;
}
