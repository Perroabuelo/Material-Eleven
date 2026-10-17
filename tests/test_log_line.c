// Pruebas en PC de las lineas del log de desarrollo (source/log_line.c),
// compiladas con el gcc del host. log_line.c es logica pura, asi que no hace
// falta el VitaSDK.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
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

int main(void) {
	test_time();
	test_values();
	test_cut();
	test_buttons();

	if (failures == 0)
		printf("test_log_line: todo bien\n");

	return failures == 0 ? 0 : 1;
}
