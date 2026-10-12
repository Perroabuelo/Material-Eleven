// Pruebas en PC del desplazamiento de listas (source/list_scroll.c),
// compiladas con el gcc del host. list_scroll.c es logica pura, asi que no
// hace falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "list_scroll.h"

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

// 100 filas de 60 px en un area de 300 px: el maximo es 5700.
static ListScroll make(int rows) {
	ListScroll s;
	memset(&s, 0, sizeof(s));
	ListScroll_SetSize(&s, rows, 60.0f, 300.0f);
	return s;
}

static void test_drag_and_ends(void) {
	ListScroll s = make(100);
	CHECK(NEAR(ListScroll_MaxOffset(&s), 5700.0f), "el maximo deberia ser 5700, dio %f", ListScroll_MaxOffset(&s));

	ListScroll_Drag(&s, 50.0f);
	CHECK(NEAR(s.offset, 0.0f), "arrastrar hacia abajo al principio no deberia moverla, dio %f", s.offset);

	ListScroll_Drag(&s, -200.0f);
	CHECK(NEAR(s.offset, 200.0f), "subir el dedo 200 px deberia bajar la vista 200, dio %f", s.offset);

	ListScroll_Drag(&s, -100000.0f);
	CHECK(NEAR(s.offset, 5700.0f), "no deberia pasarse del final, dio %f", s.offset);

	ListScroll_Drag(&s, -10.0f);
	CHECK(NEAR(s.offset, 5700.0f), "seguir arrastrando en el final no deberia moverla");
}

static void test_short_list(void) {
	ListScroll s = make(3);
	CHECK(NEAR(ListScroll_MaxOffset(&s), 0.0f), "una lista corta no deberia tener recorrido");

	ListScroll_Drag(&s, -100.0f);
	CHECK(NEAR(s.offset, 0.0f), "una lista corta no deberia desplazarse");

	ListScroll_Fling(&s, -5.0f);
	CHECK(!ListScroll_Moving(&s), "una lista corta no deberia tener inercia");
}

static void test_shrink_clamps(void) {
	ListScroll s = make(100);
	ListScroll_Drag(&s, -3000.0f);
	ListScroll_Fling(&s, -2.0f);

	// Se aplica un filtro y quedan 10 filas.
	ListScroll_SetSize(&s, 10, 60.0f, 300.0f);
	CHECK(NEAR(s.offset, 300.0f), "achicar la lista deberia acotar al nuevo maximo, dio %f", s.offset);
	CHECK(!ListScroll_Moving(&s), "acotar deberia detener la inercia");
}

static void test_inertia_monotone_and_finite(void) {
	ListScroll s = make(1000);
	ListScroll_Drag(&s, -1000.0f);
	ListScroll_Fling(&s, -3.0f);
	CHECK(ListScroll_Moving(&s), "un deslizamiento rapido deberia iniciar inercia");
	CHECK(s.velocity > 0.0f, "subir el dedo deberia bajar la vista");

	float prev_v = s.velocity, prev_off = s.offset;
	int steps = 0;
	while (ListScroll_Moving(&s) && steps < 100000) {
		ListScroll_Step(&s, FRAME_MS);
		steps++;
		CHECK(s.velocity < prev_v || s.velocity == 0.0f, "la velocidad deberia bajar en cada fotograma");
		CHECK(s.offset >= prev_off, "la vista deberia avanzar siempre en el mismo sentido");
		CHECK(s.offset >= 0.0f && s.offset <= ListScroll_MaxOffset(&s), "no deberia salir del rango");
		prev_v = s.velocity;
		prev_off = s.offset;
	}

	CHECK(steps < 1000, "la inercia deberia terminar pronto, tomo %d fotogramas", steps);
	CHECK(s.offset > 1000.0f + 300.0f, "deberia recorrer mas de una pantalla, quedo en %f", s.offset);
}

static void test_inertia_upwards(void) {
	ListScroll s = make(1000);
	ListScroll_Drag(&s, -20000.0f);
	ListScroll_Fling(&s, 3.0f);
	CHECK(s.velocity < 0.0f, "bajar el dedo deberia subir la vista");

	float start = s.offset;
	for (int i = 0; i < 10; i++)
		ListScroll_Step(&s, FRAME_MS);
	CHECK(s.offset < start, "la vista deberia subir");
}

static void test_slow_release_no_inertia(void) {
	ListScroll s = make(100);
	ListScroll_Fling(&s, -0.1f);
	CHECK(!ListScroll_Moving(&s), "soltar despacio no deberia iniciar inercia");
}

static void test_inertia_stops_at_end(void) {
	// Unas 15 filas: un deslizamiento fuerte se detiene en el final.
	ListScroll s = make(15);
	ListScroll_Fling(&s, -10.0f);

	int steps = 0;
	while (ListScroll_Moving(&s) && steps < 1000) {
		ListScroll_Step(&s, FRAME_MS);
		steps++;
	}

	CHECK(NEAR(s.offset, ListScroll_MaxOffset(&s)), "deberia quedar en el final, quedo en %f", s.offset);
	CHECK(!ListScroll_Moving(&s), "deberia detenerse al tocar el final");
}

static void test_big_dt(void) {
	ListScroll s = make(100);
	ListScroll_Drag(&s, -3000.0f);
	ListScroll_Fling(&s, -5.0f);
	ListScroll_Step(&s, 5000.0f);
	CHECK(s.offset >= 0.0f && s.offset <= ListScroll_MaxOffset(&s), "un fotograma largo no deberia sacarla del rango, dio %f", s.offset);
	CHECK(!isnan(s.offset) && !isnan(s.velocity), "no deberia dar NaN");

	ListScroll_Drag(&s, 3000.0f);
	ListScroll_Fling(&s, 5.0f);
	ListScroll_Step(&s, 5000.0f);
	CHECK(s.offset >= 0.0f, "un fotograma largo hacia arriba no deberia pasar del principio, dio %f", s.offset);

	// Un fotograma lento no frena la lista de golpe: dos de 16,67 ms y uno
	// de 33,33 ms dejan la misma velocidad.
	ListScroll a = make(1000), b = make(1000);
	ListScroll_Drag(&a, -1000.0f);
	ListScroll_Drag(&b, -1000.0f);
	ListScroll_Fling(&a, -2.0f);
	ListScroll_Fling(&b, -2.0f);
	ListScroll_Step(&a, FRAME_MS);
	ListScroll_Step(&a, FRAME_MS);
	ListScroll_Step(&b, 2.0f * FRAME_MS);
	CHECK(fabsf(a.velocity - b.velocity) < 0.001f, "la friccion deberia depender del tiempo: %f contra %f", a.velocity, b.velocity);
}

static void test_stop_and_drag_stop_inertia(void) {
	ListScroll s = make(100);
	ListScroll_Fling(&s, -2.0f);
	ListScroll_Stop(&s);
	CHECK(!ListScroll_Moving(&s), "Stop deberia detenerla");

	ListScroll_Fling(&s, -2.0f);
	ListScroll_Drag(&s, -1.0f);
	CHECK(!ListScroll_Moving(&s), "arrastrar deberia detener la inercia");
}

static void test_ensure_visible(void) {
	ListScroll s = make(100);

	// Ya visible: no se mueve.
	ListScroll_EnsureVisible(&s, 2);
	CHECK(NEAR(s.offset, 0.0f), "una fila visible no deberia mover la vista");

	// Debajo: queda al borde inferior.
	ListScroll_EnsureVisible(&s, 10);
	CHECK(NEAR(s.offset, 360.0f), "la fila 10 deberia quedar abajo (360), dio %f", s.offset);

	// Arriba: queda al borde superior.
	ListScroll_EnsureVisible(&s, 3);
	CHECK(NEAR(s.offset, 180.0f), "la fila 3 deberia quedar arriba (180), dio %f", s.offset);

	// A medias por arriba: se alinea arriba.
	s.offset = 200.0f;
	ListScroll_EnsureVisible(&s, 3);
	CHECK(NEAR(s.offset, 180.0f), "una fila a medias arriba deberia alinearse arriba, dio %f", s.offset);

	// A medias por abajo: se alinea abajo.
	s.offset = 200.0f;
	ListScroll_EnsureVisible(&s, 8);
	CHECK(NEAR(s.offset, 240.0f), "una fila a medias abajo deberia alinearse abajo, dio %f", s.offset);

	// La ultima, en el final.
	ListScroll_EnsureVisible(&s, 99);
	CHECK(NEAR(s.offset, 5700.0f), "la ultima deberia dejar la vista en el final, dio %f", s.offset);

	// Fuera de rango: se ignora.
	ListScroll_EnsureVisible(&s, 100);
	ListScroll_EnsureVisible(&s, -1);
	CHECK(NEAR(s.offset, 5700.0f), "un indice fuera de rango no deberia mover la vista");

	ListScroll_Fling(&s, 2.0f);
	ListScroll_EnsureVisible(&s, 99);
	CHECK(!ListScroll_Moving(&s), "EnsureVisible deberia detener la inercia");
}

static void test_row_at(void) {
	ListScroll s = make(100);

	CHECK(ListScroll_RowAt(&s, 0.0f) == 0, "y 0 deberia ser la fila 0");
	CHECK(ListScroll_RowAt(&s, 59.9f) == 0, "el borde de abajo de la fila 0 deberia ser 0");
	CHECK(ListScroll_RowAt(&s, 60.0f) == 1, "y 60 deberia ser la fila 1");
	CHECK(ListScroll_RowAt(&s, -1.0f) == -1, "encima del area deberia dar -1");
	CHECK(ListScroll_RowAt(&s, 300.0f) == -1, "debajo del area deberia dar -1");

	s.offset = 90.0f;
	CHECK(ListScroll_RowAt(&s, 0.0f) == 1, "desplazada 90, arriba deberia estar la fila 1");
	CHECK(ListScroll_RowAt(&s, 30.0f) == 2, "desplazada 90, y 30 deberia ser la fila 2");
	CHECK(ListScroll_FirstRow(&s) == 1, "la primera fila dibujada deberia ser la 1");
	CHECK(NEAR(ListScroll_RowY(&s, 1), -30.0f), "la fila 1 deberia dibujarse en -30");

	ListScroll shrt = make(3);
	CHECK(ListScroll_RowAt(&shrt, 200.0f) == -1, "debajo de la ultima fila deberia dar -1");
	CHECK(ListScroll_RowAt(&shrt, 179.0f) == 2, "la ultima fila deberia responder");

	ListScroll empty = make(0);
	CHECK(ListScroll_RowAt(&empty, 10.0f) == -1, "una lista vacia deberia dar -1");
	CHECK(ListScroll_FirstRow(&empty) == 0, "una lista vacia empieza en 0");
	ListScroll_EnsureVisible(&empty, 0);
	CHECK(NEAR(empty.offset, 0.0f), "EnsureVisible en una lista vacia no deberia moverla");
}

int main(void) {
	test_drag_and_ends();
	test_short_list();
	test_shrink_clamps();
	test_inertia_monotone_and_finite();
	test_inertia_upwards();
	test_slow_release_no_inertia();
	test_inertia_stops_at_end();
	test_big_dt();
	test_stop_and_drag_stop_inertia();
	test_ensure_visible();
	test_row_at();

	if (failures == 0)
		printf("test_list_scroll: todo bien\n");

	return failures == 0 ? 0 : 1;
}
