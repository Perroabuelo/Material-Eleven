// Pruebas en PC del avance de la cola (source/track_advance.c), compiladas con
// el gcc del host. track_advance.c es logica pura, asi que no hace falta el
// VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "track_advance.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

#define MAX_TRACKS 8

// Una cola de mentira: rutas en orden, cuales abren y en que puesto esta. Se
// mueve envolviendo por los extremos, como Queue_Advance.
typedef struct {
	const char *paths[MAX_TRACKS];
	int opens[MAX_TRACKS];
	int count;
	int pos;
	// Con 1, advance devuelve NULL siempre, como una cola que se vacio.
	int advance_null;

	int open_calls;
	int advance_calls;
	int null_opens;
	const char *opened;
} FakeQueue;

static int fake_open(void *ctx, const char *path) {
	FakeQueue *q = ctx;
	q->open_calls++;

	if (path == NULL) {
		q->null_opens++;
		return 0;
	}

	for (int i = 0; i < q->count; i++) {
		if (strcmp(q->paths[i], path) == 0 && q->opens[i]) {
			q->opened = q->paths[i];
			return 1;
		}
	}

	return 0;
}

static const char *fake_advance(void *ctx, int forward) {
	FakeQueue *q = ctx;
	q->advance_calls++;

	if (q->advance_null || q->count == 0)
		return NULL;

	q->pos = forward ? (q->pos + 1) % q->count : (q->pos + q->count - 1) % q->count;
	return q->paths[q->pos];
}

static const char *fake_current(void *ctx) {
	FakeQueue *q = ctx;
	return (q->count == 0) ? NULL : q->paths[q->pos];
}

static const char *const NAMES[MAX_TRACKS] = { "a", "b", "c", "d", "e", "f", "g", "h" };

// `opens` dice, pista por pista, si abre (1) o no (0).
static void fresh(FakeQueue *q, TrackAdvance_Ops *ops, int count, int pos, const int *opens) {
	memset(q, 0, sizeof(*q));
	q->count = count;
	q->pos = pos;

	for (int i = 0; i < count; i++) {
		q->paths[i] = NAMES[i];
		q->opens[i] = opens[i];
	}

	ops->open = fake_open;
	ops->advance = fake_advance;
	ops->current = fake_current;
	ops->ctx = q;
}

static void test_forward_opens_next(void) {
	FakeQueue q; TrackAdvance_Ops ops;
	const int opens[] = { 1, 1, 1 };
	fresh(&q, &ops, 3, 0, opens);

	int r = TrackAdvance_Run(&ops, 3, 1, 0);
	CHECK(r == 1, "avanzar deberia abrir una pista");
	CHECK(q.opened != NULL && strcmp(q.opened, "b") == 0, "deberia abrir b, abrio %s", q.opened ? q.opened : "(nada)");
	CHECK(q.open_calls == 1, "deberia intentar una vez, intento %d", q.open_calls);
}

static void test_replay_reopens_current(void) {
	FakeQueue q; TrackAdvance_Ops ops;
	const int opens[] = { 1, 1, 1 };
	fresh(&q, &ops, 3, 1, opens);

	int r = TrackAdvance_Run(&ops, 3, 1, 1);
	CHECK(r == 1, "repetir deberia abrir una pista");
	CHECK(q.opened != NULL && strcmp(q.opened, "b") == 0, "deberia reabrir b, abrio %s", q.opened ? q.opened : "(nada)");
	CHECK(q.advance_calls == 0, "repetir no deberia avanzar, avanzo %d veces", q.advance_calls);
	CHECK(q.pos == 1, "la cola deberia quedar en b, quedo en %d", q.pos);
}

static void test_backward_goes_back(void) {
	FakeQueue q; TrackAdvance_Ops ops;
	const int opens[] = { 1, 1, 1 };
	fresh(&q, &ops, 3, 0, opens);

	// Desde la primera, hacia atras envuelve a la ultima.
	int r = TrackAdvance_Run(&ops, 3, 0, 0);
	CHECK(r == 1, "retroceder deberia abrir una pista");
	CHECK(q.opened != NULL && strcmp(q.opened, "c") == 0, "deberia abrir c, abrio %s", q.opened ? q.opened : "(nada)");
}

static void test_skips_unreadable_same_direction(void) {
	FakeQueue q; TrackAdvance_Ops ops;
	const int opens[] = { 1, 0, 1, 1 };

	fresh(&q, &ops, 4, 0, opens);
	int r = TrackAdvance_Run(&ops, 4, 1, 0);
	CHECK(r == 1, "deberia saltar b y abrir c");
	CHECK(q.opened != NULL && strcmp(q.opened, "c") == 0, "hacia adelante deberia abrir c, abrio %s", q.opened ? q.opened : "(nada)");
	CHECK(q.open_calls == 2, "deberia intentar dos veces, intento %d", q.open_calls);

	// Hacia atras desde c, b no abre: sigue hacia atras hasta a.
	fresh(&q, &ops, 4, 2, opens);
	r = TrackAdvance_Run(&ops, 4, 0, 0);
	CHECK(r == 1, "deberia saltar b hacia atras y abrir a");
	CHECK(q.opened != NULL && strcmp(q.opened, "a") == 0, "hacia atras deberia abrir a, abrio %s", q.opened ? q.opened : "(nada)");

	// Repetir una pista que ya no abre sigue hacia adelante.
	fresh(&q, &ops, 4, 1, opens);
	r = TrackAdvance_Run(&ops, 4, 1, 1);
	CHECK(r == 1, "repetir una pista ilegible deberia pasar a la siguiente");
	CHECK(q.opened != NULL && strcmp(q.opened, "c") == 0, "deberia abrir c, abrio %s", q.opened ? q.opened : "(nada)");
}

static void test_single_track_queue(void) {
	FakeQueue q; TrackAdvance_Ops ops;
	const int opens[] = { 1 };

	fresh(&q, &ops, 1, 0, opens);
	int r = TrackAdvance_Run(&ops, 1, 1, 0);
	CHECK(r == 1, "una cola de una pista deberia volver a abrirla al avanzar");
	CHECK(q.opened != NULL && strcmp(q.opened, "a") == 0, "deberia abrir a, abrio %s", q.opened ? q.opened : "(nada)");

	fresh(&q, &ops, 1, 0, opens);
	r = TrackAdvance_Run(&ops, 1, 1, 1);
	CHECK(r == 1, "una cola de una pista deberia reabrirla al repetir");
	CHECK(q.advance_calls == 0, "repetir no deberia avanzar, avanzo %d veces", q.advance_calls);

	const int broken[] = { 0 };
	fresh(&q, &ops, 1, 0, broken);
	r = TrackAdvance_Run(&ops, 1, 1, 1);
	CHECK(r == 0, "una cola de una pista ilegible no deberia abrir nada");
	CHECK(q.open_calls == 1, "deberia intentar una sola vez, intento %d", q.open_calls);
}

static void test_none_opens(void) {
	FakeQueue q; TrackAdvance_Ops ops;
	const int opens[] = { 0, 0, 0, 0, 0 };

	for (int replay = 0; replay <= 1; replay++) {
		for (int forward = 0; forward <= 1; forward++) {
			fresh(&q, &ops, 5, 2, opens);
			int r = TrackAdvance_Run(&ops, 5, forward, replay);
			CHECK(r == 0, "sin pistas legibles deberia devolver 0 (forward %d, replay %d)", forward, replay);
			CHECK(q.open_calls == 5, "deberia intentar exactamente count veces, intento %d (forward %d, replay %d)", q.open_calls, forward, replay);
			CHECK(q.opened == NULL, "no deberia quedar nada abierto");
		}
	}
}

static void test_null_never_opened(void) {
	FakeQueue q; TrackAdvance_Ops ops;
	const int opens[] = { 1, 1, 1 };
	fresh(&q, &ops, 3, 0, opens);
	q.advance_null = 1;

	int r = TrackAdvance_Run(&ops, 3, 1, 0);
	CHECK(r == 0, "si advance devuelve NULL no deberia abrir nada");
	CHECK(q.null_opens == 0, "NULL no deberia pasarse a open, se paso %d veces", q.null_opens);
	CHECK(q.open_calls == 0, "no deberia llamar a open, lo llamo %d veces", q.open_calls);
	CHECK(q.advance_calls <= 4, "deberia avanzar como mucho una vuelta mas una, avanzo %d", q.advance_calls);
}

static void test_empty_queue(void) {
	FakeQueue q; TrackAdvance_Ops ops;
	fresh(&q, &ops, 0, 0, NULL);

	for (int replay = 0; replay <= 1; replay++) {
		int r = TrackAdvance_Run(&ops, 0, 1, replay);
		CHECK(r == 0, "una cola vacia deberia devolver 0");
	}

	CHECK(q.open_calls == 0, "una cola vacia no deberia intentar abrir, intento %d", q.open_calls);
	CHECK(q.advance_calls == 0, "una cola vacia no deberia avanzar, avanzo %d", q.advance_calls);
}

static void test_prev_restarts(void) {
	CHECK(TrackAdvance_PrevRestarts(0) == 0, "en 0 ms deberia pasar a la anterior");
	CHECK(TrackAdvance_PrevRestarts(TRACK_ADVANCE_RESTART_MS) == 0, "en 3000 ms justos deberia pasar a la anterior");
	CHECK(TrackAdvance_PrevRestarts(TRACK_ADVANCE_RESTART_MS + 1) == 1, "en 3001 ms deberia reiniciar");
	CHECK(TrackAdvance_PrevRestarts(UINT32_MAX) == 1, "en la posicion maxima deberia reiniciar");
}

int main(void) {
	test_forward_opens_next();
	test_replay_reopens_current();
	test_backward_goes_back();
	test_skips_unreadable_same_direction();
	test_single_track_queue();
	test_none_opens();
	test_null_never_opened();
	test_empty_queue();
	test_prev_restarts();

	if (failures == 0)
		printf("test_track_advance: todo bien\n");

	return failures == 0 ? 0 : 1;
}
