// Pruebas en PC de las suites de escenarios (source/script.c), compiladas con
// el gcc del host. script.c es logica pura, asi que no hace falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "script.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// script.c se compila con -DSCRIPT_TEST_ALLOC y reserva por aqui.
static int fail_next = 0;

void *Script_TestMalloc(size_t size) {
	if (fail_next) {
		fail_next = 0;
		return NULL;
	}

	return malloc(size);
}

// Las mascaras de psp2/ctrl.h, repetidas para no depender de los headers de la
// consola, y los nombres del log.
#define BTN_SELECT   0x00000001
#define BTN_START    0x00000008
#define BTN_UP       0x00000010
#define BTN_RIGHT    0x00000020
#define BTN_DOWN     0x00000040
#define BTN_LEFT     0x00000080
#define BTN_L        0x00000100
#define BTN_R        0x00000200
#define BTN_TRIANGLE 0x00001000
#define BTN_CIRCLE   0x00002000
#define BTN_CROSS    0x00004000
#define BTN_SQUARE   0x00008000

static const LogButtonName buttons[] = {
	{ BTN_CROSS,    "CROSS"    },
	{ BTN_CIRCLE,   "CIRCLE"   },
	{ BTN_SQUARE,   "SQUARE"   },
	{ BTN_TRIANGLE, "TRIANGLE" },
	{ BTN_UP,       "UP"       },
	{ BTN_DOWN,     "DOWN"     },
	{ BTN_LEFT,     "LEFT"     },
	{ BTN_RIGHT,    "RIGHT"    },
	{ BTN_L,        "L"        },
	{ BTN_R,        "R"        },
	{ BTN_START,    "START"    },
	{ BTN_SELECT,   "SELECT"   },
};
#define BUTTON_COUNT ((int)(sizeof(buttons) / sizeof(buttons[0])))

// Parsea `len` bytes copiados a un bloque de ese tamano exacto, sin NUL
// detras, para que ASan vea cualquier lectura de mas.
static Script *parse_exact(const char *text, unsigned int len, ScriptError *err) {
	char *copy = malloc(len ? len : 1);
	memcpy(copy, text, len);
	Script *script = Script_Parse(copy, len, buttons, BUTTON_COUNT, err);
	free(copy);
	return script;
}

static Script *parse(const char *text, ScriptError *err) {
	return parse_exact(text, (unsigned int)strlen(text), err);
}

// La condicion `i` de un wait.
static const char *cond_key(const ScriptStep *step, int i) {
	const char *p = step->cond;
	for (int k = 0; k < 2 * i; k++)
		p += strlen(p) + 1;
	return p;
}

static const char *cond_value(const ScriptStep *step, int i) {
	const char *key = cond_key(step, i);
	return key + strlen(key) + 1;
}

// El escenario "Guion valido".
static void test_valid(void) {
	const char *text =
		"# Una suite de prueba\n"
		"\n"
		"scenario Bajar dos filas   # el nombre lleva espacios\n"
		"  wait library view=songs\n"
		"\tpress DOWN x2\n"
		"wait library sel=2 timeout=1s\n"
		"hold L+R+SELECT 2s\n"
		"\n"
		"scenario Toques\r\n"
		"tap 480 272\r\n"
		"swipe 0 0 959 543 250ms\r\n"
		"sleep 100ms\r\n"
		"seed 42\r\n"
		"wait nowplaying file=\"ux0:/Mis cosas/#1 \\\"Hi\\\".mp3\" paused=0 # comentario\n"
		"press CROSS";
	ScriptError err;
	Script *script = parse(text, &err);

	CHECK(script != NULL, "el guion valido no parsea: linea %d, %s", err.line, err.reason);
	if (!script)
		return;

	CHECK(script->scenario_count == 2, "deberia tener 2 escenarios, tiene %d", script->scenario_count);
	CHECK(script->step_count == 12, "deberia tener 12 pasos, tiene %d", script->step_count);

	int n = 0;
	const ScriptStep *first = Script_Scenario(script, 0, &n);
	CHECK(first && first->cmd == SCRIPT_SCENARIO && strcmp(first->arg, "Bajar dos filas") == 0 && first->line == 3,
		"primer escenario: '%s' en la linea %d", first ? first->arg : "(nada)", first ? first->line : 0);
	CHECK(n == 4, "el primer escenario deberia tener 4 pasos, tiene %d", n);

	const ScriptStep *s = first + 1;
	CHECK(s->cmd == SCRIPT_WAIT && s->line == 4 && strcmp(s->arg, "library") == 0 && s->cond_count == 1
		&& strcmp(cond_key(s, 0), "view") == 0 && strcmp(cond_value(s, 0), "songs") == 0
		&& s->duration_ms == SCRIPT_WAIT_DEFAULT_MS, "wait library view=songs mal parseado");
	CHECK(strcmp(s->text, "wait library view=songs") == 0, "texto del wait: '%s'", s->text);

	s++;
	CHECK(s->cmd == SCRIPT_PRESS && s->buttons == BTN_DOWN && s->repeat == 2 && strcmp(s->text, "press DOWN x2") == 0,
		"press DOWN x2 mal parseado: botones %x, %d veces, '%s'", s->buttons, s->repeat, s->text);

	s++;
	CHECK(s->cmd == SCRIPT_WAIT && s->cond_count == 1 && strcmp(cond_value(s, 0), "2") == 0 && s->duration_ms == 1000,
		"wait con timeout=1s mal parseado: %u ms", s->duration_ms);

	s++;
	CHECK(s->cmd == SCRIPT_HOLD && s->buttons == (BTN_L | BTN_R | BTN_SELECT) && s->duration_ms == 2000,
		"hold L+R+SELECT 2s mal parseado: %x, %u ms", s->buttons, s->duration_ms);

	const ScriptStep *second = Script_Scenario(script, 1, &n);
	CHECK(second && strcmp(second->arg, "Toques") == 0 && n == 6,
		"segundo escenario: '%s' con %d pasos", second ? second->arg : "(nada)", n);
	if (!second) {
		Script_Free(script);
		return;
	}

	s = second + 1;
	CHECK(s->cmd == SCRIPT_TAP && s->x1 == 480 && s->y1 == 272 && strcmp(s->text, "tap 480 272") == 0,
		"tap mal parseado: %d,%d '%s'", s->x1, s->y1, s->text);
	s++;
	CHECK(s->cmd == SCRIPT_SWIPE && s->x1 == 0 && s->y1 == 0 && s->x2 == 959 && s->y2 == 543 && s->duration_ms == 250,
		"swipe mal parseado");
	s++;
	CHECK(s->cmd == SCRIPT_SLEEP && s->duration_ms == 100, "sleep mal parseado: %u ms", s->duration_ms);
	s++;
	CHECK(s->cmd == SCRIPT_SEED && s->seed == 42, "seed mal parseado: %u", s->seed);
	s++;
	CHECK(s->cmd == SCRIPT_WAIT && s->line == 14 && s->cond_count == 2
		&& strcmp(cond_value(s, 0), "ux0:/Mis cosas/#1 \"Hi\".mp3") == 0
		&& strcmp(cond_key(s, 1), "paused") == 0 && strcmp(cond_value(s, 1), "0") == 0,
		"wait con comillas mal parseado: '%s'", s->cond_count ? cond_value(s, 0) : "");
	s++;
	CHECK(s->cmd == SCRIPT_PRESS && s->buttons == BTN_CROSS && s->repeat == 1 && s->line == 15,
		"la ultima linea sin salto deberia parsear");

	CHECK(Script_Scenario(script, 2, &n) == NULL && n == 0, "no hay tercer escenario");

	Script_Free(script);
}

static void check_error(const char *text, int line, const char *reason_part) {
	ScriptError err;
	Script *script = parse(text, &err);

	CHECK(script == NULL, "deberia fallar: '%s'", text);
	Script_Free(script);
	if (script)
		return;

	CHECK(err.line == line, "'%s': deberia fallar en la linea %d, fallo en la %d (%s)", text, line, err.line, err.reason);
	CHECK(strstr(err.reason, reason_part) != NULL, "'%s': el motivo '%s' deberia decir '%s'", text, err.reason, reason_part);
}

static void test_errors(void) {
	// El escenario "Boton desconocido".
	check_error("scenario a\n# 2\n# 3\n\n# 5\nwait screen to=folders\npress TRIANGULO\n", 7, "TRIANGULO");
	// El guion de dev-console-scripts-error.txt.
	check_error("# fixtures: ninguno\nscenario Error\npresss CROSS\n", 3, "presss");

	check_error("scenario a\npress\n", 2, "missing arguments");
	check_error("scenario a\npress CROSS x2 x3\n", 2, "too many arguments");
	check_error("scenario a\npress CROSS+\n", 2, "invalid buttons");
	check_error("scenario a\npress +CROSS\n", 2, "invalid buttons");
	check_error("scenario a\npress cross\n", 2, "cross");
	check_error("scenario a\npress CROSS x0\n", 2, "invalid repeat");
	check_error("scenario a\npress CROSS 2\n", 2, "invalid repeat");
	check_error("scenario a\npress CROSS x1001\n", 2, "invalid repeat");
	check_error("scenario a\nhold DOWN\n", 2, "missing arguments");
	check_error("scenario a\nhold DOWN 2\n", 2, "invalid duration");
	check_error("scenario a\nhold DOWN 0s\n", 2, "invalid duration");
	check_error("scenario a\nhold DOWN -1s\n", 2, "invalid duration");
	check_error("scenario a\nhold DOWN 1.5s\n", 2, "invalid duration");
	check_error("scenario a\nhold DOWN 3601s\n", 2, "invalid duration");
	check_error("scenario a\nhold DOWN 99999999999999999999ms\n", 2, "invalid duration");
	check_error("scenario a\nsleep ms\n", 2, "invalid duration");
	check_error("scenario a\ntap 960 0\n", 2, "x out of range");
	check_error("scenario a\ntap 0 544\n", 2, "y out of range");
	check_error("scenario a\ntap -1 0\n", 2, "invalid coordinate");
	check_error("scenario a\ntap 1 2 3\n", 2, "too many arguments");
	check_error("scenario a\nswipe 0 0 959 543\n", 2, "missing arguments");
	check_error("scenario a\nswipe 0 0 959 600 1s\n", 2, "y out of range");
	check_error("scenario a\nwait\n", 2, "missing arguments");
	check_error("scenario a\nwait library\n", 2, "missing arguments");
	check_error("scenario a\nwait sel=2\n", 2, "missing category");
	check_error("scenario a\nwait library sel\n", 2, "invalid condition");
	check_error("scenario a\nwait library sel=\n", 2, "invalid condition");
	check_error("scenario a\nwait library name=\"abc\n", 2, "invalid condition");
	check_error("scenario a\nwait library name=\"a\\tb\"\n", 2, "invalid condition");
	check_error("scenario a\nwait library timeout=1s\n", 2, "missing condition");
	check_error("scenario a\nwait library sel=1 timeout=1s timeout=2s\n", 2, "twice");
	check_error("scenario a\nwait library sel=1 timeout=1\n", 2, "invalid duration");
	check_error("scenario a\nseed\n", 2, "missing arguments");
	check_error("scenario a\nseed 4294967296\n", 2, "invalid seed");
	check_error("scenario a\nseed -1\n", 2, "invalid seed");
	check_error("scenario\n", 1, "missing scenario name");
	check_error("press CROSS\nscenario a\n", 1, "before the first scenario");
	check_error("\n# solo comentarios\n\n", 0, "no scenarios");
	check_error("", 0, "no scenarios");

	// Un NUL en medio de una linea.
	const char nul[] = "scenario a\npress CROSS\0\n";
	ScriptError err;
	Script *script = parse_exact(nul, sizeof(nul) - 1, &err);
	CHECK(script == NULL && err.line == 2, "un NUL deberia fallar en la linea 2, fallo en la %d", err.line);
	Script_Free(script);

	// Sin memoria.
	fail_next = 1;
	script = parse("scenario a\n", &err);
	CHECK(script == NULL && err.line == 0 && strstr(err.reason, "memory"), "sin memoria deberia fallar: '%s'", err.reason);
	Script_Free(script);
}

static void test_limits(void) {
	ScriptError err;

	// 1024 pasos entran; uno mas falla en su linea.
	unsigned int size = 16 * SCRIPT_STEPS_MAX + 64;
	char *text = malloc(size);
	unsigned int n = (unsigned int)snprintf(text, size, "scenario a\n");
	for (int i = 1; i < SCRIPT_STEPS_MAX; i++)
		n += (unsigned int)snprintf(text + n, size - n, "press CROSS\n");

	Script *script = parse_exact(text, n, &err);
	CHECK(script && script->step_count == SCRIPT_STEPS_MAX, "%d pasos deberian entrar: linea %d, %s",
		SCRIPT_STEPS_MAX, err.line, err.reason);
	Script_Free(script);

	n += (unsigned int)snprintf(text + n, size - n, "press CROSS\n");
	script = parse_exact(text, n, &err);
	CHECK(script == NULL && err.line == SCRIPT_STEPS_MAX + 1 && strstr(err.reason, "1024"),
		"el paso %d deberia fallar en su linea: linea %d, %s", SCRIPT_STEPS_MAX + 1, err.line, err.reason);
	Script_Free(script);
	free(text);

	// El maximo, con lineas largas, sin escribir fuera del bloque.
	text = malloc(SCRIPT_TEXT_MAX);
	n = (unsigned int)snprintf(text, SCRIPT_TEXT_MAX, "scenario a\n");
	int steps = 1;
	while (steps < SCRIPT_STEPS_MAX) {
		const char *line = "wait nowplaying file=\"\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\" paused=0 x=1 y=2\n";
		unsigned int len = (unsigned int)strlen(line);
		if (n + len > SCRIPT_TEXT_MAX)
			break;
		memcpy(text + n, line, len);
		n += len;
		steps++;
	}
	script = parse_exact(text, n, &err);
	CHECK(script != NULL, "un guion largo deberia parsear: linea %d, %s", err.line, err.reason);
	CHECK(!script || script->pool_used <= script->pool_size, "el pool se paso de su tamano");
	Script_Free(script);

	// 64 KB de basura: falla sin leer ni escribir fuera.
	srand(1234);
	for (int round = 0; round < 50; round++) {
		for (unsigned int i = 0; i < SCRIPT_TEXT_MAX; i++)
			text[i] = (char)(rand() & 0xFF);
		// Sin NUL, para que la basura llegue mas lejos que la primera linea.
		if (round % 2)
			for (unsigned int i = 0; i < SCRIPT_TEXT_MAX; i++)
				if (text[i] == '\0')
					text[i] = ' ';
		script = parse_exact(text, SCRIPT_TEXT_MAX, &err);
		CHECK(script == NULL, "64 KB de basura no deberian parsear");
		Script_Free(script);
	}

	// Basura con forma de guion: palabras validas en desorden.
	static const char *words[] = { "scenario", "press", "wait", "tap", "\"", "\\", "#", "=", "x2", "CROSS",
		"+", "1s", "959", "\n", " ", "\t", "\r", "library", "sel=2", "timeout=", "\"a b\"" };
	for (int round = 0; round < 200; round++) {
		n = 0;
		while (1) {
			const char *w = words[rand() % (int)(sizeof(words) / sizeof(words[0]))];
			unsigned int len = (unsigned int)strlen(w);
			if (n + len > 4096)
				break;
			memcpy(text + n, w, len);
			n += len;
		}
		script = parse_exact(text, n, &err);
		if (script)
			CHECK(script->pool_used <= script->pool_size, "el pool se paso de su tamano");
		Script_Free(script);
	}

	free(text);

	// Uno mas que el maximo, aunque sea valido.
	text = malloc(SCRIPT_TEXT_MAX + 1);
	memset(text, '\n', SCRIPT_TEXT_MAX + 1);
	memcpy(text, "scenario a\n", 11);
	script = parse_exact(text, SCRIPT_TEXT_MAX + 1, &err);
	CHECK(script == NULL && err.line == 0 && strstr(err.reason, "64 KB"), "mas de 64 KB deberia fallar: '%s'", err.reason);
	Script_Free(script);
	free(text);
}

int main(void) {
	test_valid();
	test_errors();
	test_limits();

	if (failures == 0)
		printf("test_script: todo bien\n");

	return failures == 0 ? 0 : 1;
}
