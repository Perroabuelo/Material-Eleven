// Pruebas en PC del reconocedor de gestos (source/gesture.c), compiladas con
// el gcc del host. gesture.c es logica pura, asi que no hace falta el
// VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>

#include "gesture.h"

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

#define FRAME_MS 16.67f

static Gesture_Result down(Gesture_State *st, float x, float y) {
	Gesture_Input in = { 1, x, y, FRAME_MS };
	return Gesture_Update(st, &in);
}

static Gesture_Result up(Gesture_State *st) {
	Gesture_Input in = { 0, 0.0f, 0.0f, FRAME_MS };
	return Gesture_Update(st, &in);
}

static void test_idle(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	Gesture_Result r = up(&st);
	CHECK(r.type == GESTURE_NONE, "sin dedo no deberia pasar nada, dio %d", r.type);
	CHECK(!Gesture_Pressing(&st), "sin dedo no deberia haber nada apretado");
}

static void test_tap(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	Gesture_Result r = down(&st, 300.0f, 200.0f);
	CHECK(r.type == GESTURE_PRESS, "apoyar deberia dar PRESS, dio %d", r.type);
	CHECK(NEAR(r.start_x, 300.0f) && NEAR(r.start_y, 200.0f), "PRESS deberia informar el apoyo");
	CHECK(Gesture_Pressing(&st), "con el dedo quieto la fila deberia verse apretada");

	// Se mueve un poco, sin pasar el umbral.
	r = down(&st, 305.0f, 210.0f);
	CHECK(r.type == GESTURE_NONE, "moverse bajo el umbral no deberia arrastrar, dio %d", r.type);
	CHECK(Gesture_Pressing(&st), "bajo el umbral la fila deberia seguir apretada");

	r = up(&st);
	CHECK(r.type == GESTURE_TAP, "soltar sin arrastrar deberia dar TAP, dio %d", r.type);
	CHECK(NEAR(r.start_x, 300.0f) && NEAR(r.start_y, 200.0f), "TAP deberia informar el apoyo");
	CHECK(NEAR(r.end_x, 305.0f) && NEAR(r.end_y, 210.0f), "TAP deberia informar la salida");
	CHECK(!Gesture_Pressing(&st), "despues de soltar no deberia haber nada apretado");

	r = up(&st);
	CHECK(r.type == GESTURE_NONE, "un segundo fotograma sin dedo no deberia repetir el TAP");
}

static void test_drag_never_taps(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	down(&st, 300.0f, 300.0f);
	Gesture_Result r = down(&st, 300.0f, 290.0f);
	CHECK(r.type == GESTURE_NONE, "10 px no deberia arrastrar");

	r = down(&st, 300.0f, 280.0f);
	CHECK(r.type == GESTURE_DRAG, "20 px deberia arrastrar, dio %d", r.type);
	CHECK(NEAR(r.dy, -20.0f), "el primer arrastre deberia traer todo lo movido, dio %f", r.dy);
	CHECK(!Gesture_Pressing(&st), "al arrastrar el resaltado deberia desaparecer");

	r = down(&st, 300.0f, 270.0f);
	CHECK(r.type == GESTURE_DRAG && NEAR(r.dy, -10.0f), "despues solo el movimiento del fotograma, dio %f", r.dy);

	// Vuelve al punto de apoyo: sigue siendo arrastre.
	r = down(&st, 300.0f, 300.0f);
	CHECK(r.type == GESTURE_DRAG, "volver al apoyo no deberia deshacer el arrastre");

	r = up(&st);
	CHECK(r.type == GESTURE_RELEASE, "soltar un arrastre deberia dar RELEASE, dio %d", r.type);
	CHECK(r.type != GESTURE_TAP, "un arrastre nunca deberia dar TAP");
}

static void test_horizontal_cancels_tap(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	down(&st, 300.0f, 300.0f);
	Gesture_Result r = down(&st, 340.0f, 300.0f);
	CHECK(r.type == GESTURE_DRAG && NEAR(r.dy, 0.0f), "moverse de lado deberia arrastrar sin desplazar");
	r = up(&st);
	CHECK(r.type == GESTURE_RELEASE, "un deslizamiento de lado no deberia dar TAP");
}

static void test_lift_outside_reports_end(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	// Un toque corto que termina en otra parte informa la salida, y la
	// pantalla lo descarta si cae fuera de la lista.
	down(&st, 300.0f, 500.0f);
	down(&st, 300.0f, 510.0f);
	Gesture_Result r = up(&st);
	CHECK(r.type == GESTURE_TAP, "bajo el umbral deberia seguir siendo TAP");
	CHECK(NEAR(r.end_y, 510.0f), "deberia informar la salida en 510, dio %f", r.end_y);

	down(&st, 300.0f, 300.0f);
	down(&st, 300.0f, 400.0f);
	down(&st, 300.0f, 530.0f);
	r = up(&st);
	CHECK(r.type == GESTURE_RELEASE, "un arrastre largo deberia dar RELEASE");
	CHECK(NEAR(r.start_y, 300.0f) && NEAR(r.end_y, 530.0f), "RELEASE deberia informar apoyo y salida");
}

static void test_release_speed(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	down(&st, 300.0f, 400.0f);
	float y = 400.0f;
	for (int i = 0; i < 8; i++) {
		y -= 20.0f;
		down(&st, 300.0f, y);
	}

	Gesture_Result r = up(&st);
	CHECK(r.type == GESTURE_RELEASE, "deberia dar RELEASE");
	CHECK(NEAR(r.velocity, -20.0f / FRAME_MS), "la velocidad deberia ser -20 px por fotograma, dio %f", r.velocity);
}

static void test_still_before_release_no_speed(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	down(&st, 300.0f, 400.0f);
	down(&st, 300.0f, 350.0f);
	down(&st, 300.0f, 300.0f);
	for (int i = 0; i < GESTURE_SPEED_SAMPLES; i++)
		down(&st, 300.0f, 300.0f);

	Gesture_Result r = up(&st);
	CHECK(NEAR(r.velocity, 0.0f), "quedarse quieto antes de soltar no deberia dejar velocidad, dio %f", r.velocity);
}

static void test_one_frame_noise(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	// Un arrastre lento, con un salto de ruido de 12 px en un fotograma.
	down(&st, 300.0f, 400.0f);
	down(&st, 300.0f, 383.0f);   // pasa el umbral
	down(&st, 300.0f, 382.0f);
	down(&st, 300.0f, 370.0f);   // ruido
	down(&st, 300.0f, 369.0f);
	down(&st, 300.0f, 368.0f);
	Gesture_Result r = up(&st);

	float speed = r.velocity < 0.0f ? -r.velocity : r.velocity;
	CHECK(speed < 0.3f, "un fotograma con ruido no deberia llegar a 0,3 px/ms, dio %f", speed);
}

static void test_first_drag_frame_not_speed(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	// Se arrastra despacio hasta pasar el umbral y se suelta enseguida:
	// lo movido antes del umbral no cuenta como velocidad de un fotograma.
	down(&st, 300.0f, 400.0f);
	for (int i = 1; i <= 17; i++)
		down(&st, 300.0f, 400.0f - i);
	Gesture_Result r = up(&st);
	CHECK(r.type == GESTURE_RELEASE, "deberia dar RELEASE");
	CHECK(NEAR(r.velocity, -1.0f / FRAME_MS), "deberia ser 1 px por fotograma, dio %f", r.velocity);
}

static void test_caught(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	Gesture_Catch(&st);
	CHECK(!st.caught, "atrapar sin dedo no deberia marcar nada");

	down(&st, 300.0f, 300.0f);
	Gesture_Catch(&st);
	CHECK(!Gesture_Pressing(&st), "un toque que detuvo la lista no deberia resaltar");
	Gesture_Result r = up(&st);
	CHECK(r.type == GESTURE_RELEASE, "un toque atrapado deberia dar RELEASE, dio %d", r.type);
	CHECK(NEAR(r.velocity, 0.0f), "un toque atrapado no deberia tener velocidad");

	// El toque siguiente vuelve a ser normal.
	down(&st, 300.0f, 300.0f);
	r = up(&st);
	CHECK(r.type == GESTURE_TAP, "el toque siguiente deberia dar TAP, dio %d", r.type);
}

static void test_zero_dt(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	Gesture_Input in = { 1, 300.0f, 300.0f, 0.0f };
	Gesture_Update(&st, &in);
	in.y = 200.0f;
	Gesture_Update(&st, &in);
	Gesture_Result r = up(&st);
	CHECK(NEAR(r.velocity, 0.0f), "sin tiempo transcurrido la velocidad deberia ser 0, dio %f", r.velocity);
}

static void test_reset_forgets(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	down(&st, 300.0f, 300.0f);
	Gesture_Reset(&st);
	Gesture_Result r = up(&st);
	CHECK(r.type == GESTURE_NONE, "despues de Reset soltar no deberia dar nada, dio %d", r.type);
}

static Gesture_Result hold(Gesture_State *st, float x, float y) {
	Gesture_Input in = { 1, x, y, FRAME_MS, GESTURE_HOLD_MS };
	return Gesture_Update(st, &in);
}

// Apoya y deja el dedo quieto `ms` con el toque mantenido pedido; cuenta los HOLD.
static int hold_still(Gesture_State *st, float ms) {
	int holds = 0;

	hold(st, 300.0f, 300.0f);
	for (float t = 0.0f; t < ms; t += FRAME_MS)
		holds += hold(st, 302.0f, 301.0f).type == GESTURE_HOLD;

	return holds;
}

static void test_hold(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	// Un segundo quieto: un unico HOLD, en el apoyo, y levantar da RELEASE.
	int holds = 0, first = -1;
	Gesture_Result r = hold(&st, 300.0f, 300.0f);
	for (int i = 0; i < 60; i++) {
		r = hold(&st, 302.0f, 301.0f);
		if (r.type == GESTURE_HOLD) {
			holds++;
			if (first < 0)
				first = i + 1;
			CHECK(NEAR(r.start_x, 300.0f) && NEAR(r.start_y, 300.0f), "HOLD deberia informar el apoyo");
		}
	}
	CHECK(holds == 1, "un segundo quieto deberia dar un HOLD, dio %d", holds);
	CHECK(first * FRAME_MS >= GESTURE_HOLD_MS && (first - 1) * FRAME_MS < GESTURE_HOLD_MS, "el HOLD llego en el fotograma %d", first);
	CHECK(!Gesture_Pressing(&st), "despues del HOLD la fila no deberia verse apretada");
	r = up(&st);
	CHECK(r.type == GESTURE_RELEASE, "levantar despues del HOLD deberia dar RELEASE, dio %d", r.type);
}

static void test_hold_lift_before(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	CHECK(hold_still(&st, GESTURE_HOLD_MS - 3 * FRAME_MS) == 0, "levantar antes del umbral no deberia dar HOLD");
	Gesture_Result r = up(&st);
	CHECK(r.type == GESTURE_TAP, "levantar antes del umbral deberia dar TAP, dio %d", r.type);
}

static void test_hold_drag_before(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	// Arrastra a los 200 ms y sigue quieto despues: nunca es HOLD.
	int holds = hold_still(&st, 200.0f);
	Gesture_Result r = hold(&st, 300.0f, 340.0f);
	CHECK(r.type == GESTURE_DRAG, "pasar el umbral deberia dar DRAG, dio %d", r.type);
	for (int i = 0; i < 60; i++)
		holds += hold(&st, 300.0f, 340.0f).type == GESTURE_HOLD;
	CHECK(holds == 0, "un arrastre antes del umbral no deberia dar HOLD, dio %d", holds);
	r = up(&st);
	CHECK(r.type == GESTURE_RELEASE, "soltar el arrastre deberia dar RELEASE");
}

static void test_hold_off(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	// Sin hold_ms, como Carpetas y Ajustes: un segundo quieto sigue siendo TAP.
	int holds = 0;
	down(&st, 300.0f, 300.0f);
	for (int i = 0; i < 60; i++)
		holds += down(&st, 300.0f, 300.0f).type == GESTURE_HOLD;
	CHECK(holds == 0, "con hold_ms 0 no deberia dar HOLD, dio %d", holds);
	Gesture_Result r = up(&st);
	CHECK(r.type == GESTURE_TAP, "con hold_ms 0 un segundo quieto deberia dar TAP, dio %d", r.type);
}

static void test_hold_caught(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	// El toque que detiene una lista en movimiento no es un toque mantenido.
	hold(&st, 300.0f, 300.0f);
	Gesture_Catch(&st);
	int holds = 0;
	for (int i = 0; i < 60; i++)
		holds += hold(&st, 300.0f, 300.0f).type == GESTURE_HOLD;
	CHECK(holds == 0, "un toque atrapado no deberia dar HOLD, dio %d", holds);
	CHECK(up(&st).type == GESTURE_RELEASE, "un toque atrapado deberia dar RELEASE");
}

static void test_drag_after_hold(void) {
	Gesture_State st;
	Gesture_Reset(&st);

	CHECK(hold_still(&st, 600.0f) == 1, "deberia haber un HOLD");
	Gesture_Result r = hold(&st, 300.0f, 400.0f);
	CHECK(r.type == GESTURE_DRAG, "arrastrar despues del HOLD deberia dar DRAG, dio %d", r.type);
	r = hold(&st, 300.0f, 420.0f);
	CHECK(r.type == GESTURE_DRAG && NEAR(r.dy, 20.0f), "el arrastre deberia seguir, dio %d", r.type);
	r = up(&st);
	CHECK(r.type == GESTURE_RELEASE, "soltar el arrastre deberia dar RELEASE");

	// El toque siguiente empieza de cero.
	CHECK(hold_still(&st, 200.0f) == 0, "el toque siguiente no deberia arrastrar el tiempo anterior");
	CHECK(up(&st).type == GESTURE_TAP, "el toque siguiente deberia dar TAP");
}

int main(void) {
	test_idle();
	test_tap();
	test_drag_never_taps();
	test_horizontal_cancels_tap();
	test_lift_outside_reports_end();
	test_release_speed();
	test_still_before_release_no_speed();
	test_one_frame_noise();
	test_first_drag_frame_not_speed();
	test_caught();
	test_zero_dt();
	test_reset_forgets();
	test_hold();
	test_hold_lift_before();
	test_hold_drag_before();
	test_hold_off();
	test_hold_caught();
	test_drag_after_hold();

	if (failures == 0)
		printf("test_gesture: todo bien\n");

	return failures == 0 ? 0 : 1;
}
