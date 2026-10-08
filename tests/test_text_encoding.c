// Pruebas en PC de la conversion a UTF-8 de los .lrc (source/text_encoding.c),
// compiladas con el gcc del host.
//
//     make -C tests
//
// Sale con codigo 0 si todo pasa y distinto de 0 si algo falla, con una linea
// por cada falla.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "text_encoding.h"

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		failures++; \
		printf("FALLA %s:%d: ", __FILE__, __LINE__); \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} \
} while (0)

// Validador propio, escrito aparte del modulo para no probarlo contra si
// mismo: decodifica cada secuencia y rechaza las sobrelargas, los sustitutos y
// lo que pase de U+10FFFF.
static int valid_utf8(const unsigned char *s, size_t len) {
	size_t i = 0;

	while (i < len) {
		unsigned char c = s[i];
		size_t n;
		unsigned long cp, min;

		if (c < 0x80) { i++; continue; }
		else if ((c & 0xE0) == 0xC0) { n = 2; cp = c & 0x1F; min = 0x80; }
		else if ((c & 0xF0) == 0xE0) { n = 3; cp = c & 0x0F; min = 0x800; }
		else if ((c & 0xF8) == 0xF0) { n = 4; cp = c & 0x07; min = 0x10000; }
		else return 0;

		if (i + n > len)
			return 0;

		for (size_t k = 1; k < n; k++) {
			if ((s[i + k] & 0xC0) != 0x80)
				return 0;
			cp = (cp << 6) | (s[i + k] & 0x3F);
		}

		if (cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
			return 0;

		i += n;
	}

	return 1;
}

// Convierte y comprueba lo que vale para cualquier entrada: que devuelva 0,
// que el largo coincida con el NUL y que el resultado sea UTF-8 valido.
static char *convert(const void *in, size_t len, size_t *out_len) {
	char *out = NULL;
	int ret = TextEncoding_ToUtf8(in, len, &out, out_len);

	CHECK(ret == 0 && out != NULL, "ToUtf8 fallo con %zu bytes", len);
	if (out == NULL)
		return NULL;

	CHECK(strlen(out) <= *out_len && out[*out_len] == '\0', "la salida no termina en NUL en su largo");
	CHECK(valid_utf8((const unsigned char *)out, *out_len), "la salida no es UTF-8 valido");
	return out;
}

static void expect(const char *name, const void *in, size_t len, const char *want) {
	size_t out_len = 0;
	char *out = convert(in, len, &out_len);

	if (out == NULL)
		return;

	CHECK(out_len == strlen(want) && memcmp(out, want, out_len) == 0,
		"%s: dio \"%s\" (%zu bytes), se esperaba \"%s\"", name, out, out_len, want);
	free(out);
}

static unsigned char *read_file(const char *path, size_t *len) {
	FILE *f = fopen(path, "rb");

	if (f == NULL) {
		CHECK(0, "no se pudo abrir %s", path);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	unsigned char *buf = malloc(size > 0 ? size : 1);
	*len = fread(buf, 1, size, f);
	fclose(f);
	return buf;
}

static void test_paths(void) {
	// Sin BOM y UTF-8 valido: igual.
	expect("utf8", "Hola ñandú 어두워진", strlen("Hola ñandú 어두워진"), "Hola ñandú 어두워진");

	// BOM de UTF-8: se quita.
	expect("bom utf8", "\xEF\xBB\xBFHola", 7, "Hola");

	// UTF-16 LE y BE con BOM, con una letra fuera del BMP (U+1F600) en par de sustitutos.
	static const unsigned char le[] = { 0xFF, 0xFE, 'H', 0, 0xF1, 0, 0x3D, 0xD8, 0x00, 0xDE };
	static const unsigned char be[] = { 0xFE, 0xFF, 0, 'H', 0, 0xF1, 0xD8, 0x3D, 0xDE, 0x00 };
	expect("utf16 le", le, sizeof(le), "H\xC3\xB1\xF0\x9F\x98\x80");
	expect("utf16 be", be, sizeof(be), "H\xC3\xB1\xF0\x9F\x98\x80");

	// Windows-1252: Latin-1 directo y la zona 0x80-0x9F por tabla.
	expect("latin-1", "canci\xF3n \xF1", 9, "canción ñ");
	expect("cp1252", "\x80\x93x\x94\x99", 5, "€“x”™");

	// Los cinco huecos de la tabla pasan a U+FFFD.
	expect("huecos cp1252", "\x81\x8D\x8F\x90\x9D", 5, "\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD");
}

static void test_invalid(void) {
	// Una sobrelarga ("/" en dos bytes) no es UTF-8 valido: se toma como cp1252.
	expect("sobrelarga", "a\xC0\xAF", 3, "a\xC3\x80\xC2\xAF");

	// Un sustituto codificado en UTF-8 (U+D800) tampoco.
	expect("sustituto utf8", "\xED\xA0\x80", 3, "\xC3\xAD\xC2\xA0\xE2\x82\xAC");

	// Mas alla de U+10FFFF (0x90 es uno de los huecos de cp1252).
	expect("fuera de rango", "\xF4\x90\x80\x80", 4, "\xC3\xB4\xEF\xBF\xBD\xE2\x82\xAC\xE2\x82\xAC");

	// Despues de un BOM de UTF-8, lo invalido pasa a U+FFFD y no a cp1252.
	expect("bom utf8 invalido", "\xEF\xBB\xBF" "a\xFF" "b", 6, "a\xEF\xBF\xBD" "b");

	// UTF-16 con un sustituto alto huerfano, uno bajo suelto y un byte impar al final.
	static const unsigned char orphan[] = { 0xFF, 0xFE, 0x3D, 0xD8, 'a', 0, 0x00, 0xDC, 'b' };
	expect("utf16 huerfano", orphan, sizeof(orphan), "\xEF\xBF\xBD" "a\xEF\xBF\xBD\xEF\xBF\xBD");

	// Un alto al final del archivo, sin nada despues.
	static const unsigned char tail[] = { 0xFE, 0xFF, 0xD8, 0x3D };
	expect("utf16 alto al final", tail, sizeof(tail), "\xEF\xBF\xBD");
}

static void test_edges(void) {
	expect("vacio", NULL, 0, "");
	expect("un byte ascii", "x", 1, "x");
	expect("un byte alto", "\xE9", 1, "é");
	expect("solo bom utf8", "\xEF\xBB\xBF", 3, "");
	expect("solo bom utf16", "\xFF\xFE", 2, "");
	expect("bom utf8 cortado", "\xEF\xBB", 2, "ï»");
}

static void test_sanitize(void) {
	char *out = NULL;
	size_t out_len = 0;

	CHECK(TextEncoding_SanitizeUtf8((const unsigned char *)"ok 한", 6, &out, &out_len) == 0, "Sanitize fallo");
	CHECK(out != NULL && out_len == 6 && strcmp(out, "ok 한") == 0, "Sanitize cambio texto valido");
	free(out);

	CHECK(TextEncoding_SanitizeUtf8((const unsigned char *)"a\xC3" "b\x80", 4, &out, &out_len) == 0, "Sanitize fallo");
	CHECK(out != NULL && strcmp(out, "a\xEF\xBF\xBD" "b\xEF\xBF\xBD") == 0, "Sanitize no reemplazo lo invalido");
	free(out);
}

static void test_fixtures(void) {
	size_t ref_len = 0;
	unsigned char *ref_raw = read_file("fixtures/lyrics/01 bien.lrc", &ref_len);

	if (ref_raw == NULL)
		return;

	size_t good_len = 0;
	char *good = convert(ref_raw, ref_len, &good_len);
	CHECK(good != NULL && good_len == ref_len && memcmp(good, ref_raw, ref_len) == 0, "01 bien.lrc cambio al convertir");

	static const char *same[] = { "06 bom utf8.lrc", "09 utf16 le.lrc", "10 utf16 be.lrc" };

	for (unsigned int i = 0; i < sizeof(same) / sizeof(same[0]); i++) {
		char path[128];
		snprintf(path, sizeof(path), "fixtures/lyrics/%s", same[i]);

		size_t len = 0;
		unsigned char *raw = read_file(path, &len);
		if (raw == NULL)
			continue;

		size_t out_len = 0;
		char *out = convert(raw, len, &out_len);
		CHECK(out != NULL && good != NULL && out_len == good_len && memcmp(out, good, good_len) == 0,
			"%s no da el mismo texto que 01 bien.lrc", same[i]);
		free(out);
		free(raw);
	}

	size_t len = 0;
	unsigned char *raw = read_file("fixtures/lyrics/07 no utf8.lrc", &len);
	if (raw != NULL) {
		size_t out_len = 0;
		char *out = convert(raw, len, &out_len);
		CHECK(out != NULL && strstr(out, "Línea en Latin-1: canción, ñandú") != NULL, "07 no utf8.lrc no se leyo como Latin-1");
		free(out);
		free(raw);
	}

	free(good);
	free(ref_raw);
}

// 10 000 bloques pseudoaleatorios con semilla fija, la mitad con un BOM
// delante, para que pasen por los cuatro caminos.
static void test_random(void) {
	unsigned int seed = 12345;
	unsigned char block[300];
	static const unsigned char boms[4][3] = { { 0xEF, 0xBB, 0xBF }, { 0xFF, 0xFE }, { 0xFE, 0xFF }, { 0 } };
	static const size_t bom_len[4] = { 3, 2, 2, 0 };

	for (int n = 0; n < 10000; n++) {
		seed = seed * 1103515245u + 12345u;
		size_t len = (seed >> 16) % sizeof(block);

		for (size_t i = 0; i < len; i++) {
			seed = seed * 1103515245u + 12345u;
			block[i] = seed >> 16;
		}

		if (n % 2 == 0) {
			int b = (n / 2) % 4;
			if (len >= bom_len[b])
				memcpy(block, boms[b], bom_len[b]);
		}

		size_t out_len = 0;
		char *out = convert(block, len, &out_len);
		free(out);
	}
}

int main(void) {
	test_paths();
	test_invalid();
	test_edges();
	test_sanitize();
	test_fixtures();
	test_random();

	if (failures == 0)
		printf("test_text_encoding: todo bien\n");

	return failures == 0 ? 0 : 1;
}
