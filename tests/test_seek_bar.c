// Pruebas en PC de la barra de progreso (source/seek_bar.c), compiladas con
// el gcc del host. seek_bar.c es logica pura, asi que no hace falta el
// VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>

#include "seek_bar.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

#define NEAR(a, b) ((a) - (b) < 0.0001 && (b) - (a) < 0.0001)

// Una barra de 400 px desde x 500, con la zona tactil de 189 a 255.
static const SeekBar_Geometry GEO = { 500.0f, 400.0f, 189.0f, 255.0f };
#define MID_Y 222.0f

static SeekBar_Input press(float x, float y) {
	SeekBar_Input in = { 1, 1, 0, x, y };
	return in;
}

static SeekBar_Input hold(float x, float y) {
	SeekBar_Input in = { 0, 1, 0, x, y };
	return in;
}

// touch.c deja la ultima posicion sostenida en el fotograma de soltar.
static SeekBar_Input release(float x, float y) {
	SeekBar_Input in = { 0, 0, 1, x, y };
	return in;
}

static SeekBar_Input nothing(void) {
	SeekBar_Input in = { 0, 0, 0, 0.0f, 0.0f };
	return in;
}

static void test_fraction_at(void) {
	CHECK(NEAR(SeekBar_FractionAt(&GEO, 500.0f), 0.0f), "el borde izquierdo deberia dar 0");
	CHECK(NEAR(SeekBar_FractionAt(&GEO, 900.0f), 1.0f), "el borde derecho deberia dar 1");
	CHECK(NEAR(SeekBar_FractionAt(&GEO, 700.0f), 0.5f), "el centro deberia dar 0,5");
	CHECK(NEAR(SeekBar_FractionAt(&GEO, 100.0f), 0.0f), "fuera a la izquierda deberia acotarse a 0");
	CHECK(NEAR(SeekBar_FractionAt(&GEO, 960.0f), 1.0f), "fuera a la derecha deberia acotarse a 1");

	const SeekBar_Geometry flat = { 500.0f, 0.0f, 189.0f, 255.0f };
	const SeekBar_Geometry negative = { 500.0f, -10.0f, 189.0f, 255.0f };
	CHECK(NEAR(SeekBar_FractionAt(&flat, 600.0f), 0.0f), "una barra sin ancho deberia dar 0");
	CHECK(NEAR(SeekBar_FractionAt(&negative, 400.0f), 0.0f), "una barra de ancho negativo deberia dar 0");
}

static void test_tap_commits_at_point(void) {
	SeekBar_State st = { 0 };
	double out = -1.0;

	SeekBar_Input in = press(800.0f, MID_Y);
	SeekBar_Result r = SeekBar_Update(&st, &GEO, &in, &out);
	CHECK(r == SEEKBAR_DRAGGING, "el toque deberia agarrar la barra, dio %d", r);
	CHECK(out == -1.0, "agarrar no deberia pedir un seek");

	in = release(800.0f, MID_Y);
	r = SeekBar_Update(&st, &GEO, &in, &out);
	CHECK(r == SEEKBAR_COMMIT, "soltar deberia dar COMMIT, dio %d", r);
	CHECK(NEAR(out, 0.75), "el seek deberia ir a 0,75, fue a %f", out);
	CHECK(!st.grabbed, "despues de soltar la barra no deberia quedar agarrada");
}

static void test_drag_commits_once_at_end(void) {
	SeekBar_State st = { 0 };
	double out = -1.0;
	int commits = 0;

	SeekBar_Input in = press(600.0f, MID_Y);
	if (SeekBar_Update(&st, &GEO, &in, &out) == SEEKBAR_COMMIT)
		commits++;
	CHECK(NEAR(st.fraction, 0.25f), "deberia agarrar en 0,25, agarro en %f", st.fraction);

	const float xs[] = { 620.0f, 700.0f, 780.0f, 830.0f, 860.0f };
	for (unsigned i = 0; i < sizeof(xs) / sizeof(xs[0]); i++) {
		in = hold(xs[i], MID_Y);
		SeekBar_Result r = SeekBar_Update(&st, &GEO, &in, &out);
		CHECK(r == SEEKBAR_DRAGGING, "un fotograma sostenido deberia dar DRAGGING, dio %d", r);
		if (r == SEEKBAR_COMMIT)
			commits++;
		CHECK(NEAR(st.fraction, (xs[i] - 500.0f) / 400.0f), "la fraccion deberia seguir al dedo en x %f", xs[i]);
	}

	CHECK(out == -1.0, "arrastrar no deberia pedir un seek");

	in = release(860.0f, MID_Y);
	if (SeekBar_Update(&st, &GEO, &in, &out) == SEEKBAR_COMMIT)
		commits++;
	CHECK(commits == 1, "deberia haber un solo COMMIT, hubo %d", commits);
	CHECK(NEAR(out, 0.9), "el seek deberia ir a 0,9, fue a %f", out);
}

static void test_drag_ignores_y(void) {
	SeekBar_State st = { 0 };
	double out = -1.0;

	SeekBar_Input in = press(600.0f, MID_Y);
	SeekBar_Update(&st, &GEO, &in, &out);

	// Arriba sobre la caratula y abajo sobre los controles.
	in = hold(700.0f, 40.0f);
	SeekBar_Result r = SeekBar_Update(&st, &GEO, &in, &out);
	CHECK(r == SEEKBAR_DRAGGING, "salir hacia arriba deberia seguir arrastrando, dio %d", r);
	CHECK(NEAR(st.fraction, 0.5f), "la fraccion deberia seguir la x aunque la y salga");

	in = hold(880.0f, 500.0f);
	r = SeekBar_Update(&st, &GEO, &in, &out);
	CHECK(r == SEEKBAR_DRAGGING, "salir hacia abajo deberia seguir arrastrando, dio %d", r);

	in = release(880.0f, 500.0f);
	r = SeekBar_Update(&st, &GEO, &in, &out);
	CHECK(r == SEEKBAR_COMMIT, "soltar fuera de la zona deberia dar COMMIT, dio %d", r);
	CHECK(NEAR(out, 0.95), "el seek deberia ir a 0,95, fue a %f", out);
}

static void test_press_outside_does_not_grab(void) {
	const float points[][2] = {
		{ 700.0f, 188.0f },   // arriba
		{ 700.0f, 256.0f },   // abajo
		{ 499.0f, MID_Y },    // a la izquierda
		{ 901.0f, MID_Y },    // a la derecha
	};

	for (unsigned i = 0; i < sizeof(points) / sizeof(points[0]); i++) {
		SeekBar_State st = { 0 };
		double out = -1.0;

		SeekBar_Input in = press(points[i][0], points[i][1]);
		SeekBar_Result r = SeekBar_Update(&st, &GEO, &in, &out);
		CHECK(r == SEEKBAR_IDLE, "un toque en (%f, %f) no deberia agarrar, dio %d", points[i][0], points[i][1], r);

		// Ni deslizando el dedo hasta la barra.
		in = hold(700.0f, MID_Y);
		r = SeekBar_Update(&st, &GEO, &in, &out);
		CHECK(r == SEEKBAR_IDLE, "deslizar hasta la barra sin agarrarla no deberia arrastrar, dio %d", r);

		in = release(700.0f, MID_Y);
		r = SeekBar_Update(&st, &GEO, &in, &out);
		CHECK(r == SEEKBAR_IDLE, "soltar sin haber agarrado no deberia dar COMMIT, dio %d", r);
		CHECK(out == -1.0, "sin agarrar no deberia pedir un seek");
	}

	// Los bordes de la zona si agarran.
	SeekBar_State st = { 0 };
	SeekBar_Input in = press(900.0f, 255.0f);
	CHECK(SeekBar_Update(&st, &GEO, &in, NULL) == SEEKBAR_DRAGGING, "el borde de la zona deberia agarrar");
}

static void test_zero_width_never_grabs(void) {
	const SeekBar_Geometry flat = { 500.0f, 0.0f, 189.0f, 255.0f };
	SeekBar_State st = { 0 };

	SeekBar_Input in = press(500.0f, MID_Y);
	CHECK(SeekBar_Update(&st, &flat, &in, NULL) == SEEKBAR_IDLE, "una barra sin ancho no deberia agarrar");
}

static void test_display_fraction(void) {
	SeekBar_State st = { 0 };

	CHECK(NEAR(SeekBar_DisplayFraction(&st, 0.3f), 0.3f), "sin agarrar deberia mostrar la reproduccion");

	SeekBar_Input in = press(860.0f, MID_Y);
	SeekBar_Update(&st, &GEO, &in, NULL);
	CHECK(NEAR(SeekBar_DisplayFraction(&st, 0.3f), 0.9f), "agarrada deberia mostrar el dedo");

	in = release(860.0f, MID_Y);
	SeekBar_Update(&st, &GEO, &in, NULL);
	CHECK(NEAR(SeekBar_DisplayFraction(&st, 0.3f), 0.3f), "al soltar deberia volver a la reproduccion");
}

static void test_reset_drops_grab(void) {
	SeekBar_State st = { 0 };
	double out = -1.0;

	SeekBar_Input in = press(700.0f, MID_Y);
	SeekBar_Update(&st, &GEO, &in, &out);
	SeekBar_Reset(&st);
	CHECK(!st.grabbed, "reiniciar deberia soltar la barra");

	in = release(700.0f, MID_Y);
	SeekBar_Result r = SeekBar_Update(&st, &GEO, &in, &out);
	CHECK(r == SEEKBAR_IDLE, "un released suelto despues de reiniciar no deberia dar COMMIT, dio %d", r);
	CHECK(out == -1.0, "despues de reiniciar no deberia pedir un seek");
}

static void test_lost_touch_commits(void) {
	SeekBar_State st = { 0 };
	double out = -1.0;

	// Un fotograma sin held y sin el flanco de released tambien suelta.
	SeekBar_Input in = press(700.0f, MID_Y);
	SeekBar_Update(&st, &GEO, &in, &out);

	in = nothing();
	SeekBar_Result r = SeekBar_Update(&st, &GEO, &in, &out);
	CHECK(r == SEEKBAR_COMMIT, "un fotograma sin held deberia soltar, dio %d", r);
	CHECK(NEAR(out, 0.5), "deberia conservar la ultima fraccion, dio %f", out);

	in = nothing();
	r = SeekBar_Update(&st, &GEO, &in, &out);
	CHECK(r == SEEKBAR_IDLE, "despues de soltar deberia quedar en reposo, dio %d", r);
}

int main(void) {
	test_fraction_at();
	test_tap_commits_at_point();
	test_drag_commits_once_at_end();
	test_drag_ignores_y();
	test_press_outside_does_not_grab();
	test_zero_width_never_grabs();
	test_display_fraction();
	test_reset_drops_grab();
	test_lost_touch_commits();

	if (failures == 0)
		printf("test_seek_bar: todo bien\n");

	return failures == 0 ? 0 : 1;
}
