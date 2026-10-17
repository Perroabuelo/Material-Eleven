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

// Un fotograma cada 16 ms, como la consola.
#define FRAME_US 16667ULL

// El estado de las pantallas que el lado de la consola guardaria.
typedef struct {
	const char *category;
	const char *body;
} FakeState;

static const char *fake_last_state(const char *category, unsigned int *len, void *ctx) {
	const FakeState *state = ctx;

	if (!state || !state->body || strcmp(state->category, category) != 0)
		return NULL;

	*len = (unsigned int)strlen(state->body);
	return state->body;
}

// Una suite de un escenario con `steps` como pasos.
static Script *one_scenario(const char *steps) {
	char text[4096];
	ScriptError err;

	snprintf(text, sizeof(text), "scenario prueba\n%s", steps);
	Script *script = parse(text, &err);
	CHECK(script != NULL, "'%s' no parsea: linea %d, %s", steps, err.line, err.reason);
	return script;
}

static void observe(ScriptRun *run, const char *line) {
	ScriptRun_Observe(run, line, (unsigned int)strlen(line));
}

static void test_run_press(void) {
	Script *script = one_scenario("press DOWN x2\n");
	if (!script)
		return;

	ScriptRun run;
	ScriptRun_Init(&run, script, 0, NULL, NULL);

	unsigned int want[] = { BTN_DOWN, 0, BTN_DOWN, 0 };
	uint64_t now = 1000000;
	for (int i = 0; i < 4; i++, now += FRAME_US) {
		ScriptFrame f = ScriptRun_Tick(&run, now);
		CHECK(f.buttons == want[i] && f.result == SCRIPT_RUNNING, "press DOWN x2, fotograma %d: %x", i, f.buttons);
		CHECK((i == 0) == (f.started != NULL), "press DOWN x2, fotograma %d: el paso deberia empezar solo en el primero", i);
	}

	ScriptFrame f = ScriptRun_Tick(&run, now);
	CHECK(f.result == SCRIPT_PASS && f.buttons == 0, "press DOWN x2 deberia terminar con pass despues de 4 fotogramas");
	f = ScriptRun_Tick(&run, now + FRAME_US);
	CHECK(f.result == SCRIPT_PASS && f.started == NULL, "despues del final, sigue pass");

	Script_Free(script);
}

static void test_run_hold(void) {
	Script *script = one_scenario("hold DOWN 2s\npress CROSS\n");
	if (!script)
		return;

	ScriptRun run;
	ScriptRun_Init(&run, script, 0, NULL, NULL);

	uint64_t start = 5000000, now = start;
	ScriptFrame f;
	int held = 0;
	while (1) {
		f = ScriptRun_Tick(&run, now);
		if (f.buttons != BTN_DOWN)
			break;
		held++;
		now += FRAME_US;
	}

	CHECK(now - start >= 2000000 && now - start < 2000000 + FRAME_US,
		"hold DOWN 2s deberia soltar a los 2 s, solto a los %llu us", (unsigned long long)(now - start));
	CHECK(f.buttons == 0 && f.started == NULL, "hold: un fotograma sin botones al final");
	CHECK(held > 100, "hold: %d fotogramas con DOWN", held);

	f = ScriptRun_Tick(&run, now + FRAME_US);
	CHECK(f.buttons == BTN_CROSS && f.started && f.started->cmd == SCRIPT_PRESS, "despues del hold empieza el press");

	Script_Free(script);
}

static void test_run_tap_swipe(void) {
	Script *script = one_scenario("tap 480 272\nswipe 0 100 200 300 1s\n");
	if (!script)
		return;

	ScriptRun run;
	ScriptRun_Init(&run, script, 0, NULL, NULL);

	uint64_t start = 0, now = start;
	ScriptFrame f;
	while (1) {
		f = ScriptRun_Tick(&run, now);
		if (!f.touching)
			break;
		CHECK(f.x == 480 && f.y == 272, "tap: el dedo en %d,%d", f.x, f.y);
		now += 1000;
	}
	CHECK(now - start == SCRIPT_TAP_MS * 1000, "tap deberia durar %d ms, duro %llu us", SCRIPT_TAP_MS,
		(unsigned long long)(now - start));
	CHECK(f.started == NULL, "tap: el fotograma sin dedo no empieza otro paso");

	start = now + FRAME_US;
	f = ScriptRun_Tick(&run, start);
	CHECK(f.started && f.started->cmd == SCRIPT_SWIPE && f.touching && f.x == 0 && f.y == 100,
		"swipe: empieza en 0,100, dio %d,%d", f.x, f.y);
	f = ScriptRun_Tick(&run, start + 500000);
	CHECK(f.touching && f.x == 100 && f.y == 200, "swipe: a la mitad en 100,200, dio %d,%d", f.x, f.y);
	f = ScriptRun_Tick(&run, start + 999000);
	CHECK(f.touching && f.x == 199 && f.y == 299, "swipe: casi al final en 199,299, dio %d,%d", f.x, f.y);
	f = ScriptRun_Tick(&run, start + 1200000);
	CHECK(f.touching && f.x == 200 && f.y == 300, "swipe: el extremo 200,300, dio %d,%d", f.x, f.y);
	f = ScriptRun_Tick(&run, start + 1216000);
	CHECK(!f.touching && f.result == SCRIPT_RUNNING, "swipe: un fotograma sin dedo al final");
	f = ScriptRun_Tick(&run, start + 1232000);
	CHECK(f.result == SCRIPT_PASS, "swipe: despues termina el escenario");

	Script_Free(script);
}

static void test_run_sleep_seed(void) {
	Script *script = one_scenario("seed 7\nsleep 500ms\npress CROSS\n");
	if (!script)
		return;

	ScriptRun run;
	ScriptRun_Init(&run, script, 0, NULL, NULL);

	ScriptFrame f = ScriptRun_Tick(&run, 0);
	CHECK(f.started && f.started->cmd == SCRIPT_SEED && f.started->seed == 7, "el primer fotograma empieza el seed");
	f = ScriptRun_Tick(&run, FRAME_US);
	CHECK(f.started && f.started->cmd == SCRIPT_SLEEP, "el segundo empieza el sleep");
	f = ScriptRun_Tick(&run, FRAME_US + 499000);
	CHECK(f.started == NULL && f.buttons == 0, "el sleep sigue a los 499 ms");
	f = ScriptRun_Tick(&run, FRAME_US + 500000);
	CHECK(f.started && f.started->cmd == SCRIPT_PRESS && f.buttons == BTN_CROSS,
		"a los 500 ms empieza el press en el mismo fotograma");

	Script_Free(script);
}

// Los escenarios "Campos de mas en la linea" y "Valor distinto".
static void test_wait_matches(void) {
	Script *script = one_scenario("wait library sel=2\nwait nowplaying file=\"a \\\"b\\\".mp3\"\n");
	if (!script)
		return;

	const ScriptStep *wait = Script_Scenario(script, 0, NULL) + 1;
	LogParsed line;

	const char *more = "library view=songs group=- sel=2 count=291 mode=normal menu=none";
	CHECK(LogLine_ParseBody(more, (unsigned int)strlen(more), &line) && Script_WaitMatches(wait, &line),
		"wait library sel=2 deberia cumplirse con campos de mas");

	const char *other = "library view=songs sel=20";
	CHECK(LogLine_ParseBody(other, (unsigned int)strlen(other), &line) && !Script_WaitMatches(wait, &line),
		"wait library sel=2 no deberia cumplirse con sel=20");

	const char *category = "folders sel=2";
	CHECK(LogLine_ParseBody(category, (unsigned int)strlen(category), &line) && !Script_WaitMatches(wait, &line),
		"wait library sel=2 no deberia cumplirse con otra categoria");

	const char *missing = "library view=songs";
	CHECK(LogLine_ParseBody(missing, (unsigned int)strlen(missing), &line) && !Script_WaitMatches(wait, &line),
		"wait library sel=2 no deberia cumplirse sin sel");

	const char *quoted = "nowplaying file=\"a \\\"b\\\".mp3\" paused=0";
	CHECK(LogLine_ParseBody(quoted, (unsigned int)strlen(quoted), &line) && Script_WaitMatches(wait + 1, &line),
		"un valor con comillas deberia cumplirse");

	Script_Free(script);
}

static void test_wait_lookahead(void) {
	// La linea llega en el mismo fotograma que el press, antes de que los
	// wait empiecen, y en el orden contrario al del guion.
	Script *script = one_scenario("press DOWN x2\nwait library sel=1\nsleep 100ms\nwait library sel=2\n");
	if (!script)
		return;

	ScriptRun run;
	ScriptRun_Init(&run, script, 0, NULL, NULL);

	uint64_t now = 0;
	ScriptRun_Tick(&run, now);
	observe(&run, "0.000 library view=songs sel=2\n");
	observe(&run, "0.000 library view=songs sel=1\n");

	ScriptFrame f;
	int frames = 0;
	do {
		now += FRAME_US;
		f = ScriptRun_Tick(&run, now);
		frames++;
	} while (f.result == SCRIPT_RUNNING && frames < 100);

	CHECK(f.result == SCRIPT_PASS, "las lineas vistas durante el press deberian cumplir los dos wait");
	// 3 del press, 1 por wait y 100 ms de sleep: muy por debajo del timeout.
	CHECK(frames < 20, "deberia terminar enseguida, tardo %d fotogramas", frames);

	Script_Free(script);

	// Una linea anterior a la ultima accion ya no cuenta.
	script = one_scenario("press DOWN\npress DOWN\nwait library sel=1 timeout=100ms\n");
	if (!script)
		return;

	ScriptRun_Init(&run, script, 0, NULL, NULL);
	now = 0;
	ScriptRun_Tick(&run, now);
	observe(&run, "0.000 library view=songs sel=1\n");
	do {
		now += FRAME_US;
		f = ScriptRun_Tick(&run, now);
	} while (f.result == SCRIPT_RUNNING && now < 10000000);

	CHECK(f.result == SCRIPT_FAIL, "una linea de antes de la ultima accion no deberia cumplir el wait");
	CHECK(f.last && strcmp(f.last, "library view=songs sel=1") == 0, "last deberia ser la linea vista: '%s'",
		f.last ? f.last : "(nada)");

	Script_Free(script);
}

static void test_wait_state(void) {
	// El escenario "El estado actual ya cumple", y un wait sin accion previa.
	Script *script = one_scenario("wait library view=songs\nwait library view=songs\n");
	if (!script)
		return;

	FakeState state = { "library", "library view=songs group=- sel=0 count=291 mode=normal menu=none\n" };
	ScriptRun run;
	ScriptRun_Init(&run, script, 0, fake_last_state, &state);

	ScriptFrame f = ScriptRun_Tick(&run, 0);
	CHECK(f.started && f.result == SCRIPT_RUNNING, "el primer wait empieza");
	f = ScriptRun_Tick(&run, FRAME_US);
	CHECK(f.started && f.started->line == 3, "el segundo wait empieza en el fotograma siguiente");
	f = ScriptRun_Tick(&run, 2 * FRAME_US);
	CHECK(f.result == SCRIPT_PASS, "los dos wait se cumplen con el estado actual");

	Script_Free(script);

	// Sin estado, un wait sin accion previa ve las lineas desde el inicio
	// del escenario.
	script = one_scenario("wait screen to=folders\n");
	if (!script)
		return;

	ScriptRun_Init(&run, script, 0, fake_last_state, NULL);
	observe(&run, "2.660 screen to=folders heap_kb=1362\n");
	f = ScriptRun_Tick(&run, 0);
	CHECK(f.started, "el wait empieza");
	f = ScriptRun_Tick(&run, FRAME_US);
	CHECK(f.result == SCRIPT_PASS, "una linea vista antes del primer fotograma deberia cumplir el wait");

	Script_Free(script);
}

static void test_wait_timeout(void) {
	// El escenario "Timeout": en la linea 12 del guion.
	Script *script = one_scenario("# 2\n# 3\n# 4\n# 5\n# 6\n# 7\n# 8\n# 9\n# 10\n# 11\nwait library sel=5 timeout=1s\npress CROSS\n");
	if (!script)
		return;

	FakeState state = { "library", "library view=songs group=- sel=0 count=291 mode=normal menu=none\n" };
	ScriptRun run;
	ScriptRun_Init(&run, script, 0, fake_last_state, &state);

	uint64_t start = 3000000, now = start;
	ScriptFrame f;
	do {
		f = ScriptRun_Tick(&run, now);
		CHECK(f.buttons == 0, "el press despues del wait no deberia llegar");
		now += FRAME_US;
	} while (f.result == SCRIPT_RUNNING && now < start + 10000000);

	now -= FRAME_US;
	CHECK(f.result == SCRIPT_FAIL, "el wait deberia vencer");
	CHECK(now - start >= 1000000 && now - start < 1000000 + FRAME_US, "deberia vencer al segundo, vencio a los %llu us",
		(unsigned long long)(now - start));
	CHECK(f.line == 12, "el motivo deberia tener line=12, tiene %d", f.line);
	CHECK(f.reason && strcmp(f.reason, "wait timeout") == 0, "motivo '%s'", f.reason ? f.reason : "(nada)");
	CHECK(f.last && strcmp(f.last, "library view=songs group=- sel=0 count=291 mode=normal menu=none") == 0,
		"last deberia ser el estado actual: '%s'", f.last ? f.last : "(nada)");

	f = ScriptRun_Tick(&run, now + FRAME_US);
	CHECK(f.result == SCRIPT_FAIL && f.line == 12, "despues de fallar, sigue fail");
	Script_Free(script);

	// Con lineas vistas, last es la ultima de la categoria, aunque haya otras
	// despues.
	script = one_scenario("press DOWN\nwait library sel=5 timeout=100ms\n");
	if (!script)
		return;

	ScriptRun_Init(&run, script, 0, fake_last_state, &state);
	ScriptRun_Tick(&run, 0);
	observe(&run, "0.010 library view=songs sel=1\n");
	observe(&run, "0.020 library view=songs sel=2 name=\"a b\"\n");
	observe(&run, "0.030 input up=DOWN\n");
	now = 0;
	do {
		now += FRAME_US;
		f = ScriptRun_Tick(&run, now);
	} while (f.result == SCRIPT_RUNNING && now < 10000000);
	CHECK(f.result == SCRIPT_FAIL && f.last && strcmp(f.last, "library view=songs sel=2 name=\"a b\"") == 0,
		"last deberia ser la ultima library: '%s'", f.last ? f.last : "(nada)");
	Script_Free(script);

	// Sin ninguna linea de la categoria, last queda vacio.
	script = one_scenario("wait track event=open timeout=100ms\n");
	if (!script)
		return;

	ScriptRun_Init(&run, script, 0, fake_last_state, &state);
	now = 0;
	do {
		f = ScriptRun_Tick(&run, now);
		now += FRAME_US;
	} while (f.result == SCRIPT_RUNNING && now < 10000000);
	CHECK(f.result == SCRIPT_FAIL && f.last == NULL, "sin lineas track, last deberia faltar");
	Script_Free(script);
}

// Mas waits entre dos acciones que los que caben en el grupo: los de mas
// solo ven lo que llega desde que empiezan.
static void test_wait_group_overflow(void) {
	char steps[4096];
	unsigned int n = (unsigned int)snprintf(steps, sizeof(steps), "press CROSS\n");
	for (int i = 0; i <= SCRIPT_GROUP_MAX; i++)
		n += (unsigned int)snprintf(steps + n, sizeof(steps) - n, "wait library sel=%d\n", i);

	Script *script = one_scenario(steps);
	if (!script)
		return;

	ScriptRun run;
	ScriptRun_Init(&run, script, 0, NULL, NULL);

	char line[64];
	uint64_t now = 0;
	ScriptRun_Tick(&run, now);
	for (int i = 0; i < SCRIPT_GROUP_MAX; i++) {
		snprintf(line, sizeof(line), "0.000 library sel=%d\n", i);
		observe(&run, line);
	}

	ScriptFrame f;
	int frames = 0;
	do {
		now += FRAME_US;
		f = ScriptRun_Tick(&run, now);
		// El ultimo wait recibe su linea cuando ya empezo.
		if (f.started && f.started->line == 3 + SCRIPT_GROUP_MAX) {
			snprintf(line, sizeof(line), "0.000 library sel=%d\n", SCRIPT_GROUP_MAX);
			observe(&run, line);
		}
		frames++;
	} while (f.result == SCRIPT_RUNNING && frames < 1000);

	CHECK(f.result == SCRIPT_PASS, "los waits de mas deberian cumplirse con lineas que llegan despues de empezar");
	Script_Free(script);
}

static void test_state(void) {
	SuiteState in = { 3, 2, 1, 2, 1 }, out;
	char buf[SUITE_STATE_MAX];

	unsigned int len = SuiteState_Format(&in, buf, sizeof(buf));
	CHECK(len > 0, "el estado deberia formatearse");
	CHECK(SuiteState_Parse(buf, len, &out) && memcmp(&in, &out, sizeof(in)) == 0, "ida y vuelta del estado");

	SuiteState none = { 0, -1, 0, 0, 0 };
	len = SuiteState_Format(&none, buf, sizeof(buf));
	CHECK(SuiteState_Parse(buf, len, &out) && out.current == -1 && out.next == 0, "ida y vuelta con current=-1");

	SuiteState big = { SCRIPT_STEPS_MAX, SCRIPT_STEPS_MAX, 1, SCRIPT_STEPS_MAX, SCRIPT_STEPS_MAX };
	len = SuiteState_Format(&big, buf, sizeof(buf));
	CHECK(SuiteState_Parse(buf, len, &out) && memcmp(&big, &out, sizeof(big)) == 0, "ida y vuelta con los maximos");

	CHECK(SuiteState_Format(&in, buf, 10) == 0, "un buffer chico deberia dar 0");

	// Cortado en cualquier punto, incluso a mitad de un numero, es como si
	// no existiera.
	SuiteState wide = { 12, 11, 0, 10, 1 };
	len = SuiteState_Format(&wide, buf, sizeof(buf));
	for (unsigned int cut = 0; cut < len; cut++) {
		char *copy = malloc(cut ? cut : 1);
		memcpy(copy, buf, cut);
		CHECK(!SuiteState_Parse(copy, cut, &out), "el estado cortado en %u bytes no deberia leerse", cut);
		free(copy);
	}

	static const char *bad[] = {
		"next=1\ncurrent=0\nrelaunch=0\npass=0\nfail=0\nend\nmas\n",
		"next=1\ncurrent=0\nrelaunch=2\npass=0\nfail=0\nend\n",
		"next=1\ncurrent=-2\nrelaunch=0\npass=0\nfail=0\nend\n",
		"next=-1\ncurrent=0\nrelaunch=0\npass=0\nfail=0\nend\n",
		"next=1\ncurrent=0\nrelaunch=0\npass=0\nfail=-1\nend\n",
		"current=0\nnext=1\nrelaunch=0\npass=0\nfail=0\nend\n",
		"next=1\r\ncurrent=0\r\nrelaunch=0\r\npass=0\r\nfail=0\r\nend\r\n",
		"next=1025\ncurrent=0\nrelaunch=0\npass=0\nfail=0\nend\n",
		"next=999999\ncurrent=0\nrelaunch=0\npass=0\nfail=0\nend\n",
		"next=1\ncurrent=0\nrelaunch=0\npass=\nfail=0\nend\n",
		"next=1\ncurrent=0\nrelaunch=0\npass=0x\nfail=0\nend\n",
		"",
	};
	for (unsigned int i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
		CHECK(!SuiteState_Parse(bad[i], (unsigned int)strlen(bad[i]), &out), "no deberia leer el estado %u", i);

	// Basura.
	char junk[SUITE_STATE_MAX];
	srand(99);
	for (int round = 0; round < 1000; round++) {
		unsigned int n = (unsigned int)(rand() % (int)sizeof(junk)) + 1;
		for (unsigned int i = 0; i < n; i++)
			junk[i] = (char)(rand() & 0xFF);
		CHECK(!SuiteState_Parse(junk, n, &out), "la basura no deberia leerse como estado");
	}

	char huge[SUITE_STATE_MAX + 1];
	memset(huge, '\n', sizeof(huge));
	CHECK(!SuiteState_Parse(huge, sizeof(huge), &out), "un estado de mas de %d bytes no deberia leerse", SUITE_STATE_MAX);
}

int main(void) {
	test_valid();
	test_state();
	test_errors();
	test_limits();
	test_run_press();
	test_run_hold();
	test_run_tap_swipe();
	test_run_sleep_seed();
	test_wait_matches();
	test_wait_lookahead();
	test_wait_state();
	test_wait_timeout();
	test_wait_group_overflow();

	if (failures == 0)
		printf("test_script: todo bien\n");

	return failures == 0 ? 0 : 1;
}
