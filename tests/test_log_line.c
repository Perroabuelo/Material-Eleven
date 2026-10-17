// Pruebas en PC de las lineas del log de desarrollo (source/log_line.c),
// compiladas con el gcc del host. log_line.c es logica pura, asi que no hace
// falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log_line.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// Las mascaras de psp2/ctrl.h, repetidas para no depender de los headers de la
// consola.
#define BTN_SELECT 0x00000001
#define BTN_UP     0x00000010
#define BTN_DOWN   0x00000040
#define BTN_CROSS  0x00004000
#define BTN_SQUARE 0x00008000

static const LogButtonName buttons[] = {
	{ BTN_CROSS,  "CROSS"  },
	{ BTN_SQUARE, "SQUARE" },
	{ BTN_UP,     "UP"     },
	{ BTN_DOWN,   "DOWN"   },
	{ BTN_SELECT, "SELECT" },
};
#define BUTTON_COUNT ((int)(sizeof(buttons) / sizeof(buttons[0])))

// Termina la linea y la deja como texto C en `out`.
static const char *finish(LogLine *line, char *out) {
	unsigned int len = LogLine_Finish(line);
	memcpy(out, line->text, len);
	out[len] = '\0';
	return out;
}

static int ends_with(const char *s, const char *suffix) {
	size_t a = strlen(s), b = strlen(suffix);
	return a >= b && strcmp(s + a - b, suffix) == 0;
}

// 1 si `s` (de `len` bytes) es UTF-8 valido.
static int valid_utf8(const unsigned char *s, size_t len) {
	size_t i = 0;
	while (i < len) {
		unsigned char c = s[i];
		size_t n;
		if (c < 0x80) n = 1;
		else if (c >= 0xC2 && c <= 0xDF) n = 2;
		else if (c >= 0xE0 && c <= 0xEF) n = 3;
		else if (c >= 0xF0 && c <= 0xF4) n = 4;
		else return 0;
		if (i + n > len)
			return 0;
		for (size_t k = 1; k < n; k++) {
			if ((s[i + k] & 0xC0) != 0x80)
				return 0;
		}
		i += n;
	}
	return 1;
}

// Cuenta las comillas sin escapar de `s`.
static int bare_quotes(const char *s) {
	int n = 0;
	for (const char *p = s; *p; p++) {
		if (*p == '\\' && p[1])
			p++;
		else if (*p == '"')
			n++;
	}
	return n;
}

static void test_time(void) {
	LogLine line;
	char out[LOG_LINE_MAX + 1];

	LogLine_Begin(&line, 0, "log");
	CHECK(strcmp(finish(&line, out), "0.000 log\n") == 0, "tiempo 0: '%s'", out);

	LogLine_Begin(&line, 999, "log");
	CHECK(strncmp(finish(&line, out), "0.000 ", 6) == 0, "999 us deberia dar 0.000: '%s'", out);

	LogLine_Begin(&line, 3500000, "log");
	CHECK(strncmp(finish(&line, out), "3.500 ", 6) == 0, "3 500 000 us deberia dar 3.500: '%s'", out);

	// Una hora, un minuto, un segundo y 234 ms.
	LogLine_Begin(&line, 3661234567ULL, "log");
	CHECK(strncmp(finish(&line, out), "3661.234 ", 9) == 0, "mas de una hora: '%s'", out);
}

static void check_value(const char *value, const char *expected) {
	LogLine line;
	char out[LOG_LINE_MAX + 1];
	char want[LOG_LINE_MAX + 1];

	LogLine_Begin(&line, 12345000, "track");
	LogLine_Str(&line, "file", value);
	snprintf(want, sizeof(want), "12.345 track file=%s\n", expected);
	CHECK(strcmp(finish(&line, out), want) == 0, "valor '%s': esperaba '%s', dio '%s'", value, want, out);
}

static void test_values(void) {
	check_value("song.mp3", "song.mp3");
	check_value("ux0:/Mis cosas/Say \"Hi\".mp3", "\"ux0:/Mis cosas/Say \\\"Hi\\\".mp3\"");
	check_value("a\\b", "\"a\\\\b\"");
	check_value("uno\ndos", "\"uno\\ndos\"");
	check_value("", "\"\"");
	check_value("canción", "canción");
	check_value(NULL, "\"\"");

	LogLine line;
	char out[LOG_LINE_MAX + 1];
	LogLine_Begin(&line, 1000, "library");
	LogLine_Str(&line, "view", "songs");
	LogLine_Int(&line, "sel", 2);
	LogLine_Int(&line, "count", -1);
	CHECK(strcmp(finish(&line, out), "0.001 library view=songs sel=2 count=-1\n") == 0, "varios campos: '%s'", out);

	// Finish dos veces no agrega otro salto de linea.
	CHECK(LogLine_Finish(&line) == strlen(out), "Finish deberia ser idempotente");
}

static void check_cut(const char *what, const char *value) {
	LogLine line;
	char out[LOG_LINE_MAX + 1];

	LogLine_Begin(&line, 1000000, "track");
	LogLine_Str(&line, "event", "open");
	LogLine_Str(&line, "file", value);
	LogLine_Str(&line, "result", "ok");
	unsigned int len = LogLine_Finish(&line);
	memcpy(out, line.text, len);
	out[len] = '\0';

	CHECK(len <= LOG_LINE_MAX, "%s: la linea mide %u bytes", what, len);
	CHECK(strlen(out) == len, "%s: la linea no deberia tener bytes nulos", what);
	CHECK(ends_with(out, " cut=1\n"), "%s: deberia terminar en cut=1: '...%s'", what, out + (len > 20 ? len - 20 : 0));
	CHECK(strstr(out, "result=") == NULL, "%s: despues del recorte no van mas campos", what);
	CHECK(bare_quotes(out) % 2 == 0, "%s: las comillas deberian quedar cerradas", what);
	CHECK(valid_utf8((const unsigned char *)out, len), "%s: la linea deberia ser UTF-8 valido", what);
}

static void test_cut(void) {
	char value[2001];

	memset(value, 'a', 2000);
	value[2000] = '\0';
	check_cut("ASCII de 2000 bytes", value);

	// "ñ" (2 bytes) repetido: el corte cae en medio de un caracter segun la
	// paridad de lo que va antes, por eso tambien con un prefijo de un byte.
	for (int i = 0; i < 1000; i++)
		memcpy(value + 2 * i, "\xC3\xB1", 2);
	value[2000] = '\0';
	check_cut("caracteres de 2 bytes", value);
	value[0] = 'x';
	value[1] = ' ';
	check_cut("caracteres de 2 bytes con espacio", value);

	// "€" (3 bytes), con 0, 1 y 2 bytes de prefijo, para cortar en cada
	// posicion posible dentro del caracter.
	for (int shift = 0; shift < 3; shift++) {
		int len = 0;
		for (int i = 0; i < shift; i++)
			value[len++] = 'x';
		while (len + 3 <= 2000) {
			memcpy(value + len, "\xE2\x82\xAC", 3);
			len += 3;
		}
		value[len] = '\0';
		check_cut("caracteres de 3 bytes", value);
	}

	// Escapes al final: un \" no se parte.
	memset(value, '"', 2000);
	value[2000] = '\0';
	check_cut("solo comillas", value);

	// Un campo que no cabe por poco no corta el anterior.
	LogLine line;
	char out[LOG_LINE_MAX + 1];
	memset(value, 'b', 490);
	value[490] = '\0';
	LogLine_Begin(&line, 0, "x");
	LogLine_Str(&line, "a", value);
	LogLine_Str(&line, "k", "123456789012345678901234567890");
	finish(&line, out);
	CHECK(strstr(out, value) != NULL, "el primer valor deberia quedar entero");
	CHECK(ends_with(out, " cut=1\n") && strlen(out) <= LOG_LINE_MAX, "el segundo deberia cortarse: '%s'", out + 480);

	// Una linea que llena justo el espacio no se marca como cortada.
	LogLine_Begin(&line, 0, "x");
	memset(value, 'c', 495);
	value[495] = '\0';
	LogLine_Str(&line, "a", value);
	finish(&line, out);
	CHECK(!ends_with(out, "cut=1\n"), "un valor que cabe no deberia cortarse (%zu bytes)", strlen(out));
}

static void test_buttons(void) {
	LogLine line;
	char out[LOG_LINE_MAX + 1];

	LogLine_Begin(&line, 0, "input");
	LogLine_Buttons(&line, "down", 0, buttons, BUTTON_COUNT);
	CHECK(strcmp(finish(&line, out), "0.000 input\n") == 0, "sin botones no va el campo: '%s'", out);

	LogLine_Begin(&line, 0, "input");
	LogLine_Buttons(&line, "down", BTN_SQUARE, buttons, BUTTON_COUNT);
	CHECK(strcmp(finish(&line, out), "0.000 input down=SQUARE\n") == 0, "un boton: '%s'", out);

	// En el orden de la tabla, no en el de los bits; el bit desconocido se
	// ignora.
	LogLine_Begin(&line, 0, "input");
	LogLine_Buttons(&line, "down", BTN_SELECT | BTN_UP | BTN_CROSS | 0x80000000u, buttons, BUTTON_COUNT);
	LogLine_Buttons(&line, "up", BTN_DOWN, buttons, BUTTON_COUNT);
	CHECK(strcmp(finish(&line, out), "0.000 input down=CROSS+UP+SELECT up=DOWN\n") == 0, "varios botones: '%s'", out);
}

// Arma una linea "<t> ev n=<n>" de largo fijo para un mismo numero de cifras.
static void make_line(LogLine *line, unsigned long long time_us, int n) {
	LogLine_Begin(line, time_us, "ev");
	LogLine_Int(line, "n", n);
}

// Lee todo lo que queda en el buffer, de a `chunk` bytes, y lo deja en `out`.
static unsigned int drain(LogRing *ring, char *out, unsigned int out_size, unsigned int chunk) {
	unsigned int total = 0;
	for (;;) {
		unsigned int max = out_size - 1 - total < chunk ? out_size - 1 - total : chunk;
		unsigned int got = LogRing_Read(ring, out + total, max);
		if (got == 0)
			break;
		total += got;
	}
	out[total] = '\0';
	return total;
}

static void test_ring_wraps(void) {
	// "1.000 ev n=1\n" mide 13 bytes; caben 4 en 60.
	char mem[60];
	char out[256];
	LogRing ring;
	LogLine line;

	LogRing_Init(&ring, mem, sizeof(mem));
	for (int i = 1; i <= 4; i++) {
		make_line(&line, 1000000, i);
		CHECK(LogRing_Push(&ring, &line) == 1, "la linea %d deberia caber", i);
	}

	// Vaciar en parte: dos lineas y media.
	unsigned int got = LogRing_Read(&ring, out, 32);
	out[got] = '\0';
	CHECK(got == 32 && strncmp(out, "1.000 ev n=1\n1.000 ev n=2\n1.000 ", 32) == 0, "lectura parcial: '%s'", out);

	// Las siguientes dan la vuelta al final del bloque.
	for (int i = 5; i <= 6; i++) {
		make_line(&line, 2000000, i);
		CHECK(LogRing_Push(&ring, &line) == 1, "la linea %d deberia caber al dar la vuelta", i);
	}

	drain(&ring, out, sizeof(out), 7);
	CHECK(strcmp(out, "ev n=3\n1.000 ev n=4\n2.000 ev n=5\n2.000 ev n=6\n") == 0, "orden despues de la vuelta: '%s'", out);
	CHECK(ring.used == 0 && LogRing_Read(&ring, out, 10) == 0, "el buffer deberia quedar vacio");
	CHECK(ring.accepted == 6 && ring.dropped == 0, "totales: %u aceptadas, %u perdidas", ring.accepted, ring.dropped);
}

static void test_ring_drops(void) {
	// Tres lineas de 13 bytes llenan 39 de 44.
	char mem[44];
	char out[256];
	LogRing ring;
	LogLine line;

	LogRing_Init(&ring, mem, sizeof(mem));
	for (int i = 1; i <= 3; i++) {
		make_line(&line, 1000000, i);
		LogRing_Push(&ring, &line);
	}

	make_line(&line, 1000000, 4);
	CHECK(LogRing_Push(&ring, &line) == 0, "la cuarta no deberia caber");
	make_line(&line, 1000000, 5);
	CHECK(LogRing_Push(&ring, &line) == 0, "la quinta no deberia caber");
	CHECK(ring.used == 39, "una linea rechazada no deberia dejar bytes (%u usados)", ring.used);
	CHECK(ring.pending == 2 && ring.dropped == 2, "deberia contar dos perdidas (%u, %u)", ring.pending, ring.dropped);

	// Con sitio para la linea pero no para la nota, que va primero ("5.000 log
	// event=dropped count=2\n" mide 32 bytes): no entra ninguna y se sigue
	// contando.
	LogRing_Read(&ring, out, 13);
	CHECK(LogRing_Push(&ring, &line) == 0, "sin sitio para la nota la linea no deberia entrar");
	CHECK(ring.pending == 3 && ring.used == 26, "si la nota no cabe se sigue contando (%u, %u usados)", ring.pending, ring.used);

	// Vacio: entra la nota con la cuenta correcta, pero la linea ya no cabe y
	// la cuenta vuelve a 1.
	LogRing_Read(&ring, out, sizeof(out));
	make_line(&line, 5000000, 6);
	CHECK(LogRing_Push(&ring, &line) == 0, "la nota (32) y la linea (13) no caben juntas en 44");
	CHECK(ring.pending == 1, "la nota entro: la cuenta deberia ser 1 por la linea nueva (%u)", ring.pending);
	drain(&ring, out, sizeof(out), 100);
	CHECK(strcmp(out, "5.000 log event=dropped count=3\n") == 0, "la nota: '%s'", out);

	// La nota va antes que la siguiente linea, con el tiempo de esa linea.
	LogLine_Begin(&line, 6000000, "x");
	CHECK(LogRing_Push(&ring, &line) == 1, "con sitio la linea deberia entrar despues de la nota");
	drain(&ring, out, sizeof(out), 100);
	CHECK(strcmp(out, "6.000 log event=dropped count=1\n6.000 x\n") == 0, "nota y linea: '%s'", out);
	CHECK(ring.pending == 0, "la cuenta deberia quedar en 0");
	CHECK(ring.accepted == 4 && ring.dropped == 4, "totales: %u aceptadas, %u perdidas", ring.accepted, ring.dropped);
}

static void test_ring_too_small(void) {
	// Un buffer de tamano 0 y otro menor que una linea, rodeados de bytes
	// testigo: nada se escribe fuera y todo se cuenta.
	char mem[16];
	LogRing ring;
	LogLine line;

	memset(mem, 0x5A, sizeof(mem));
	LogRing_Init(&ring, mem + 4, 0);
	make_line(&line, 0, 1);
	CHECK(LogRing_Push(&ring, &line) == 0 && LogRing_Push(&ring, &line) == 0, "un buffer vacio no deberia aceptar nada");
	CHECK(ring.dropped == 2, "deberia contar las dos (%u)", ring.dropped);

	LogRing_Init(&ring, NULL, 100);
	CHECK(LogRing_Push(&ring, &line) == 0, "sin memoria no deberia aceptar nada");

	LogRing_Init(&ring, mem + 4, 8);
	CHECK(LogRing_Push(&ring, &line) == 0 && LogRing_Push(&ring, &line) == 0, "un buffer de 8 bytes no deberia aceptar lineas de 13");
	CHECK(ring.dropped == 2 && ring.used == 0, "deberia contar y no guardar nada");

	int intact = 1;
	for (unsigned int i = 0; i < sizeof(mem); i++)
		intact &= (unsigned char)mem[i] == 0x5A;
	CHECK(intact, "no deberia escribirse ningun byte");

	char out[8];
	CHECK(LogRing_Read(&ring, out, sizeof(out)) == 0, "leer un buffer vacio deberia dar 0");
}

static void state_line(LogLine *line, unsigned long long time_us, int sel) {
	LogLine_Begin(line, time_us, "library");
	LogLine_Str(line, "view", "songs");
	LogLine_Int(line, "sel", sel);
}

static void test_state(void) {
	LogState state;
	LogLine line;
	int written = 0;

	LogState_Reset(&state);

	// El escenario "Estado repetido": la misma dos veces (con distinto
	// tiempo), una distinta y la primera otra vez.
	state_line(&line, 1000000, 1);
	written += LogState_Changed(&state, &line);
	state_line(&line, 1016000, 1);
	CHECK(LogState_Changed(&state, &line) == 0, "la misma linea con otro tiempo no deberia escribirse");
	state_line(&line, 1032000, 2);
	written += LogState_Changed(&state, &line);
	state_line(&line, 1048000, 1);
	written += LogState_Changed(&state, &line);
	CHECK(written == 3, "deberian escribirse 3 lineas, se escribieron %d", written);

	// Tras vaciar el canal, la misma linea se escribe.
	LogState_Reset(&state);
	state_line(&line, 2000000, 1);
	CHECK(LogState_Changed(&state, &line) == 1, "tras LogState_Reset la siguiente deberia escribirse");

	// Un campo de mas tambien es un cambio.
	state_line(&line, 2016000, 1);
	LogLine_Str(&line, "mode", "normal");
	CHECK(LogState_Changed(&state, &line) == 1, "una linea mas larga deberia contar como distinta");
}

// Parsea `len` bytes copiados a un bloque de ese tamano exacto, sin NUL
// detras, para que ASan vea cualquier lectura de mas.
static int parse_exact(const char *text, unsigned int len, int with_time, LogParsed *out) {
	char *copy = malloc(len ? len : 1);
	memcpy(copy, text, len);
	int ok = with_time ? LogLine_Parse(copy, len, out) : LogLine_ParseBody(copy, len, out);
	free(copy);
	return ok;
}

// Arma "track file=<value> n=7", lo parsea y compara el valor.
static void check_round_trip(const char *value) {
	LogLine line;
	LogParsed parsed;

	LogLine_Begin(&line, 12345000, "track");
	LogLine_Str(&line, "file", value);
	LogLine_Int(&line, "n", 7);
	unsigned int len = LogLine_Finish(&line);

	CHECK(parse_exact(line.text, len, 1, &parsed), "ida y vuelta de '%s': no parsea", value);
	CHECK(parsed.time_us == 12345000, "ida y vuelta de '%s': tiempo %llu", value, (unsigned long long)parsed.time_us);
	CHECK(strcmp(parsed.category, "track") == 0, "ida y vuelta de '%s': categoria '%s'", value, parsed.category);
	CHECK(parsed.count == 2, "ida y vuelta de '%s': %d campos", value, parsed.count);

	const char *file = LogParsed_Get(&parsed, "file");
	CHECK(file && strcmp(file, value) == 0, "ida y vuelta de '%s': dio '%s'", value, file ? file : "(nada)");
	const char *n = LogParsed_Get(&parsed, "n");
	CHECK(n && strcmp(n, "7") == 0, "ida y vuelta de '%s': n='%s'", value, n ? n : "(nada)");
}

static void test_parse_round_trip(void) {
	check_round_trip("song.mp3");
	check_round_trip("ux0:/Mis cosas/una pista.flac");
	check_round_trip("Say \"Hi\"");
	check_round_trip("a\\b\\");
	check_round_trip("uno\ndos\n");
	check_round_trip("");
	check_round_trip("canción ふかふか");
	check_round_trip("k=v");

	// Sin el salto de linea final tambien se lee.
	LogParsed parsed;
	const char *bare = "3.500 library view=songs group=- sel=2";
	CHECK(parse_exact(bare, strlen(bare), 1, &parsed), "sin salto de linea deberia parsear");
	CHECK(parsed.time_us == 3500000 && parsed.count == 3, "sin salto de linea: tiempo %llu, %d campos",
		(unsigned long long)parsed.time_us, parsed.count);
	CHECK(LogParsed_Get(&parsed, "mode") == NULL, "un campo que no esta deberia dar NULL");

	// Una linea sin campos.
	const char *lone = "0.000 log\n";
	CHECK(parse_exact(lone, strlen(lone), 1, &parsed) && parsed.count == 0
		&& strcmp(parsed.category, "log") == 0, "una linea sin campos deberia parsear");

	// Sin tiempo, como la condicion de un wait.
	const char *body = "library sel=2 name=\"a b\"";
	CHECK(parse_exact(body, strlen(body), 0, &parsed), "el cuerpo deberia parsear");
	CHECK(parsed.time_us == 0 && strcmp(parsed.category, "library") == 0 && parsed.count == 2,
		"cuerpo: categoria '%s', %d campos", parsed.category, parsed.count);
	const char *name = LogParsed_Get(&parsed, "name");
	CHECK(name && strcmp(name, "a b") == 0, "cuerpo: name='%s'", name ? name : "(nada)");
}

static void test_parse_cut(void) {
	char value[2000];
	LogLine line;
	LogParsed parsed;

	// Con comillas (tiene espacios) y sin ellas: el valor recortado y cut=1
	// llegan como campos.
	for (int quoted = 0; quoted < 2; quoted++) {
		memset(value, quoted ? ' ' : 'a', sizeof(value) - 1);
		value[sizeof(value) - 1] = '\0';

		LogLine_Begin(&line, 1000, "track");
		LogLine_Str(&line, "file", value);
		LogLine_Str(&line, "after", "x");
		unsigned int len = LogLine_Finish(&line);

		CHECK(parse_exact(line.text, len, 1, &parsed), "linea recortada (comillas=%d): no parsea", quoted);
		const char *file = LogParsed_Get(&parsed, "file");
		const char *cut = LogParsed_Get(&parsed, "cut");
		CHECK(file && strlen(file) > 400 && strncmp(file, value, strlen(file)) == 0,
			"linea recortada (comillas=%d): valor de %zu bytes", quoted, file ? strlen(file) : 0);
		CHECK(cut && strcmp(cut, "1") == 0, "linea recortada (comillas=%d): falta cut=1", quoted);
		CHECK(LogParsed_Get(&parsed, "after") == NULL, "linea recortada (comillas=%d): no deberia tener after", quoted);
	}

	// Recortada justo antes de un escape: el escape no queda a medias.
	memset(value, '"', sizeof(value) - 1);
	value[sizeof(value) - 1] = '\0';
	LogLine_Begin(&line, 1000, "track");
	LogLine_Str(&line, "file", value);
	unsigned int len = LogLine_Finish(&line);
	CHECK(parse_exact(line.text, len, 1, &parsed), "linea recortada entre escapes: no parsea");
}

static void test_parse_rejects(void) {
	static const char *bad[] = {
		"",
		"\n",
		"1.000",
		"1.000\n",
		"1.000 \n",              // sin categoria
		"1.00 log",              // dos decimales
		"1,000 log",
		".000 log",
		"x.000 log",
		"1.000  log",            // dos espacios
		"1.000 log ",            // espacio al final
		"1.000 log  a=1",
		"1.000 log a",           // sin '='
		"1.000 log =1",          // sin clave
		"1.000 log a=",          // valor vacio sin comillas
		"1.000 log a=\"abc",     // comillas sin cerrar
		"1.000 log a=\"abc\\\"", // la unica comilla de cierre esta escapada
		"1.000 log a=\"ab\\",    // escape al final
		"1.000 log a=\"a\\tb\"", // escape desconocido
		"1.000 log a=b\"c",      // comilla en un valor sin comillas
		"1.000 log a=b\\c",      // barra en un valor sin comillas
		"1.000 log a=\"x\"y",    // algo pegado a la comilla de cierre
		"1.000 log a\"=1",
		"1.000 log a=1\n\n",
		"1.000 log a=\"x\ny\"",  // salto de linea crudo dentro de comillas
		"1234567890123.000 log", // tiempo con demasiadas cifras
	};
	LogParsed parsed;

	for (unsigned int i = 0; i < sizeof(bad) / sizeof(bad[0]); i++)
		CHECK(!parse_exact(bad[i], strlen(bad[i]), 1, &parsed), "deberia rechazar '%s'", bad[i]);

	// Un NUL en medio.
	const char nul[] = "1.000 log a=b\0c";
	CHECK(!parse_exact(nul, sizeof(nul) - 1, 1, &parsed), "deberia rechazar un NUL en medio");

	// Mas de LOG_LINE_MAX bytes.
	char longer[LOG_LINE_MAX + 2];
	memcpy(longer, "1.000 log a=", 12);
	memset(longer + 12, 'x', sizeof(longer) - 12);
	CHECK(!parse_exact(longer, sizeof(longer), 1, &parsed), "deberia rechazar una linea de mas de %d bytes", LOG_LINE_MAX);

	// Un campo mas que LOG_FIELDS_MAX.
	char many[LOG_LINE_MAX];
	unsigned int n = (unsigned int)snprintf(many, sizeof(many), "1.000 log");
	for (int i = 0; i <= LOG_FIELDS_MAX; i++)
		n += (unsigned int)snprintf(many + n, sizeof(many) - n, " a=%d", i);
	CHECK(!parse_exact(many, n, 1, &parsed), "deberia rechazar %d campos", LOG_FIELDS_MAX + 1);
	CHECK(parse_exact(many, n - 5, 1, &parsed) && parsed.count == LOG_FIELDS_MAX,
		"deberia aceptar %d campos", LOG_FIELDS_MAX);

	// Y la linea mas larga posible, llena de escapes, sin pasarse del buffer.
	char full[LOG_LINE_MAX];
	n = (unsigned int)snprintf(full, sizeof(full), "1.000 log a=\"");
	while (n + 3 < LOG_LINE_MAX) {
		full[n++] = '\\';
		full[n++] = 'n';
	}
	full[n++] = '"';
	full[n++] = '\n';
	CHECK(parse_exact(full, n, 1, &parsed), "la linea mas larga deberia parsear");

	// El cuerpo sin tiempo tambien rechaza.
	CHECK(!parse_exact("", 0, 0, &parsed), "un cuerpo vacio deberia rechazarse");
	CHECK(!parse_exact("library sel", 11, 0, &parsed), "un cuerpo sin '=' deberia rechazarse");
}

int main(void) {
	test_time();
	test_values();
	test_cut();
	test_buttons();
	test_ring_wraps();
	test_ring_drops();
	test_ring_too_small();
	test_state();
	test_parse_round_trip();
	test_parse_cut();
	test_parse_rejects();

	if (failures == 0)
		printf("test_log_line: todo bien\n");

	return failures == 0 ? 0 : 1;
}
