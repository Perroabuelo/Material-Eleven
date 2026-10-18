// Pruebas en PC del temporizador para dormir (source/sleep_timer.c),
// compiladas con el gcc del host. sleep_timer.c es logica pura y recibe el
// reloj desde afuera, asi que aqui el reloj es de mentira.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "sleep_timer.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

#define SEC  1000000ULL
#define MIN  (60 * SEC)
// Un fotograma a 60 Hz.
#define FRAME 16667ULL

// Un reloj que no empieza en 0, como el de la consola.
#define T0 (123 * SEC)

// Avanza fotograma a fotograma de `from` a `to` y cuenta los vencimientos.
// Devuelve en *at el instante del primero.
static int run(Sleep_Timer *t, uint64_t from, uint64_t to, uint64_t *at) {
	int expired = 0;
	float gain;

	for (uint64_t now = from; now <= to; now += FRAME) {
		if (SleepTimer_Tick(t, now, &gain) == SLEEP_TICK_EXPIRED) {
			if (expired == 0 && at != NULL)
				*at = now;
			expired++;
		}
	}

	return expired;
}

static void test_each_option_expires_once(void) {
	const Sleep_Option timed[] = { SLEEP_15, SLEEP_30, SLEEP_45, SLEEP_60, SLEEP_90 };
	const uint32_t minutes[] = { 15, 30, 45, 60, 90 };

	for (int i = 0; i < 5; i++) {
		Sleep_Timer t;
		SleepTimer_Set(&t, timed[i], T0);
		CHECK(SleepTimer_OptionMinutes(timed[i]) == minutes[i], "la opcion %d deberia contar %u minutos", i, minutes[i]);

		uint64_t deadline = T0 + minutes[i] * MIN;
		uint64_t at = 0;

		// Hasta un fotograma antes, nada.
		CHECK(run(&t, T0, deadline - FRAME, NULL) == 0, "%u min: no deberia vencer antes de tiempo", minutes[i]);
		CHECK(t.option == timed[i], "%u min: deberia seguir activo antes de vencer", minutes[i]);

		int n = run(&t, deadline - FRAME + 1, deadline + 5 * MIN, &at);
		CHECK(n == 1, "%u min: deberia vencer una sola vez, vencio %d", minutes[i], n);
		CHECK(at >= deadline && at < deadline + FRAME, "%u min: deberia vencer a su hora", minutes[i]);
		CHECK(t.option == SLEEP_OFF, "%u min: deberia quedar apagado", minutes[i]);
	}

	// Apagado y fin de pista no vencen por tiempo.
	const Sleep_Option untimed[] = { SLEEP_OFF, SLEEP_END_OF_TRACK };
	for (int i = 0; i < 2; i++) {
		Sleep_Timer t;
		SleepTimer_Set(&t, untimed[i], T0);
		CHECK(run(&t, T0, T0 + 100 * MIN, NULL) == 0, "%s no deberia vencer por tiempo", SleepTimer_Name(untimed[i]));
		CHECK(t.option == untimed[i], "%s deberia seguir igual", SleepTimer_Name(untimed[i]));
		CHECK(SleepTimer_OptionMinutes(untimed[i]) == 0, "%s no cuenta minutos", SleepTimer_Name(untimed[i]));
	}
}

static void test_fade_gain(void) {
	Sleep_Timer t;
	SleepTimer_Set(&t, SLEEP_15, T0);
	uint64_t deadline = T0 + 15 * MIN;
	float gain;

	SleepTimer_Tick(&t, T0, &gain);
	CHECK(gain == 1.0f, "al empezar la ganancia deberia ser 1, es %f", gain);

	SleepTimer_Tick(&t, deadline - 10 * SEC, &gain);
	CHECK(gain == 1.0f, "a 10 s justos la ganancia deberia ser 1, es %f", gain);

	SleepTimer_Tick(&t, deadline - 5 * SEC, &gain);
	CHECK(fabsf(gain - 0.5f) < 1e-4f, "a 5 s la ganancia deberia ser 0.5, es %f", gain);

	SleepTimer_Tick(&t, deadline - 1 * SEC, &gain);
	CHECK(fabsf(gain - 0.1f) < 1e-4f, "a 1 s la ganancia deberia ser 0.1, es %f", gain);

	// Lineal y sin subir en todo el fundido.
	SleepTimer_Set(&t, SLEEP_15, T0);
	float prev = 1.0f;
	int rose = 0;
	for (uint64_t now = deadline - 11 * SEC; now < deadline; now += FRAME) {
		SleepTimer_Tick(&t, now, &gain);
		if (gain > prev)
			rose++;

		float expected = (now + 10 * SEC >= deadline) ? (float)(deadline - now) / (float)(10 * SEC) : 1.0f;
		CHECK(fabsf(gain - expected) < 1e-4f, "a %llu us del final la ganancia deberia ser %f, es %f",
			(unsigned long long)(deadline - now), expected, gain);
		prev = gain;
	}
	CHECK(rose == 0, "la ganancia no deberia subir durante el fundido, subio %d veces", rose);

	Sleep_Tick r = SleepTimer_Tick(&t, deadline, &gain);
	CHECK(r == SLEEP_TICK_EXPIRED && gain == 0.0f, "al vencer la ganancia deberia ser 0, es %f", gain);

	SleepTimer_Tick(&t, deadline + FRAME, &gain);
	CHECK(gain == 1.0f, "apagado la ganancia deberia volver a 1, es %f", gain);
}

static void test_minutes_left(void) {
	Sleep_Timer t;
	SleepTimer_Set(&t, SLEEP_15, T0);
	uint64_t deadline = T0 + 15 * MIN;

	CHECK(SleepTimer_MinutesLeft(&t, T0) == 15, "con 15:00 deberia mostrar 15, muestra %u", SleepTimer_MinutesLeft(&t, T0));
	CHECK(SleepTimer_MinutesLeft(&t, T0 + 30 * SEC) == 15, "con 14:30 deberia mostrar 15");
	CHECK(SleepTimer_MinutesLeft(&t, T0 + 1 * SEC) == 15, "con 14:59 deberia mostrar 15");
	CHECK(SleepTimer_MinutesLeft(&t, T0 + 1 * MIN) == 14, "con 14:00 deberia mostrar 14, muestra %u", SleepTimer_MinutesLeft(&t, T0 + MIN));
	CHECK(SleepTimer_MinutesLeft(&t, deadline - 20 * SEC) == 1, "con 0:20 deberia mostrar 1, muestra %u", SleepTimer_MinutesLeft(&t, deadline - 20 * SEC));
	CHECK(SleepTimer_MinutesLeft(&t, deadline - 1) == 1, "a un microsegundo del final deberia mostrar 1");
	CHECK(SleepTimer_MinutesLeft(&t, deadline) == 0, "vencido deberia mostrar 0");

	SleepTimer_Set(&t, SLEEP_90, T0);
	CHECK(SleepTimer_MinutesLeft(&t, T0) == 90, "90 min deberia mostrar 90");

	SleepTimer_Set(&t, SLEEP_END_OF_TRACK, T0);
	CHECK(SleepTimer_MinutesLeft(&t, T0) == 0, "fin de pista no tiene minutos");
	SleepTimer_Set(&t, SLEEP_OFF, T0);
	CHECK(SleepTimer_MinutesLeft(&t, T0) == 0, "apagado no tiene minutos");
}

static void test_replace_and_cancel_during_fade(void) {
	Sleep_Timer t;
	float gain;

	// Elegir 60 a mitad del fundido de 15 cuenta 60 desde ahi.
	SleepTimer_Set(&t, SLEEP_15, T0);
	uint64_t mid = T0 + 15 * MIN - 5 * SEC;
	SleepTimer_Tick(&t, mid, &gain);
	CHECK(gain < 1.0f, "a 5 s del final deberia estar fundiendo");

	SleepTimer_Set(&t, SLEEP_60, mid);
	SleepTimer_Tick(&t, mid + FRAME, &gain);
	CHECK(gain == 1.0f, "reemplazar deberia devolver la ganancia a 1, es %f", gain);
	CHECK(SleepTimer_MinutesLeft(&t, mid) == 60, "reemplazar deberia contar 60 desde la eleccion");
	CHECK(run(&t, mid, mid + 60 * MIN - FRAME, NULL) == 0, "el de 15 no deberia vencer despues de reemplazarlo");
	CHECK(run(&t, mid + 60 * MIN - FRAME + 1, mid + 61 * MIN, NULL) == 1, "el de 60 deberia vencer a su hora");

	// Reemplazar 15 por 15 dos minutos despues empieza de nuevo.
	SleepTimer_Set(&t, SLEEP_15, T0);
	SleepTimer_Set(&t, SLEEP_15, T0 + 2 * MIN);
	CHECK(SleepTimer_MinutesLeft(&t, T0 + 2 * MIN) == 15, "volver a elegir 15 deberia reiniciar la cuenta");

	// Apagado a mitad del fundido cancela sin vencer.
	SleepTimer_Set(&t, SLEEP_15, T0);
	SleepTimer_Tick(&t, mid, &gain);
	SleepTimer_Set(&t, SLEEP_OFF, mid);
	CHECK(SleepTimer_Tick(&t, mid + FRAME, &gain) == SLEEP_TICK_NONE && gain == 1.0f, "cancelar deberia volver a ganancia 1 sin vencer");
	CHECK(run(&t, mid, mid + 10 * MIN, NULL) == 0, "un temporizador cancelado no deberia vencer");
	CHECK(t.option == SLEEP_OFF, "deberia quedar apagado");

	// Una opcion fuera de rango se toma como apagado.
	SleepTimer_Set(&t, SLEEP_OPTION_COUNT, T0);
	CHECK(t.option == SLEEP_OFF && t.deadline_us == 0, "una opcion invalida deberia apagar");
}

static void test_track_ended(void) {
	Sleep_Timer t;

	SleepTimer_Set(&t, SLEEP_END_OF_TRACK, T0);
	CHECK(SleepTimer_TrackEnded(&t) == 1, "en fin de pista deberia responder 1");
	CHECK(t.option == SLEEP_OFF, "y quedar apagado");
	CHECK(SleepTimer_TrackEnded(&t) == 0, "la segunda pista que termina ya no deberia responder");

	const Sleep_Option others[] = { SLEEP_OFF, SLEEP_15, SLEEP_90 };
	for (int i = 0; i < 3; i++) {
		SleepTimer_Set(&t, others[i], T0);
		CHECK(SleepTimer_TrackEnded(&t) == 0, "%s no deberia responder al fin de pista", SleepTimer_Name(others[i]));
		CHECK(t.option == others[i], "%s deberia seguir igual", SleepTimer_Name(others[i]));
	}
}

static void test_clock_jump(void) {
	Sleep_Timer t;
	float gain = 1.0f;

	// Un salto que pasa de largo todo el fundido vence una vez, con ganancia 0.
	SleepTimer_Set(&t, SLEEP_15, T0);
	SleepTimer_Tick(&t, T0 + MIN, &gain);
	CHECK(SleepTimer_Tick(&t, T0 + 3 * 60 * MIN, &gain) == SLEEP_TICK_EXPIRED, "un salto de reloj deberia vencer");
	CHECK(gain == 0.0f, "con ganancia 0, es %f", gain);
	CHECK(SleepTimer_Tick(&t, T0 + 3 * 60 * MIN + FRAME, &gain) == SLEEP_TICK_NONE, "y no volver a vencer");
	CHECK(SleepTimer_Tick(&t, T0 + 6 * 60 * MIN, &gain) == SLEEP_TICK_NONE, "ni con otro salto");
}

static void test_names(void) {
	const char *expected[SLEEP_OPTION_COUNT] = { "off", "15", "30", "45", "60", "90", "track" };

	for (int o = 0; o < SLEEP_OPTION_COUNT; o++)
		CHECK(strcmp(SleepTimer_Name((Sleep_Option)o), expected[o]) == 0, "la opcion %d deberia llamarse %s, se llama %s",
			o, expected[o], SleepTimer_Name((Sleep_Option)o));

	CHECK(strcmp(SleepTimer_Name(SLEEP_OPTION_COUNT), "off") == 0, "una opcion invalida deberia llamarse off");
}

int main(void) {
	test_each_option_expires_once();
	test_fade_gain();
	test_minutes_left();
	test_replace_and_cancel_during_fade();
	test_track_ended();
	test_clock_jump();
	test_names();

	if (failures == 0)
		printf("test_sleep_timer: todo bien\n");

	return failures == 0 ? 0 : 1;
}
