// Pruebas en PC del giro del vinilo (source/viz_spin.c), compiladas con el gcc
// del host.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <math.h>
#include <stdio.h>

#include "viz_spin.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

#define DT (1.0f / 60)

static void test_target(void) {
	CHECK(fabsf(VizSpin_TargetSpeed() - 3.4907f) < 1e-3f, "33 1/3 rpm deberian ser 3,49 rad/s, dio %f", VizSpin_TargetSpeed());
}

static void test_spin_up(void) {
	VizSpin s;
	VizSpin_Reset(&s);

	float prev = 0.0f;
	int smooth = 1, frames = 0;
	while (s.speed < VizSpin_TargetSpeed() && frames < 600) {
		VizSpin_Update(&s, 1, DT);
		smooth &= s.speed > prev && s.speed - prev < 0.2f * VizSpin_TargetSpeed();
		prev = s.speed;
		frames++;
	}

	CHECK(smooth, "deberia acelerar de forma continua");
	CHECK(s.speed == VizSpin_TargetSpeed(), "deberia llegar a la velocidad, quedo en %f", s.speed);
	CHECK(frames * DT < 2.5f, "deberia llegar en menos de 2,5 s, tardo %f", frames * DT);
	CHECK(frames > 30, "no deberia arrancar de golpe (%d fotogramas)", frames);
}

static void test_brake(void) {
	VizSpin s;
	VizSpin_Reset(&s);
	for (int i = 0; i < 300; i++)
		VizSpin_Update(&s, 1, DT);

	float prev = s.speed;
	int smooth = 1, frames = 0;
	while (s.speed > 0.0f && frames < 600) {
		VizSpin_Update(&s, 0, DT);
		smooth &= s.speed < prev && (s.speed == 0.0f || prev - s.speed < 0.2f * VizSpin_TargetSpeed());
		prev = s.speed;
		frames++;
	}

	CHECK(smooth, "deberia frenar de forma continua");
	CHECK(s.speed == 0.0f, "deberia detenerse, quedo en %f", s.speed);
	CHECK(frames * DT < 2.5f, "deberia detenerse en menos de 2,5 s, tardo %f", frames * DT);
	CHECK(frames * DT > 1.0f, "no deberia parar en seco (%f s)", frames * DT);

	float angle = s.angle;
	for (int i = 0; i < 60; i++)
		VizSpin_Update(&s, 0, DT);
	CHECK(s.angle == angle, "detenido no deberia girar");
}

static void test_angle_bounded(void) {
	VizSpin s;
	VizSpin_Reset(&s);

	// Tres horas a 60 fps.
	int bounded = 1;
	for (long i = 0; i < 3L * 3600 * 60; i++) {
		VizSpin_Update(&s, 1, DT);
		bounded &= s.angle >= 0.0f && s.angle < 6.2832f;
	}
	CHECK(bounded, "el angulo deberia quedar en [0, 2pi)");

	// Despues de horas sigue avanzando lo mismo por fotograma.
	float a = s.angle;
	VizSpin_Update(&s, 1, DT);
	float step = s.angle - a;
	if (step < 0.0f)
		step += 6.2831853f;
	CHECK(fabsf(step - VizSpin_TargetSpeed() * DT) < 1e-4f, "el paso deberia ser %f, dio %f", VizSpin_TargetSpeed() * DT, step);
}

static void test_dt(void) {
	VizSpin s;
	VizSpin_Reset(&s);
	VizSpin_Update(&s, 1, 0.0f);
	VizSpin_Update(&s, 1, -1.0f);
	CHECK(s.speed == 0.0f && s.angle == 0.0f, "sin tiempo no deberia cambiar nada");
}

int main(void) {
	test_target();
	test_spin_up();
	test_brake();
	test_angle_bounded();
	test_dt();

	if (failures == 0)
		printf("test_viz_spin: todo bien\n");

	return failures == 0 ? 0 : 1;
}
